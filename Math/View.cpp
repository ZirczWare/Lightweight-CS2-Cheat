#include "View.h"
#include <smmintrin.h>
#include <immintrin.h>
#include <intrin.h>
#include "Vector3.h"
#include "../ImGui/imgui.h"
#include "../Offsets/Offsets.h"
#include "../Memory/Memory.h"
#include "../Console/Console.h"

static bool g_HasAVX2 = false;

static __m128 CachedRow0;
static __m128 CachedRow1;
static __m128 CachedRow3;

static __m256 R0_x, R0_y, R0_z, R0_w;
static __m256 R1_x, R1_y, R1_z, R1_w;
static __m256 R3_x, R3_y, R3_z, R3_w;

void View::Initialize()
{
        int cpuInfo[4];
        __cpuid(cpuInfo, 7);
        bool avx2Supported = (cpuInfo[1] & (1 << 5)) != 0;

        __cpuid(cpuInfo, 1);
        bool fmaSupported = (cpuInfo[2] & (1 << 12)) != 0;

        g_HasAVX2 = avx2Supported && fmaSupported;
}

void View::Update()
{
        Memory::Read(Offsets::client_dll::dwViewMatrix, Matrix);

        __m128 r0 = _mm_loadu_ps(Matrix[0]);
        __m128 r1 = _mm_loadu_ps(Matrix[1]);
        __m128 r3 = _mm_loadu_ps(Matrix[3]);

        __m128 cx = _mm_set1_ps(ScreenCenter.x);
        __m128 cy = _mm_set1_ps(ScreenCenter.y);

        CachedRow0 = _mm_add_ps(_mm_mul_ps(r0, cx), _mm_mul_ps(r3, cx));
        CachedRow1 = _mm_sub_ps(_mm_mul_ps(r3, cy), _mm_mul_ps(r1, cy));
        CachedRow3 = r3;

        if (g_HasAVX2)
        {
                alignas(16) float cr0[4], cr1[4], cr3[4];
                _mm_storeu_ps(cr0, CachedRow0);
                _mm_storeu_ps(cr1, CachedRow1);
                _mm_storeu_ps(cr3, CachedRow3);

                R0_x = _mm256_set1_ps(cr0[0]); R0_y = _mm256_set1_ps(cr0[1]); R0_z = _mm256_set1_ps(cr0[2]); R0_w = _mm256_set1_ps(cr0[3]);
                R1_x = _mm256_set1_ps(cr1[0]); R1_y = _mm256_set1_ps(cr1[1]); R1_z = _mm256_set1_ps(cr1[2]); R1_w = _mm256_set1_ps(cr1[3]);
                R3_x = _mm256_set1_ps(cr3[0]); R3_y = _mm256_set1_ps(cr3[1]); R3_z = _mm256_set1_ps(cr3[2]); R3_w = _mm256_set1_ps(cr3[3]);
        }
}

bool View::WorldToScreen(const Vector3& World, ImVec2& Screen)
{
        alignas(16) static float r0[4], r1[4], r3[4];
        _mm_storeu_ps(r0, CachedRow0);
        _mm_storeu_ps(r1, CachedRow1);
        _mm_storeu_ps(r3, CachedRow3);

        float w = r3[0] * World.x + r3[1] * World.y + r3[2] * World.z + r3[3];

        if (w < 0.01f)
        {
                Screen = { 0.f, 0.f };
                return false;
        }

        float invW = 1.0f / w;
        Screen.x = (r0[0] * World.x + r0[1] * World.y + r0[2] * World.z + r0[3]) * invW;
        Screen.y = (r1[0] * World.x + r1[1] * World.y + r1[2] * World.z + r1[3]) * invW;

        return true;
}

void View::WorldToScreenBulk(const Vector3* inWorld, ImVec2* outScreen, bool* outVisibility, size_t count)
{
        size_t i = 0;

        if (g_HasAVX2)
        {
                const __m256 minW = _mm256_set1_ps(0.01f);
                for (; i + 7 < count; i += 8)
                {
                        __m256 vx = _mm256_setr_ps(inWorld[i].x, inWorld[i + 1].x, inWorld[i + 2].x, inWorld[i + 3].x,
                                inWorld[i + 4].x, inWorld[i + 5].x, inWorld[i + 6].x, inWorld[i + 7].x);
                        __m256 vy = _mm256_setr_ps(inWorld[i].y, inWorld[i + 1].y, inWorld[i + 2].y, inWorld[i + 3].y,
                                inWorld[i + 4].y, inWorld[i + 5].y, inWorld[i + 6].y, inWorld[i + 7].y);
                        __m256 vz = _mm256_setr_ps(inWorld[i].z, inWorld[i + 1].z, inWorld[i + 2].z, inWorld[i + 3].z,
                                inWorld[i + 4].z, inWorld[i + 5].z, inWorld[i + 6].z, inWorld[i + 7].z);

                        __m256 w = _mm256_fmadd_ps(vz, R3_z, R3_w);
                        w = _mm256_fmadd_ps(vy, R3_y, w);
                        w = _mm256_fmadd_ps(vx, R3_x, w);

                        __m256 mask = _mm256_cmp_ps(w, minW, _CMP_GE_OQ);
                        int maskInt = _mm256_movemask_ps(mask);

                        __m256 x = _mm256_fmadd_ps(vz, R0_z, R0_w);
                        x = _mm256_fmadd_ps(vy, R0_y, x);
                        x = _mm256_fmadd_ps(vx, R0_x, x);

                        __m256 y = _mm256_fmadd_ps(vz, R1_z, R1_w);
                        y = _mm256_fmadd_ps(vy, R1_y, y);
                        y = _mm256_fmadd_ps(vx, R1_x, y);

                        __m256 invW = _mm256_rcp_ps(w);

                        x = _mm256_mul_ps(x, invW);
                        y = _mm256_mul_ps(y, invW);

                        alignas(32) float resX[8], resY[8];
                        _mm256_storeu_ps(resX, x);
                        _mm256_storeu_ps(resY, y);

                        for (int k = 0; k < 8; ++k)
                        {
                                bool visible = (maskInt & (1 << k)) != 0;
                                outVisibility[i + k] = visible;
                                outScreen[i + k] = visible ? ImVec2{ resX[k], resY[k] } : ImVec2{ 0.f, 0.f };
                        }
                }
        }

        for (; i < count; ++i)
                outVisibility[i] = WorldToScreen(inWorld[i], outScreen[i]);
}