#include "framework.h"
#include "ModPath.hpp"
#include "CarRandomizer.hpp"
#include "DebugVehicleSelection.hpp"
#include "FEPlayerCarDB.hpp"
#include "UserProfile.hpp"
#include "EAXSound.hpp"
#include "NIS.hpp"

#include <filesystem>

#include "injector/injector.hpp"
#include "mINI/src/mini/ini.h"

uintptr_t pInitializeEverything;

static bool ReadIniBoolValue(mINI::INIStructure& ini, const std::string& section, const std::string& key, bool& val)
{
	if (!ini.has(section))
		return val;

	if (!ini[section].has(key))
		return val;

	// trim inline comment
	std::string valStr = ini[section][key];
	size_t posComment = valStr.find(';');
	if (posComment != valStr.npos)
	{
		valStr = valStr.substr(0, posComment);
		
		// trim space
		size_t posSpace = valStr.find(' ');
		if (posSpace != valStr.npos)
		{
			valStr = valStr.substr(0, posSpace);
		}
	}

	if (valStr == "true")
	{
		val = true;
		return val;
	}

	try
	{
		int in_val = std::stoi(valStr, nullptr, 0);
		val = in_val != 0;
	}
	catch (...)
	{
		return val;
	}

	return val;
}

static void InitConfig()
{
	std::filesystem::path iniPath = ModPath::GetThisModulePath<std::filesystem::path>().replace_extension("ini");

	mINI::INIFile iniFile(iniPath);
	mINI::INIStructure ini;
	
	if (!iniFile.read(ini))
		return;

	if (!ini.has("Main"))
		return;

	bool bNoSound = false;
	bool bNoRegular = false;
	bool bNoTraffic = false;
	bool bNoCops = false;
	bool bIncludeSemis = false;

	ReadIniBoolValue(ini, "Main", "NoSound",      bNoSound);
	ReadIniBoolValue(ini, "Main", "NoRegular",    bNoRegular);
	ReadIniBoolValue(ini, "Main", "NoTraffic",    bNoTraffic);
	ReadIniBoolValue(ini, "Main", "NoCops",       bNoCops);
	ReadIniBoolValue(ini, "Main", "IncludeSemis", bIncludeSemis);

	CarRandomizer::SetExcludeRegularCars(bNoRegular);
	CarRandomizer::SetExcludeTrafficCars(bNoTraffic);
	CarRandomizer::SetExcludeCopCars(bNoCops);
	CarRandomizer::SetIncludeTrafficSemis(bIncludeSemis);

	if (bNoSound)
	{
		uintptr_t loc_A631B8 = 0xA631B8;
		*reinterpret_cast<int*>(loc_A631B8) = 0;
	}
}

static void PostInit()
{
#ifdef _DEBUG
	freopen("CON", "wb", stdout);
	freopen("CON", "wb", stderr);
#endif

	InitConfig();

	DebugVehicleSelection::Init();
	FEPlayerCarDB::Init();
	UserProfile::Init();
	EAXSound::Init();
	NIS::Init();

	CarRandomizer::Init();
}

static void InitializeEverything_Hook(int argc, char** argv)
{
	reinterpret_cast<void(*)(int, char**)>(pInitializeEverything)(argc, argv);

	PostInit();
}

static void PreInit()
{
	uintptr_t loc_6B8EBC = 0x6B8EBC;

	pInitializeEverything = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6B8EBC));
	injector::MakeCALL(loc_6B8EBC, InitializeEverything_Hook);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		PreInit();
	}
	return TRUE;
}
