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

static void InitializeEverything_Hook(int argc, char** argv)
{
	bool bNoSound = isNoSoundEnabled_Arg(argc, argv);
	if (bNoSound)
		*reinterpret_cast<int*>(0x00A631B8) = 0;

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


static void PreInit()
{
	if (ShouldDisableSound())
		*reinterpret_cast<int*>(0x00A631B8) = 0;

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

