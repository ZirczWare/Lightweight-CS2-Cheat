#include "JSON.h"
#include <nlohmann/json.hpp>
#include <fstream>

JSONRESULT JSON::LoadFromDisk(nlohmann::json& j, const char* Filepath)
{
	using json = nlohmann::json;

	j = nullptr;

	std::ifstream inputFile(Filepath);
	if (!inputFile.is_open())
		return CANNOT_OPEN;

	try
	{
		j = json::parse(inputFile);
	}
	catch (const json::parse_error&)
	{
		j = nullptr;
		return PARSE_ERROR;
	}

	inputFile.close();

	return SUCCESS;
}