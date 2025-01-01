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
#include <chrono>
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
	//uintptr_t pAttribGenPresetRide;
	uintptr_t p_bRandom;
	//uintptr_t p_SHGetFolderPathA;
	uintptr_t pDALCareer_SetCar;
	uintptr_t pGRaceStatus_GetRacerCount;
	uintptr_t pGRaceParameters_GetEventID;
	uintptr_t pGRaceParameters_GetRaceType;
	uintptr_t pGRaceParameters_GetIsBossRace;
	uintptr_t pGRaceParameters_GetIsChallengeSeriesRace;
	uintptr_t pGRaceParameters_GetPlayerCarType;
	uintptr_t pGRaceParameters_GetUsePresetRide;
	uintptr_t pGRaceParameters_GetPlayerCarTypeHash;

	std::chrono::steady_clock::time_point nextSpeechExecution;
	uintptr_t pSpeech_Manager_Update;

	uintptr_t pGameFlowManagerState;

	//std::unordered_map<uint32_t, std::string> sCarLibraryMap;
	std::unordered_map<uint32_t, uint32_t> sCarLibraryHandles;
	//std::vector<uintptr_t> sCarRecords;
	std::vector<uint32_t> sCarLibraryKeys;

	const uint32_t sTrafficCarLibraryKeys[] =
	{
		STRINGHASH32("cs_semi"), // this one is safe to use...

		STRINGHASH32("traf4dseda"),
		STRINGHASH32("traf4dsedb"),
		STRINGHASH32("traf4dsedc"),
		STRINGHASH32("trafcourt"),
		STRINGHASH32("trafficcoup"),
		STRINGHASH32("trafha"),
		STRINGHASH32("trafstwag"),
		STRINGHASH32("traftaxi"),

		STRINGHASH32("trafamb"),
		STRINGHASH32("trafcemtr"),
		STRINGHASH32("trafdmptr"),
		STRINGHASH32("traffire"),
		STRINGHASH32("trafgarb"),

		STRINGHASH32("trafcamper"),
		STRINGHASH32("trafminivan"),
		STRINGHASH32("trafnews"),
		STRINGHASH32("trafpickupa"),
		STRINGHASH32("trafsuva"),
		STRINGHASH32("trafvanb"),
	};

	const uint32_t sTrafficSemiLibraryKeys[] =
	{
		STRINGHASH32("semia"),
		STRINGHASH32("semib"),
		STRINGHASH32("semibox"),
		STRINGHASH32("semicmt"),
		STRINGHASH32("semicon"),
		STRINGHASH32("semicrate"),
		STRINGHASH32("semilog"),
	};

	const uint32_t sCopCarLibraryKeys[] =
	{
		STRINGHASH32("copgto"),
		STRINGHASH32("copgtoghost"),
		STRINGHASH32("copmidsize"),
		STRINGHASH32("copghost"),
		STRINGHASH32("copsport"),
		STRINGHASH32("copcross"),
		STRINGHASH32("copsportghost"),
		STRINGHASH32("copsporthench"),
		STRINGHASH32("copsuv"),
		STRINGHASH32("copsuvpatrol"),
		STRINGHASH32("copsuvl"),
	};

	uint32_t CurrentVehicle;

	uint32_t CurrentVehicleCS;
	//bool bCSVehicleIsPreset;
	//char sCurrentVehicleTypeCS[128];

	bool bWasInPostRace;
	bool bWasBossCanyonRace;
	bool bChallengeRace;
	bool bInPostBossFlow;

	bool bQueueEAXRefreshFromNIS;

	bool bExcludeTrafficCars;
	bool bIncludeTrafficSemis;
	bool bExcludeCopCars;
	bool bExcludeRegularCars;

	void SetExcludeTrafficCars(bool state)
	{
		bExcludeTrafficCars = state;
	}

	bool GetExcludeTrafficCars()
	{
		return bExcludeTrafficCars;
	}

	void SetIncludeTrafficSemis(bool state)
	{
		bIncludeTrafficSemis = state;
	}

	bool GetIncludeTrafficSemis()
	{
		return bIncludeTrafficSemis;
	}

	void SetExcludeCopCars(bool state)
	{
		bExcludeCopCars = state;
	}

	bool GetExcludeCopCars()
	{
		return bExcludeCopCars;
	}

	void SetExcludeRegularCars(bool state)
	{
		bExcludeRegularCars = state;
	}

	bool GetExcludeRegularCars()
	{
		return bExcludeRegularCars;
	}

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

	static const char* GetPVehicleNameByKey(uint32_t k)
	{
		Attrib_Instance instance = { 0 };
		reinterpret_cast<void(__thiscall*)(Attrib_Instance*, unsigned int, unsigned int)>(pAttribGenPVehicle)(&instance, k, 0);

		uintptr_t layout = reinterpret_cast<uintptr_t>(instance.mLayoutPtr);
		return *reinterpret_cast<char**>(layout + 0x24);
	}

	//static const char* GetPresetRideNameByKey(uint32_t k)
	//{
	//	Attrib_Instance instance = { 0 };
	//	reinterpret_cast<void(__thiscall*)(Attrib_Instance*, unsigned int, unsigned int)>(pAttribGenPresetRide)(&instance, k, 0);
	//
	//	uintptr_t layout = reinterpret_cast<uintptr_t>(instance.mLayoutPtr);
	//	return *reinterpret_cast<char**>(layout + 0xC);
	//}

	//static uint32_t TryGetPresetKey(uintptr_t carRecord)
	//{
	//	if (!*reinterpret_cast<bool*>(carRecord + 0x16))
	//		return 0;
	//
	//	return *reinterpret_cast<uint32_t*>(carRecord + 0xC);
	//}
	//
	//static uint32_t TryGetVehicleKey(uintptr_t carRecord)
	//{
	//	return *reinterpret_cast<uint32_t*>(carRecord + 0x8);
	//}

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

	static int InjectDebugVehicle(uint32_t key)
	{
		uintptr_t profile = UserProfile::Get(0);

		if (!profile)
			return 0x12345678;

		uintptr_t mCarStable = UserProfile::GetCarStable(profile);
		uintptr_t carRecord = FEPlayerCarDB::GetCarByIndex(mCarStable, 0);

		int handle = *reinterpret_cast<int*>(carRecord);

		*reinterpret_cast<uint32_t*>(carRecord + 4) = STRINGHASH32("uncustomizable");
		*reinterpret_cast<uint32_t*>(carRecord + 8) = key;
		*reinterpret_cast<uint8_t*>(carRecord + 0x10) = 0xFF;

		return handle;
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

			// delay speech updates by 15 seconds because it may make the game unstable...
			nextSpeechExecution = std::chrono::steady_clock::now() + std::chrono::seconds(15);
		}
	}

	static void SetVehicle(uint32_t key)
	{
		SetVehicle(GetPVehicleNameByKey(key));

		if (sCarLibraryHandles.find(key) != sCarLibraryHandles.end())
			DALCareer_SetCar(sCarLibraryHandles[key]);
		else
		{
			DALCareer_SetCar(InjectDebugVehicle(key));
		}
	}

#ifdef _DEBUG
	//bool bDoSecondCar;

	const uint32_t DebugCars[] =
	{
		//STRINGHASH32("player_cop_gto"),
		//STRINGHASH32("darius"),
		//STRINGHASH32("zonda"),
		//STRINGHASH32("murcielago640"),
		//STRINGHASH32("player_cop_corvette"),
		STRINGHASH32("copcross"),
		STRINGHASH32("copgto"),
	};

	int DebugCarCounter = 0;

#endif

	static void SetRandomVehicle()
	{
		if (bChallengeRace)
		{
			bChallengeRace = false;
			return;
		}

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
	}

	static void GetCarList()
	{
		// obtain car keys from the car stable
		uintptr_t profile = UserProfile::Get(0);

		if (!profile)
			return;

		// failsafe
		if (bExcludeRegularCars && bExcludeTrafficCars && bExcludeCopCars)
			bExcludeRegularCars = false;

		uintptr_t mCarStable = UserProfile::GetCarStable(profile);

		size_t maxCarRecords = FEPlayerCarDB::GetNumCarRecords();

		std::vector<uint32_t> CarKeys;
		std::vector<uintptr_t> CarRecords;

		//sCarRecords.clear();
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

		//sCarLibraryMap.clear();
		sCarLibraryKeys.clear();
		sCarLibraryHandles.clear();

		if (!bExcludeRegularCars)
		{
			for (int i = 0; i < CarKeys.size(); i++)
			{
				uint32_t k = CarKeys[i];
				uintptr_t carRecord = CarRecords[i];

				int handle = *reinterpret_cast<int*>(carRecord);

				sCarLibraryKeys.push_back(k);
				//sCarLibraryMap[k] = GetPVehicleNameByKey(k);
				sCarLibraryHandles[k] = handle;
			}
		}

		if (!bExcludeTrafficCars)
		{
			for (const uint32_t& k : sTrafficCarLibraryKeys)
				sCarLibraryKeys.push_back(k);

			if (bIncludeTrafficSemis)
				for (const uint32_t& k : sTrafficSemiLibraryKeys)
					sCarLibraryKeys.push_back(k);

		}


		if (!bExcludeCopCars)
		{
			for (const uint32_t& k : sCopCarLibraryKeys)
				sCarLibraryKeys.push_back(k);
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

	uintptr_t loc_452C0C;
	uintptr_t loc_452B9B;
	static void __declspec(naked) hkNISListenerActivitySomething()
	{
		_asm
		{
			test eax, eax
			jz epMissingVehicleNLAS
			mov edx, [eax]
			mov ecx, eax
			call dword ptr [edx + 0xC0]
			jmp [loc_452B9B]
		epMissingVehicleNLAS:
			jmp [loc_452C0C]
		}
	}

	static bool ShouldDelaySpeechUpdate()
	{
		auto currentTime = std::chrono::steady_clock::now();

		if (currentTime >= nextSpeechExecution)
			return false;

		return true;
	}

	static void Speech_Manager_Update_Hook(float dT)
	{
		if (ShouldDelaySpeechUpdate())
			return;

		return reinterpret_cast<void(*)(int)>(pSpeech_Manager_Update)(dT);
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
		
		// no need to check any further if it's the only one...
		if (mRacerCount == 1)
			return reinterpret_cast<int(__thiscall*)(uintptr_t)>(pGRaceStatus_GetRacerCount)(that);

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

		bChallengeRace = reinterpret_cast<bool(__thiscall*)(uintptr_t)>(pGRaceParameters_GetIsChallengeSeriesRace)(that);
		bWasInPostRace = false;

		return reinterpret_cast<const char*(__thiscall*)(uintptr_t)>(pGRaceParameters_GetEventID)(that);
	}

	static const char* __stdcall GRaceParameters_GetPlayerCarType_Hook()
	{
		uintptr_t that;
		_asm mov that, ecx

		bChallengeRace = reinterpret_cast<bool(__thiscall*)(uintptr_t)>(pGRaceParameters_GetIsChallengeSeriesRace)(that);

		if (!bChallengeRace)
			return reinterpret_cast<const char* (__thiscall*)(uintptr_t)>(pGRaceParameters_GetPlayerCarType)(that);
		
		//unsigned int index = bRandom(sCarLibraryKeys.size());
		//CurrentVehicleCS = sCarLibraryKeys[index];

		return GetPVehicleNameByKey(CurrentVehicleCS);
	}

	static uint32_t __stdcall GRaceParameters_GetPlayerCarTypeHash_Hook()
	{
		uintptr_t that;
		_asm mov that, ecx

		bChallengeRace = reinterpret_cast<bool(__thiscall*)(uintptr_t)>(pGRaceParameters_GetIsChallengeSeriesRace)(that);

		if (!bChallengeRace)
			return reinterpret_cast<uint32_t(__thiscall*)(uintptr_t)>(pGRaceParameters_GetPlayerCarTypeHash)(that);

		unsigned int index = bRandom(sCarLibraryKeys.size());
		CurrentVehicleCS = sCarLibraryKeys[index];

		return CurrentVehicleCS;
	}

	static bool __stdcall GRaceParameters_GetUsePresetRide_Hook()
	{
		uintptr_t that;
		_asm mov that, ecx
	
		bChallengeRace = reinterpret_cast<bool(__thiscall*)(uintptr_t)>(pGRaceParameters_GetIsChallengeSeriesRace)(that);
	
		if (!bChallengeRace)
			return reinterpret_cast<bool(__thiscall*)(uintptr_t)>(pGRaceParameters_GetUsePresetRide)(that);
	
		return false;
	}

	static void __stdcall GameFlowManager_LoadFrontend_Hook()
	{
		uintptr_t that;
		_asm mov that, ecx

		bWasInPostRace = false;
		bChallengeRace = false;

		return reinterpret_cast<void(__thiscall*)(uintptr_t)>(pGameFlowManager_LoadFrontend)(that);
	}

#pragma runtime_checks("", restore)

	void Init()
	{
		uintptr_t loc_6B7957 = 0x6B7957;
		uintptr_t loc_423031 = 0x423031;
		//uintptr_t loc_4D180F = 0x4D180F;
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
		uintptr_t loc_6459AD = 0x6459AD;
		uintptr_t loc_52228D = 0x52228D;

		uintptr_t loc_765780 = 0x765780;
		uintptr_t loc_7657A1 = 0x7657A1;
		uintptr_t loc_64577D = 0x64577D;

		loc_452C0C = 0x452C0C;
		loc_452B9B = 0x452B9B;
		uintptr_t loc_452B91 = 0x452B91;

		pGetTicker = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6B7957));
		injector::MakeCALL(loc_6B7957, GetTicker_Hook);


		pAttribGenPVehicle = static_cast<uintptr_t>(injector::GetBranchDestination(loc_423031));
		//pAttribGenPresetRide = static_cast<uintptr_t>(injector::GetBranchDestination(loc_4D180F));
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

		injector::MakeJMP(loc_452B91, hkNISListenerActivitySomething);

		pGRaceStatus_GetRacerCount = static_cast<uintptr_t>(injector::GetBranchDestination(loc_668F1E));
		injector::MakeCALL(loc_668F1E, GRaceStatus_GetRacerCount_Hook);

		pGRaceParameters_GetPlayerCarType = static_cast<uintptr_t>(injector::GetBranchDestination(loc_765780));
		injector::MakeCALL(loc_765780, GRaceParameters_GetPlayerCarType_Hook);

		pGRaceParameters_GetPlayerCarTypeHash = static_cast<uintptr_t>(injector::GetBranchDestination(loc_64577D));
		injector::MakeCALL(loc_64577D, GRaceParameters_GetPlayerCarTypeHash_Hook);

		pGRaceParameters_GetUsePresetRide = static_cast<uintptr_t>(injector::GetBranchDestination(loc_7657A1));
		injector::MakeCALL(loc_7657A1, GRaceParameters_GetUsePresetRide_Hook);

		pGRaceParameters_GetEventID = static_cast<uintptr_t>(injector::GetBranchDestination(loc_65610B));
		injector::MakeCALL(loc_65610B, GRaceParameters_GetEventID_Hook);

		pGameFlowManager_LoadFrontend = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6DBC83));
		injector::MakeCALL(loc_6DBC83, GameFlowManager_LoadFrontend_Hook);

		pSpeech_Manager_Update = static_cast<uintptr_t>(injector::GetBranchDestination(loc_52228D));
		injector::MakeCALL(loc_52228D, Speech_Manager_Update_Hook);

		pGRaceParameters_GetRaceType = static_cast<uintptr_t>(injector::GetBranchDestination(loc_4CB390));
		pGRaceParameters_GetIsBossRace = static_cast<uintptr_t>(injector::GetBranchDestination(loc_64145F));
		pGRaceParameters_GetIsChallengeSeriesRace = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6459AD));

		//uintptr_t ppGetFolder = *reinterpret_cast<uintptr_t*>(loc_7166AA + 2);
		//p_SHGetFolderPathA = *reinterpret_cast<uintptr_t*>(ppGetFolder);

		pDALCareer_SetCar = static_cast<uintptr_t>(injector::GetBranchDestination(loc_4D98D0));
		
		FEPostRaceStateManager::Init();
		FECarClassSelectStateManager::Init();
		MemcardManager::Init();

		

		GetCarList();

		nextSpeechExecution = std::chrono::steady_clock::now();

		//strcpy_s(testVehicleName, "viper");

		//printf("bTestTrigger = 0x%X\n", &bTestTrigger);
		//printf("testVehicleName = 0x%X\n", testVehicleName);
	}
}
