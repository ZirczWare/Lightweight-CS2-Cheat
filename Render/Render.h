#pragma once

#include "../ImGui/imgui.h"
#include "../Entity/Entity.h"

namespace Render
{
	struct Data
	{
		ImVec4 Boxes[Entity::MAX_ENEMIES]{};
		bool VisibleOnScreen[Entity::MAX_ENEMIES]{};
		size_t Count{};
	};

	inline Data FrontBuffer{};
	inline Data BackBuffer{};
}