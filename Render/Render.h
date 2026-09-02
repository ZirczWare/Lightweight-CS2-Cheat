#pragma once

#include "../ImGui/imgui.h"

namespace Render
{
	struct Data
	{
		ImVec4 Boxes[64]{};
		bool VisibleOnScreen[64]{};
		size_t Count{};
	};

	inline Data FrontBuffer{};
	inline Data BackBuffer{};
}