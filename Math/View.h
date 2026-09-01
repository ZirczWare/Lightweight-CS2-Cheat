#pragma once

#include "Vector3.h"
#include <immintrin.h>
#include "../ImGui/imgui.h"

namespace View
{
	inline float Matrix[4][4];
	inline ImVec2 ScreenCenter;

	void Initialize();
	void Update();
	bool WorldToScreen(const Vector3& World, ImVec2& Screen);
	void WorldToScreenBulk(const Vector3* inWorld, ImVec2* outScreen, bool* outVisibility, size_t count);
}