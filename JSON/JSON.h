#pragma once

#include <nlohmann/json.hpp>

enum JSONRESULT { SUCCESS, CANNOT_OPEN, PARSE_ERROR };

namespace JSON
{
	JSONRESULT LoadFromDisk(nlohmann::json& j, const char* Filepath);
}