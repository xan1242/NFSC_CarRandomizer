#include "framework.h"
#include "ModPath.hpp"
#include "CarRandomizer.hpp"
#include "DebugVehicleSelection.hpp"
#include "FEPlayerCarDB.hpp"
#include "UserProfile.hpp"
#include "EAXSound.hpp"
#include "NIS.hpp"

#include <filesystem>

#include "../includes/injector/injector.hpp"

uintptr_t pInitializeEverything;

static void PostInit()
{
#ifdef _DEBUG
	freopen("CON", "wb", stdout);
	freopen("CON", "wb", stderr);
#endif

	DebugVehicleSelection::Init();
	FEPlayerCarDB::Init();
	UserProfile::Init();
	EAXSound::Init();
	NIS::Init();

	CarRandomizer::Init();
}

static bool isNoSoundEnabled_Arg(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i) 
	{
		if (strcmp(argv[i], "-nosound") == 0) 
		{
			return true;
		}
	}
	return false;
}

static bool isNoTrafficCarsEnabled_Arg(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-rndcars_notraffic") == 0)
		{
			return true;
		}
	}
	return false;
}


static bool isTrafficSemisEnabled_Arg(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-rndcars_includesemis") == 0)
		{
			return true;
		}
	}
	return false;
}

static bool isNoCopCarsEnabled_Arg(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-rndcars_nocops") == 0)
		{
			return true;
		}
	}
	return false;
}

static bool isNoRegularCarsEnabled_Arg(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-rndcars_noregular") == 0)
		{
			return true;
		}
	}
	return false;
}

static void InitializeEverything_Hook(int argc, char** argv)
{
	bool bNoSound = isNoSoundEnabled_Arg(argc, argv);
	if (bNoSound)
		*reinterpret_cast<int*>(0x00A631B8) = 0;

	if (isNoTrafficCarsEnabled_Arg(argc, argv))
		CarRandomizer::SetExcludeTrafficCars(true);

	if (isTrafficSemisEnabled_Arg(argc, argv))
		CarRandomizer::SetIncludeTrafficSemis(true);

	if (isNoCopCarsEnabled_Arg(argc, argv))
		CarRandomizer::SetExcludeCopCars(true);

	if (isNoRegularCarsEnabled_Arg(argc, argv))
		CarRandomizer::SetExcludeRegularCars(true);

	reinterpret_cast<void(*)(int, char**)>(pInitializeEverything)(argc, argv);

	PostInit();
}

static bool ShouldDisableSound()
{
	try
	{
		std::filesystem::path chkPath = ModPath::GetThisModulePath<std::filesystem::path>().parent_path();
		chkPath /= "nosound.txt";

		return std::filesystem::exists(chkPath);
	}
	catch (...)
	{
		return false;
	}
}

static bool ShouldDisableTrafficList()
{
	try
	{
		std::filesystem::path chkPath = ModPath::GetThisModulePath<std::filesystem::path>().parent_path();
		chkPath /= "rndcars_notraffic.txt";

		return std::filesystem::exists(chkPath);
	}
	catch (...)
	{
		return false;
	}
}

static bool ShouldEnableTrafficSemiList()
{
	try
	{
		std::filesystem::path chkPath = ModPath::GetThisModulePath<std::filesystem::path>().parent_path();
		chkPath /= "rndcars_includesemis.txt";

		return std::filesystem::exists(chkPath);
	}
	catch (...)
	{
		return false;
	}
}

static bool ShouldDisableCopList()
{
	try
	{
		std::filesystem::path chkPath = ModPath::GetThisModulePath<std::filesystem::path>().parent_path();
		chkPath /= "rndcars_nocops.txt";

		return std::filesystem::exists(chkPath);
	}
	catch (...)
	{
		return false;
	}
}

static bool ShouldDisableRegularList()
{
	try
	{
		std::filesystem::path chkPath = ModPath::GetThisModulePath<std::filesystem::path>().parent_path();
		chkPath /= "rndcars_noregular.txt";

		return std::filesystem::exists(chkPath);
	}
	catch (...)
	{
		return false;
	}
}

static void PreInit()
{
	if (ShouldDisableSound())
		*reinterpret_cast<int*>(0x00A631B8) = 0;

	if (ShouldDisableTrafficList())
		CarRandomizer::SetExcludeTrafficCars(true);

	if (ShouldEnableTrafficSemiList())
		CarRandomizer::SetIncludeTrafficSemis(true);

	if (ShouldDisableCopList())
		CarRandomizer::SetExcludeCopCars(true);

	if (ShouldDisableRegularList())
		CarRandomizer::SetExcludeRegularCars(true);

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

