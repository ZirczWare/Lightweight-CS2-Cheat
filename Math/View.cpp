#include "View.h"
#include "Vector3.h"
#include "../ImGui/imgui.h"
#include "../Offsets/Offsets.h"
#include "../Memory/Memory.h"
#include "../Console/Console.h"

static __m128 CachedRow0;
static __m128 CachedRow1;
static __m128 CachedRow3;

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