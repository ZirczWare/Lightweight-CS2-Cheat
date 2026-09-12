#pragma once

#include <cstdarg>
#include <cstdint>

namespace Entity
{
	struct Data
	{
		std::uintptr_t AbsOriginAddress{};
	};

	constexpr std::uint8_t MAX_ENTITIES = 64;
	constexpr std::uint8_t MAX_ENEMIES = MAX_ENTITIES / 2;
}