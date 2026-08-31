#pragma once

#include <string>

namespace HTTP
{
	bool Download(const char* URL, const char* Filename);
	std::string GetError();
};