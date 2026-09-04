#pragma once

#include <cstdint>
#include <cstdarg>

namespace Entity
{
	struct Data
	{
		std::uintptr_t AbsOriginAddress{};
	};

	constexpr int MAX_ENTITIES = 64;
	constexpr int MAX_ENEMIES = MAX_ENTITIES / 2;
}