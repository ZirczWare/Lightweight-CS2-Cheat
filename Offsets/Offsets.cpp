#include "Offsets.h"
#include "../HTTP/HTTP.h"
#include "../JSON/JSON.h"
#include "../Memory/Memory.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include "../A2X Dumper/A2XDumper.h"
#include "../Console/Console.h"

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
	JSONRESULT Result = JSON::LoadFromDisk(JSON, Filepath);

	switch (Result)
	{
	case SUCCESS:
		return true;

	case CANNOT_OPEN:
		ErrorMessage = "Couldn't open JSON file: " + string(Filepath);

	case PARSE_ERROR:
		ErrorMessage = "Error parsing JSON file: " + string(Filepath);

	default:
		ErrorMessage = "Unknown JSON error on file: " + string(Filepath);
	}

	return false;
}

static bool LoadJSONFromWeb(json& JSON, const char* Address, const char* Filepath)
{
	bool Success = HTTP::Download(Address, Filepath);
	if (!Success)
	{
		ErrorMessage = HTTP::GetError();
		return false;
	}

	return LoadJSON(JSON, Filepath);
}

static bool HandleJSONLoadingFromWeb()
{
	bool Success;

	Success = LoadJSONFromWeb(CLIENT_DLL, CLIENT_DLL_ADDRESS, CLIENT_DLL_FILENAME);
	if (!Success)
	{
		ErrorMessage = HTTP::GetError();
		return false;
	}

	Success = LoadJSONFromWeb(OFFSETS, OFFSETS_ADDRESS, OFFSETS_FILENAME);
	if (!Success)
	{
		ErrorMessage = HTTP::GetError();
		return false;
	}

	return true;
}

static void ExtractEngine2DLL()
{
	const auto& JSON = OFFSETS.at("engine2.dll");
	const auto& ClientDLL = Memory::GetClientDLL();

	Offsets::engine2_dll::dwBuildNumber = ClientDLL + JSON.at("dwBuildNumber").get<ptrdiff_t>();
}

static void ExtractClientDLL()
{
	const auto& JSON = OFFSETS.at("client.dll");
	const auto& ClientDLL = Memory::GetClientDLL();

	Offsets::client_dll::dwEntityList = ClientDLL + JSON.at("dwEntityList").get<ptrdiff_t>();
	Offsets::client_dll::dwViewMatrix = ClientDLL + JSON.at("dwViewMatrix").get<ptrdiff_t>();
	Offsets::client_dll::dwLocalPlayerPawn = ClientDLL + JSON.at("dwLocalPlayerPawn").get<ptrdiff_t>();
}

static void ExtractCCSPlayerController()
{
	const auto& JSON = CLIENT_DLL.at("client.dll").at("classes").at("CCSPlayerController").at("fields");

	Offsets::CCSPlayerController::m_hPlayerPawn = JSON.at("m_hPlayerPawn").get<ptrdiff_t>();
	Offsets::CCSPlayerController::m_bPawnIsAlive = JSON.at("m_bPawnIsAlive").get<ptrdiff_t>();
}

static void ExtractC_BaseEntity()
{
	const auto& JSON = CLIENT_DLL.at("client.dll").at("classes").at("C_BaseEntity").at("fields");

	Offsets::C_BaseEntity::m_pGameSceneNode = JSON.at("m_pGameSceneNode").get<ptrdiff_t>();
	Offsets::C_BaseEntity::m_iTeamNum = JSON.at("m_iTeamNum").get<ptrdiff_t>();
}

static void ExtractCGameSceneNode()
{
	const auto& JSON = CLIENT_DLL.at("client.dll").at("classes").at("CGameSceneNode").at("fields");

	Offsets::CGameSceneNode::m_vecOrigin = JSON.at("m_vecOrigin").get<ptrdiff_t>();
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
		ExtractClientDLL();
		ExtractCCSPlayerController();
		ExtractC_BaseEntity();
		ExtractCGameSceneNode();
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
		filesystem::remove(CLIENT_DLL_FILENAME);
		filesystem::remove(OFFSETS_FILENAME);
	}
	catch (const filesystem_error& e)
	{
		ErrorMessage = "Error removing old files: " + string(e.what()) + ", code: " + string(e.code().message());
		return false;
	}

	return true;
}

static bool LoadOffsetsAndUpdatBuildnumber()
{
	bool LoadedAllOffsets = LoadAllOffsets();
	if (!LoadedAllOffsets)
		return false;

	bool OldBuildnumberUpdated = UpdateOldBuildnumber();
	if (!OldBuildnumberUpdated)
		return false;

	CLIENT_DLL = OFFSETS = nullptr;

	return true;
}

static bool HandleJSONLoadingLocally()
{
	bool DumpSuccesful = A2XDumper::Dump();
	if (!DumpSuccesful)
	{
		ErrorMessage = "Error dumping offsets locally";
		return false;
	}

	bool JSONLoadSuccessful;

	JSONLoadSuccessful = LoadJSON(CLIENT_DLL, CLIENT_DLL_FILENAME);
	if (!JSONLoadSuccessful)
		return false;

	JSONLoadSuccessful = LoadJSON(OFFSETS, OFFSETS_FILENAME);
	if (!JSONLoadSuccessful)
		return false;

	return true;
}

bool Offsets::Init()
{
	bool BuildnumberLoadingSuccess = HandleBuildnumberLoading();
	if (!BuildnumberLoadingSuccess)
		return false;

	bool OffsetsGood;

	OffsetsGood = AreOffsetsGood();
	if (OffsetsGood)
	{
		bool LoadedAllOffsets = LoadAllOffsets();
		if (LoadedAllOffsets)
		{
			CLIENT_DLL = OFFSETS = nullptr;
			return true;
		}
	}

	bool RemovedPreviousOffsets;

	RemovedPreviousOffsets = RemovePreviousJSONFiles();
	if (!RemovedPreviousOffsets)
		return false;

	bool JSONLocalLoadingSuccess, LoadedOffsetsAndUpdatedBuildnumber;

	JSONLocalLoadingSuccess = HandleJSONLoadingLocally();
	if (JSONLocalLoadingSuccess)
	{
		LoadedOffsetsAndUpdatedBuildnumber = LoadOffsetsAndUpdatBuildnumber();
		if (LoadedOffsetsAndUpdatedBuildnumber)
			return true;
	}

	RemovedPreviousOffsets = RemovePreviousJSONFiles();
	if (!RemovedPreviousOffsets)
		return false;

	bool JSONWebLoadingSuccess = HandleJSONLoadingFromWeb();
	if (!JSONWebLoadingSuccess)
		return false;

	LoadedOffsetsAndUpdatedBuildnumber = LoadOffsetsAndUpdatBuildnumber();
	if (!LoadedOffsetsAndUpdatedBuildnumber)
		return false;

	return true;
}

string Offsets::GetError()
{
	return ErrorMessage;
}