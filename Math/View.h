#pragma once

#include "Vector3.h"
#include <immintrin.h>
#include "../ImGui/imgui.h"

namespace View
{
	inline float Matrix[4][4];
	inline ImVec2 ScreenCenter;

	void Update();
	bool WorldToScreen(const Vector3& World, ImVec2& Screen);
}