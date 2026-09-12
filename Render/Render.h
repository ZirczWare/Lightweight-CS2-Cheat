#pragma once

#include "../Entity/Entity.h"
#include "../ImGui/imgui.h"
#include <cstdint>

namespace Render
{
	struct Data
	{
		ImVec4 Boxes[Entity::MAX_ENEMIES]{};
		bool VisibleOnScreen[Entity::MAX_ENEMIES]{};
		std::uint8_t Count{};
	};

	inline Data FrontBuffer{};
	inline Data BackBuffer{};
}