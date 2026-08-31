#include "Offsets.h"
#include "../HTTP/HTTP.h"
#include "../JSON/JSON.h"
#include "../Memory/Memory.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using nlohmann::json;
using namespace std;
using namespace filesystem;

static json CLIENT_DLL, OFFSETS;

constexpr const char* CLIENT_DLL_ADDRESS = "https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/client_dll.json";
constexpr const char* CLIENT_DLL_FILENAME = "client_dll.json";

constexpr const char* OFFSETS_ADDRESS = "https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/offsets.json";
constexpr const char* OFFSETS_FILENAME = "offsets.json";

constexpr const char* OLD_BUILDNUMBER_FILENAME = "build.info";

static int64_t OldBuildNumber = 0;
static int64_t NewBuildNumber = 0;

static string ErrorMessage = "";

static bool LoadJSON(json& JSON, const char* Filepath)
{
	JSONRESULT Result = JSON::LoadJSONFromDisk(JSON, Filepath);

	switch (Result)
	{
	case SUCCESS:
		return true;

	case CANNOT_OPEN:
		ErrorMessage = "Couldn't open JSON file: " + string(Filepath);
		return false;

	case PARSE_ERROR:
		ErrorMessage = "Error parsing JSON file: " + string(Filepath);
		return false;

	default:
		ErrorMessage = "Unknown JSON error on file: " + string(Filepath);
		return false;
	}
}

static bool HandleJSONLoading(json& JSON, const char* Address, const char* Filepath)
{
	if (exists(Filepath))
		return LoadJSON(JSON, Filepath);
	
	bool Success = HTTP::Download(Address, Filepath);
	if (!Success)
	{
		ErrorMessage = HTTP::GetError();
		return false;
	}

	return LoadJSON(JSON, Filepath);
}

static void ExtractEngine2DLL()
{
	const auto& JSON = OFFSETS.at("engine2.dll");

	Offsets::engine2_dll::dwBuildNumber = JSON.at("dwBuildNumber").get<uintptr_t>();
	Offsets::engine2_dll::dwNetworkGameClient = JSON.at("dwNetworkGameClient").get<uintptr_t>();
	Offsets::engine2_dll::dwNetworkGameClient_clientTickCount = JSON.at("dwNetworkGameClient_clientTickCount").get<uintptr_t>();
	Offsets::engine2_dll::dwNetworkGameClient_deltaTick = JSON.at("dwNetworkGameClient_deltaTick").get<uintptr_t>();
	Offsets::engine2_dll::dwNetworkGameClient_isBackgroundMap = JSON.at("dwNetworkGameClient_isBackgroundMap").get<uintptr_t>();
	Offsets::engine2_dll::dwNetworkGameClient_serverTickCount = JSON.at("dwNetworkGameClient_serverTickCount").get<uintptr_t>();
	Offsets::engine2_dll::dwNetworkGameClient_signOnState = JSON.at("dwNetworkGameClient_signOnState").get<uintptr_t>();
}

static void GetNewBuildnumber()
{
	NewBuildNumber = 0;

	std::ptrdiff_t BuildnumberOffset;

	try
	{
		BuildnumberOffset = OFFSETS.at("engine2.dll").at("dwBuildNumber");
	}
	catch (const json::exception& e)
	{
		ErrorMessage = "JSON parse error: " + string(e.what());
		return; // NewBuildNumber stays 0, passable check will fail
	}

	Memory::Read(Memory::GetEngine2DLL() + BuildnumberOffset, NewBuildNumber);
}

static bool NewBuildnumberPassable()
{
	GetNewBuildnumber();

	if (NewBuildNumber == 0 || NewBuildNumber > 20000) // Probably memory garbage, means bad offset
	{
		ErrorMessage = "Buildnumber invalid";
		return false;
	}

	return true;
}

static bool TestBuildnumberValidity()
{
	bool Passable = NewBuildnumberPassable();
	if (!Passable)
		return false;

	return OldBuildNumber == NewBuildNumber;
}

static bool AreOffsetsGood()
{
	if (!exists(CLIENT_DLL_FILENAME) || !exists(OFFSETS_FILENAME))
		return false;

	if (!LoadJSON(CLIENT_DLL, CLIENT_DLL_FILENAME) || !LoadJSON(OFFSETS, OFFSETS_FILENAME))
		return false;

	bool BuildnumberValid = TestBuildnumberValidity();
	if (!BuildnumberValid)
	{
		NewBuildNumber = 0;
		return false;
	}

	return true;
}

static bool WriteNewBuildnumber()
{
	ofstream BuildnumberFile(OLD_BUILDNUMBER_FILENAME);
	if (!BuildnumberFile.is_open())
	{
		ErrorMessage = "Failed to create file: " + string(OLD_BUILDNUMBER_FILENAME);
		return false;
	}

	BuildnumberFile << to_string(NewBuildNumber);
	BuildnumberFile.close();

	return true;
}

static bool UpdateOldBuildnumber()
{
	bool Passable = NewBuildnumberPassable();
	if (!Passable)
		return false;

	return WriteNewBuildnumber();
}

static bool LoadOldBuildnumber()
{
	ifstream BuildnumberFile(OLD_BUILDNUMBER_FILENAME);
	if (!BuildnumberFile.is_open())
	{
		ErrorMessage = "Failed to open buildnumber file";
		return false;
	}

	if (BuildnumberFile >> OldBuildNumber)
		return true;

	OldBuildNumber = 0;
	return true;
}

static bool HandleBuildnumberLoading()
{
	if (!exists(OLD_BUILDNUMBER_FILENAME))
		return true; // First time setup

	return LoadOldBuildnumber();
}

static bool LoadAllOffsets()
{
	try
	{
		ExtractEngine2DLL();
	}
	catch (const json::exception& e)
	{
		ErrorMessage = "JSON parse error: " + string(e.what());
		return false;
	}

	return true;
}

static bool RemovePreviousJSONFiles()
{
	try
	{
		remove(CLIENT_DLL_FILENAME);
		remove(OFFSETS_FILENAME);
	}
	catch (const filesystem_error& e)
	{
		ErrorMessage = "Error removing old files: " + string(e.what()) + ", code: " + string(e.code().message());
		return false;
	}

	return true;
}

bool Offsets::Init()
{
	bool BuildnumberLoadingSuccess = HandleBuildnumberLoading();
	if (!BuildnumberLoadingSuccess)
		return false;

	bool OffsetsGood, LoadedAllOffsets;

	OffsetsGood = AreOffsetsGood();
	if (OffsetsGood)
	{
		LoadedAllOffsets = LoadAllOffsets();
		if (LoadedAllOffsets)
			return true;
	}

	CLIENT_DLL = OFFSETS = nullptr;

	bool RemovedPreviousOffsets = RemovePreviousJSONFiles();
	if (!RemovedPreviousOffsets)
		return false;

	bool JSONLoadingSuccess;

	JSONLoadingSuccess = HandleJSONLoading(CLIENT_DLL, CLIENT_DLL_ADDRESS, CLIENT_DLL_FILENAME);
	if (!JSONLoadingSuccess)
		return false;

	JSONLoadingSuccess = HandleJSONLoading(OFFSETS, OFFSETS_ADDRESS, OFFSETS_FILENAME);
	if (!JSONLoadingSuccess)
		return false;

	LoadedAllOffsets = LoadAllOffsets();
	if (!LoadedAllOffsets)
		return false;

	bool OldBuildnumberUpdated = UpdateOldBuildnumber();
	if (!OldBuildnumberUpdated)
		return false;

	return true;
}

string Offsets::GetError()
{
	return ErrorMessage;
}