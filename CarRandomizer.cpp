#include "framework.h"
#include "CarRandomizer.hpp"
#include "DebugVehicleSelection.hpp"
#include "UserProfile.hpp"
#include "FEPlayerCarDB.hpp"
#include "EAXSound.hpp"
#include "NIS.hpp"
#include "../includes/injector/injector.hpp"
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <shlobj_core.h>

//
// ISSUES:
// - car sounds cause crashes -- probably fixed? EAXSound::ReStartRace fixed it maybe...
// - in final boss race, after the first race, because of the car switch another phantom player appears and causes a crash at the finish (GCareer::IsPlayerCrewWinner crashes because it can't find a vehicle for player checks) -- fixed by nuking the player
// - sometimes car sounds don't appear
// - just car sounds in general cause crashes - NIS_RevManager crashes if EAXSound::ReStartRace is called before a NIS... many edge cases exist...
// - (not a big issue) maybe add presetride into the mix
//

namespace CarRandomizer
{
	uintptr_t pGetTicker;

	uintptr_t pGame_RaceDone;
	uintptr_t pGame_DoPostBossFlow;
	uintptr_t pGameFlowManager_LoadFrontend;
	uintptr_t pAttribGenPVehicle;
	uintptr_t p_bRandom;
	//uintptr_t p_SHGetFolderPathA;
	uintptr_t pDALCareer_SetCar;
	uintptr_t pGRaceStatus_GetRacerCount;
	uintptr_t pGRaceParameters_GetEventID;
	uintptr_t pGRaceParameters_GetRaceType;
	uintptr_t pGRaceParameters_GetIsBossRace;

	uintptr_t pGameFlowManagerState;

	std::unordered_map<uint32_t, std::string> sCarLibraryMap;
	std::unordered_map<uint32_t, uint32_t> sCarLibraryHandles;
	std::vector<uint32_t> sCarLibraryKeys;

	uint32_t CurrentVehicle;

	bool bWasInPostRace;
	bool bWasBossCanyonRace;
	bool bInPostBossFlow;

	bool bQueueEAXRefreshFromNIS;

	//bool bTestTrigger;
	//char testVehicleName[128];

	static int GetGameFlowManagerState()
	{
		if (!pGameFlowManagerState)
			return -1;
		return *reinterpret_cast<int*>(pGameFlowManagerState);
	}

	static unsigned int bRandom(int range)
	{
		if (!p_bRandom)
			return 1;

		return reinterpret_cast<unsigned int(*)(int)>(p_bRandom)(range);
	}

	//static HRESULT WINAPI SHGetFolderPathA_Game(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
	//{
	//	if (!p_SHGetFolderPathA)
	//		return SHGetFolderPathA(hwnd, csidl, hToken, dwFlags, pszPath);
	//
	//	return reinterpret_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(p_SHGetFolderPathA)(hwnd, csidl, hToken, dwFlags, pszPath);
	//}

	static bool DALCareer_SetCar(int handle)
	{
		if (!pDALCareer_SetCar)
			return false;

		return reinterpret_cast<bool(__stdcall*)(int)>(pDALCareer_SetCar)(handle);
	}

	static void SetVehicle(const char* vehicleName)
	{
		if (GetGameFlowManagerState() != 6)
			return;

		uintptr_t pDebugVehicleSelection = DebugVehicleSelection::Get();
		if (!pDebugVehicleSelection)
			return;

		DebugVehicleSelection::SwitchPlayerVehicle(pDebugVehicleSelection, vehicleName);

		if (!bInPostBossFlow)
		{
			if (NIS::bIsInNIS())
				bQueueEAXRefreshFromNIS = true;
			else
				EAXSound::Refresh();
		}
	}

	static void SetVehicle(uint32_t key)
	{
		SetVehicle(sCarLibraryMap[key].c_str());
		DALCareer_SetCar(sCarLibraryHandles[key]);
	}

#ifdef _DEBUG
	//bool bDoSecondCar;

	const uint32_t DebugCars[] =
	{
		0x950F8F92, // player_cop_gto
		0x655FEEBD, // darius
		0xAD54F689, // zonda
		0xE4DB67B3, // murcielago640
		0xB336162E, // player_cop_corvette
	};

	int DebugCarCounter = 0;

#endif

	static void SetRandomVehicle()
	{
#ifdef _DEBUG
		SetVehicle(DebugCars[DebugCarCounter]);
		DebugCarCounter++;
		DebugCarCounter %= _countof(DebugCars);
#else
		unsigned int index = bRandom(sCarLibraryKeys.size());

		if (CurrentVehicle == sCarLibraryKeys[index])
		{
			bWasInPostRace = false;
			return;
		}

		CurrentVehicle = sCarLibraryKeys[index];

		SetVehicle(sCarLibraryKeys[index]);
#endif
		bWasInPostRace = false;
		bWasBossCanyonRace = false;
		bInPostBossFlow = false;
	}

	//
	// not necessary since the active vehicle is actually set properly
	// 
	
	//static void SaveActiveVehicle(const char* name)
	//{
	//	char gameSaveDirPath[MAX_PATH];
	//	SHGetFolderPathA_Game(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, gameSaveDirPath);
	//
	//	std::filesystem::path savePath = gameSaveDirPath;
	//	savePath /= "NFS Carbon";
	//	savePath /= name;
	//	savePath += "_RNDCAR";
	//
	//	try
	//	{
	//		std::ofstream ofile(savePath, std::ios::binary);
	//		if (!ofile.is_open())
	//			return;
	//
	//		ofile.write((const char*)&CurrentVehicle, sizeof(CurrentVehicle));
	//
	//		ofile.close();
	//	}
	//	catch (...)
	//	{
	//		return;
	//	}
	//}
	//
	//static void LoadActiveVehicle(const char* name)
	//{
	//	char gameSaveDirPath[MAX_PATH];
	//	SHGetFolderPathA_Game(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, gameSaveDirPath);
	//
	//	std::filesystem::path savePath = gameSaveDirPath;
	//	savePath /= "NFS Carbon";
	//	savePath /= name;
	//	savePath += "_RNDCAR";
	//
	//	try
	//	{
	//		std::ifstream ifile(savePath, std::ios::binary);
	//		if (!ifile.is_open())
	//			return;
	//
	//		ifile.read((char*)&CurrentVehicle, sizeof(CurrentVehicle));
	//
	//		ifile.close();
	//	}
	//	catch (...)
	//	{
	//		return;
	//	}
	//}

	//static void DoDebugVehicleSwitcher()
	//{
	//	SetVehicle(testVehicleName);
	//
	//	//uintptr_t pDebugVehicleSelection = DebugVehicleSelection::Get();
	//	//if (!pDebugVehicleSelection)
	//	//	return;
	//	//
	//	//DebugVehicleSelection::SwitchPlayerVehicle(pDebugVehicleSelection, testVehicleName);
	//	//
	//	//
	//	////CommitAudioAssets();
	//	//
	//	//EAXAudioRefresh();
	//
	//	//uintptr_t pEAXSound = GetEAXSound();
	//	//if (pEAXSound)
	//	//{
	//	//	*reinterpret_cast<bool*>(0xA8BA77) = true;
	//	//	EAXSound_RefreshNewGameplay(pEAXSound);
	//	//}
	//
	//	//CommitAudioAssets();
	//	//TriggerAudioRestartRace();
	//	//bQueueAudioCommit = true;
	//	//bQueueAudioRestartRace = true;
	//}

	static void HandleMainLoop()
	{
		if (bQueueEAXRefreshFromNIS)
		{
			if (!NIS::bIsInNIS())
			{
				bQueueEAXRefreshFromNIS = false;
				if (!bInPostBossFlow)
					EAXSound::Refresh();
			}
		}

		//if (bQueueAudioRestartRace)
		//{
		//	bQueueAudioRestartRace = false;
		//	//CommitAudioAssets();
		//	TriggerAudioRestartRace();
		//}
	
		//if (bQueueAudioCommit)
		//{
		//	bQueueAudioCommit = false;
		//	bQueueAudioRestartRace = true;
		//	CommitAudioAssets();
		//}
	
		//if (QueueEAXAudioRefresh)
		//{
		//	if (QueueEAXAudioRefresh == 1)
		//		QueueEAXAudioRefresh++;
		//	else if (QueueEAXAudioRefresh <= 2)
		//	{
		//		QueueEAXAudioRefresh = 0;
		//		EAXAudioRefresh();
		//	}
		//}
	
		//if (bTestTrigger)
		//{
		//	bTestTrigger = false;
		//	DoDebugVehicleSwitcher();
		//}
	}

	static void GetCarList()
	{
		// obtain car keys from the car stable
		uintptr_t profile = UserProfile::Get(0);

		if (!profile)
			return;

		uintptr_t mCarStable = UserProfile::GetCarStable(profile);

		size_t maxCarRecords = FEPlayerCarDB::GetNumCarRecords();

		std::vector<uint32_t> CarKeys;
		std::vector<uintptr_t> CarRecords;

		//sCarLibrary.clear();

		for (int i = 0; i < maxCarRecords; i++)
		{
			uintptr_t carRecord = FEPlayerCarDB::GetCarByIndex(mCarStable, i);

			int handle = *reinterpret_cast<int*>(carRecord);
			uint32_t VehicleKey = *reinterpret_cast<uint32_t*>(carRecord + 8);

			if (handle == -1)
				continue;

			CarKeys.push_back(VehicleKey);
			CarRecords.push_back(carRecord);
		}

		sCarLibraryMap.clear();
		sCarLibraryKeys.clear();
		sCarLibraryHandles.clear();

		for (int i = 0; i < CarKeys.size(); i++)
		{
			uint32_t k = CarKeys[i];
			uintptr_t carRecord = CarRecords[i];

			int handle = *reinterpret_cast<int*>(carRecord);

			Attrib_Instance instance = { 0 };
			reinterpret_cast<void(__thiscall*)(Attrib_Instance*, unsigned int, unsigned int)>(pAttribGenPVehicle)(&instance, k, 0);

			uintptr_t layout = reinterpret_cast<uintptr_t>(instance.mLayoutPtr);
			char* CollectionName = *reinterpret_cast<char**>(layout + 0x24);

			sCarLibraryKeys.push_back(k);
			sCarLibraryMap[k] = CollectionName;
			sCarLibraryHandles[k] = handle;
		}

	}

	namespace FEPostRaceStateManager
	{
		constexpr size_t vtidx_Start = 7;
		constexpr size_t vtidx_ExitPostRace = 71;
		uintptr_t pStart;
		uintptr_t pExitPostRace;

#pragma runtime_checks("", off)

		//static void __stdcall ExitPostRace_Hook(int exitReason)
		//{
		//	uintptr_t that;
		//	_asm mov that, ecx
		//
		//	//SetVehicle(testVehicleName);
		//	//SetRandomVehicle();
		//
		//	return reinterpret_cast<void(__thiscall*)(uintptr_t, int)>(pExitPostRace)(that, exitReason);
		//}

		static void __stdcall Start_Hook()
		{
			uintptr_t that;
			_asm mov that, ecx

			//SetVehicle(testVehicleName);
			//SetRandomVehicle();

			bWasInPostRace = true;

			return reinterpret_cast<void(__thiscall*)(uintptr_t)>(pStart)(that);
		}

#pragma runtime_checks("", restore)

		static void Init()
		{
			uintptr_t loc_59B7C7 = 0x59B7C7;

			uintptr_t* vt = *reinterpret_cast<uintptr_t**>(loc_59B7C7 + 2);

			pStart = vt[vtidx_Start];
			injector::WriteMemory(&vt[vtidx_Start], Start_Hook, true);

			//pExitPostRace = vt[vtidx_ExitPostRace];
			//injector::WriteMemory(&vt[vtidx_ExitPostRace], ExitPostRace_Hook, true);
		}
	}

	namespace MemcardManager
	{
		//constexpr size_t vtidx_SaveDone = 4;
		constexpr size_t vtidx_LoadDone = 7;

		uintptr_t pOnLoadDone;
		//uintptr_t pOnSaveDone;


#pragma runtime_checks("", off)

		static void __stdcall LoadDone_Hook(const char* name)
		{
			uintptr_t that;
			_asm mov that, ecx

			reinterpret_cast<void(__thiscall*)(uintptr_t, const char*)>(pOnLoadDone)(that, name);

			GetCarList();

			//LoadActiveVehicle(name);
		}

		//static void __stdcall SaveDone_Hook(const char* name)
		//{
		//	uintptr_t that;
		//	_asm mov that, ecx
		//
		//	reinterpret_cast<void(__thiscall*)(uintptr_t, const char*)>(pOnSaveDone)(that, name);
		//
		//	//SaveActiveVehicle(name);
		//}

#pragma runtime_checks("", restore)

		static void Init()
		{
			uintptr_t loc_5ABF5C = 0x5ABF5C;

			uintptr_t* vt = *reinterpret_cast<uintptr_t**>(loc_5ABF5C + 2);


			pOnLoadDone = vt[vtidx_LoadDone];
			injector::WriteMemory(&vt[vtidx_LoadDone], LoadDone_Hook, true);

			//pOnSaveDone = vt[vtidx_SaveDone];
			//injector::WriteMemory(&vt[vtidx_SaveDone], SaveDone_Hook, true);
		}
	}

	namespace FECarClassSelectStateManager
	{
		constexpr size_t vtidx_HandlePadAccept = 30;
		constexpr size_t vtidx_HandlePadStart = 58;
		//uintptr_t pHandlePadAccept;
		uintptr_t pHandlePadStart;
		uintptr_t pShowDialog;

#pragma runtime_checks("", off)

		static void __stdcall HandlePadAccept_Hook()
		{
			uintptr_t that;
			_asm mov that, ecx

			// redirect HandlePadAccept to HandlePadStart to disable the test drive (via buttons at least, probably doesn't disable the mouse button interaction)
			return reinterpret_cast<void(__thiscall*)(uintptr_t)>(pHandlePadStart)(that);
		}

		static void __stdcall ShowDialog(int nextState)
		{
			uintptr_t that;
			_asm mov that, ecx

			// redirect test drive to car selection (via buttons at least, probably doesn't disable the mouse button interaction)
			return reinterpret_cast<void(__thiscall*)(uintptr_t, int)>(pShowDialog)(that, 4);
		}

#pragma runtime_checks("", restore)

		static void Init()
		{
			uintptr_t loc_847644 = 0x847644;
			uintptr_t loc_847742 = 0x847742;

			uintptr_t* vt = *reinterpret_cast<uintptr_t**>(loc_847644 + 2);

			pHandlePadStart = vt[vtidx_HandlePadStart];

			//pHandlePadAccept = vt[vtidx_HandlePadAccept];
			injector::WriteMemory(&vt[vtidx_HandlePadAccept], HandlePadAccept_Hook, true);

			pShowDialog = static_cast<uintptr_t>(injector::GetBranchDestination(loc_847742));
			injector::MakeCALL(loc_847742, ShowDialog);
		}
	}

	static void Game_RaceDone_Hook()
	{
		//SetVehicle(testVehicleName);
		if (bWasInPostRace && !bWasBossCanyonRace)
			SetRandomVehicle();

		if (pGame_RaceDone)
			return reinterpret_cast<void(*)()>(pGame_RaceDone)();
	}

	static void Game_DoPostBossFlow_Hook(uintptr_t instance)
	{
		if (pGame_DoPostBossFlow)
			reinterpret_cast<void(*)(uintptr_t)>(pGame_DoPostBossFlow)(instance);

		bInPostBossFlow = true;

		// the vehicle switch must be delayed for bosses because you don't get the post boss flow otherwise...
		if (bWasInPostRace && bWasBossCanyonRace)
			SetRandomVehicle();
	}

	static uint32_t GetTicker_Hook()
	{
		uint32_t retVal = reinterpret_cast<uint32_t(*)()>(pGetTicker)();
	
		HandleMainLoop();
	
		return retVal;
	}

	static bool __stdcall ValidateRacerInfo(uintptr_t GRacerInfo)
	{
		// actually idk what this value is -- index? position? I only have ProStreet GRacerInfo to go off of...
		uint32_t index = *reinterpret_cast<uint32_t*>(GRacerInfo);
		if (!index)
			return false;

		// usually a pretty good indicator - if the name is empty, something went really, really bad
		char* name = reinterpret_cast<char*>(GRacerInfo + 8);
		if (name[0] == '\0')
			return false;

		// #TODO add more here...

		return true;
	}

	static bool __stdcall IsCanyonBossDuel(uintptr_t GRaceParameters)
	{
		constexpr int raceTypeCanyon = 0xD;

		int raceType = reinterpret_cast<int(__thiscall*)(uintptr_t)>(pGRaceParameters_GetRaceType)(GRaceParameters);
		if (raceType != raceTypeCanyon)
			return false;

		return reinterpret_cast<bool(__thiscall*)(uintptr_t)>(pGRaceParameters_GetIsBossRace)(GRaceParameters);
	}

	static void __declspec(naked) hkGetWinningPlayerInfo()
	{
		_asm
		{
			pushad
			push esi
			call ValidateRacerInfo
			test al, al
			jnz epNormalExitGWPI
			popad
			mov cl, 1
			ret

		epNormalExitGWPI:
			popad
			mov cl, [esi+0x1C8]
			ret
		}
	}

#pragma runtime_checks("", off)

	static int __stdcall GRaceStatus_GetRacerCount_Hook()
	{
		uintptr_t that;
		_asm mov that, ecx

		constexpr size_t offRacerCount = 0x6A08;
		//constexpr size_t offRaceParms = 0x6A1C;
		constexpr size_t offRacerInfos = 0x18;

		constexpr size_t sizeGRacerInfo = 0x384;
		
		struct GRacerInfo
		{
			uint8_t data[sizeGRacerInfo];
		};

		int mRacerCount = *reinterpret_cast<int*>(that + offRacerCount);
		GRacerInfo* mRacerInfo = reinterpret_cast<GRacerInfo*>(that + offRacerInfos);
		
		// racer validation must be put in place in order to avoid crashes
		// the game creates a new player when the car switches, this ensures that it never happens
		// sadly, the tutorial is still broken...

		std::vector<GRacerInfo> validRacers;
		for (int i = 0; i < mRacerCount; i++)
		{
			if (ValidateRacerInfo(reinterpret_cast<uintptr_t>(&mRacerInfo[i])))
			{
				validRacers.push_back(mRacerInfo[i]);
			}
		}

		for (int i = 0; i < validRacers.size(); i++)
		{
			memset(&mRacerInfo[i], 0, sizeof(GRacerInfo));
			memcpy(&mRacerInfo[i], &validRacers[i], sizeof(GRacerInfo));
		}

		*reinterpret_cast<int*>(that + offRacerCount) = validRacers.size();

		// set the index to the first one if this is the ONLY car...
		//if (validRacers.size() == 1)
		//{
		//	*reinterpret_cast<uint32_t*>(&mRacerInfo[0]) = 1;
		//}

		return reinterpret_cast<int(__thiscall*)(uintptr_t)>(pGRaceStatus_GetRacerCount)(that);
	}

	static const char* __stdcall GRaceParameters_GetEventID_Hook()
	{
		uintptr_t that;
		_asm mov that, ecx

		// check if this was a boss canyon duel
		bWasBossCanyonRace = IsCanyonBossDuel(that);

		return reinterpret_cast<const char*(__thiscall*)(uintptr_t)>(pGRaceParameters_GetEventID)(that);
	}

	static void __stdcall GameFlowManager_LoadFrontend_Hook()
	{
		uintptr_t that;
		_asm mov that, ecx

		bWasInPostRace = false;

		return reinterpret_cast<void(__thiscall*)(uintptr_t)>(pGameFlowManager_LoadFrontend)(that);
	}

#pragma runtime_checks("", restore)

	void Init()
	{
		uintptr_t loc_6B7957 = 0x6B7957;
		uintptr_t loc_423031 = 0x423031;
		uintptr_t loc_555A0A = 0x555A0A;
		uintptr_t loc_5CD04D = 0x5CD04D;
		uintptr_t loc_66A55E = 0x66A55E;
		uintptr_t loc_66A9E8 = 0x66A9E8;
		//uintptr_t loc_7166AA = 0x7166AA;
		uintptr_t loc_4D98D0 = 0x4D98D0;
		uintptr_t loc_641168 = 0x641168;
		uintptr_t loc_668F1E = 0x668F1E;
		uintptr_t loc_65610B = 0x65610B;
		uintptr_t loc_4CB390 = 0x4CB390;
		uintptr_t loc_64145F = 0x64145F;
		uintptr_t loc_6DBC83 = 0x6DBC83;


		pGetTicker = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6B7957));
		injector::MakeCALL(loc_6B7957, GetTicker_Hook);


		pAttribGenPVehicle = static_cast<uintptr_t>(injector::GetBranchDestination(loc_423031));
		p_bRandom = static_cast<uintptr_t>(injector::GetBranchDestination(loc_555A0A));

		pGameFlowManagerState = *reinterpret_cast<uintptr_t*>(loc_5CD04D + 2);

		//pEventSysAllocate = static_cast<uintptr_t>(injector::GetBranchDestination(loc_66997A));
		//pECommitAudioAssets = static_cast<uintptr_t>(injector::GetBranchDestination(loc_669994));

		pGame_RaceDone = *reinterpret_cast<uintptr_t*>(loc_66A55E + 1);
		injector::WriteMemory(loc_66A55E + 1, &Game_RaceDone_Hook, true);

		pGame_DoPostBossFlow = *reinterpret_cast<uintptr_t*>(loc_66A9E8 + 1);
		injector::WriteMemory(loc_66A9E8 + 1, &Game_DoPostBossFlow_Hook, true);

		injector::MakeNOP(loc_641168 + 5);
		injector::MakeCALL(loc_641168, hkGetWinningPlayerInfo);

		pGRaceStatus_GetRacerCount = static_cast<uintptr_t>(injector::GetBranchDestination(loc_668F1E));
		injector::MakeCALL(loc_668F1E, GRaceStatus_GetRacerCount_Hook);

		pGRaceParameters_GetEventID = static_cast<uintptr_t>(injector::GetBranchDestination(loc_65610B));
		injector::MakeCALL(loc_65610B, GRaceParameters_GetEventID_Hook);

		pGameFlowManager_LoadFrontend = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6DBC83));
		injector::MakeCALL(loc_6DBC83, GameFlowManager_LoadFrontend_Hook);

		pGRaceParameters_GetRaceType = static_cast<uintptr_t>(injector::GetBranchDestination(loc_4CB390));
		pGRaceParameters_GetIsBossRace = static_cast<uintptr_t>(injector::GetBranchDestination(loc_64145F));

		//uintptr_t ppGetFolder = *reinterpret_cast<uintptr_t*>(loc_7166AA + 2);
		//p_SHGetFolderPathA = *reinterpret_cast<uintptr_t*>(ppGetFolder);

		pDALCareer_SetCar = static_cast<uintptr_t>(injector::GetBranchDestination(loc_4D98D0));
		
		FEPostRaceStateManager::Init();
		FECarClassSelectStateManager::Init();
		MemcardManager::Init();

		

		GetCarList();

		//strcpy_s(testVehicleName, "viper");

		//printf("bTestTrigger = 0x%X\n", &bTestTrigger);
		//printf("testVehicleName = 0x%X\n", testVehicleName);
	}
}
