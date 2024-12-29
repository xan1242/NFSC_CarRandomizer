#include "framework.h"
#include "EAXSound.hpp"
#include "../includes/injector/injector.hpp"
#include <iostream>

namespace EAXSound
{
	uintptr_t ppEAXSound;

	//uintptr_t pRefreshNewGamePlay;
	//uintptr_t pAttachPlayerCars;
	//uintptr_t pStartNewGamePlay;
	//uintptr_t pLoadInGameSoundBanks;
	//uintptr_t pCommitAssets;
	uintptr_t pReStartRace;
	//uintptr_t pInitializeInGame;
	//uintptr_t pMixMapReadyCallback;
	//uintptr_t pLoadCommonIngameFiles;

	//uintptr_t pUnloadAemsInGame;

	//uintptr_t p_gb_DORESTART_RACE;

	static uintptr_t Get()
	{
		if (!ppEAXSound)
			return 0;

		return *reinterpret_cast<uintptr_t*>(ppEAXSound);
	}

	//static void RefreshNewGameplay(uintptr_t instance)
	//{
	//	if (!pRefreshNewGamePlay)
	//		return;
	//
	//	reinterpret_cast<void(__thiscall*)(uintptr_t)>(pRefreshNewGamePlay)(instance);
	//}
	//
	//static void AttachPlayerCars(uintptr_t instance)
	//{
	//	if (!pAttachPlayerCars)
	//		return;
	//
	//	reinterpret_cast<void(__thiscall*)(uintptr_t)>(pAttachPlayerCars)(instance);
	//}
	//
	//static void StartNewGamePlay(uintptr_t instance)
	//{
	//	if (!pStartNewGamePlay)
	//		return;
	//
	//	reinterpret_cast<void(__thiscall*)(uintptr_t)>(pStartNewGamePlay)(instance);
	//}
	//
	//static void CommitAssets(uintptr_t instance)
	//{
	//	if (!pCommitAssets)
	//		return;
	//
	//	reinterpret_cast<void(__thiscall*)(uintptr_t)>(pCommitAssets)(instance);
	//}

	static void ReStartRace(uintptr_t instance, bool bIs321)
	{
		if (!pReStartRace)
			return;

		reinterpret_cast<void(__thiscall*)(uintptr_t, bool)>(pReStartRace)(instance, bIs321);
	}

	//static void InitializeInGame(uintptr_t instance)
	//{
	//	if (!pInitializeInGame)
	//		return;
	//
	//	reinterpret_cast<void(__thiscall*)(uintptr_t)>(pInitializeInGame)(instance);
	//}
	//
	//static void MixMapReadyCallback(uintptr_t instance)
	//{
	//	if (!pMixMapReadyCallback)
	//		return;
	//
	//	reinterpret_cast<void(__thiscall*)(uintptr_t)>(pMixMapReadyCallback)(instance);
	//}
	//
	//static void LoadInGameSoundBanks(uintptr_t instance, void(__cdecl* callback)(void*), void* callback_param)
	//{
	//	if (!pLoadInGameSoundBanks)
	//		return;
	//
	//	reinterpret_cast<void(__thiscall*)(uintptr_t, void(__cdecl*)(void*), void*)>(pLoadInGameSoundBanks)(instance, callback, callback_param);
	//}
	//
	//static void UnloadAemsInGame()
	//{
	//	if (!pUnloadAemsInGame)
	//		return;
	//
	//	reinterpret_cast<void(*)()>(pUnloadAemsInGame)();
	//}
	//
	//static void TriggerAudioRestartRace()
	//{
	//	if (!p_gb_DORESTART_RACE)
	//		return;
	//
	//	*reinterpret_cast<bool*>(p_gb_DORESTART_RACE) = true;
	//}
	//
	//static void LoadCommonIngameFiles()
	//{
	//	if (!pLoadCommonIngameFiles)
	//		return;
	//
	//	reinterpret_cast<void(*)()>(pLoadCommonIngameFiles)();
	//}

	void Refresh()
	{
		uintptr_t pEAXSound = Get();
		if (pEAXSound)
		{
			//CommitAssets(pEAXSound);
			//LoadCommonIngameFiles();

			//
			//EAXSound_RefreshNewGameplay(pEAXSound);
			//EAXSound_InitializeInGame(pEAXSound);
			//AttachPlayerCars(pEAXSound);
			//EAXSound_MixMapReadyCallback(pEAXSound);

			// #TODO figure out the sweetners and why are they so damn loud with each car switch! they seem to load every time!
			// this is the only way to get the engine sounds working, but, it's highly unstable to do this after race finishes
			//StartNewGamePlay(pEAXSound);

			ReStartRace(pEAXSound, false);

			//EAXSound_ReStartRace(pEAXSound, false);
			//TriggerAudioRestartRace();
			//*reinterpret_cast<bool*>(0xA8BA77) = true;
			//EAXSound_RefreshNewGameplay(pEAXSound);

			//UnloadAemsInGame();
			//EAXSound_LoadInGameSoundBanks(pEAXSound, PostSoundReload, reinterpret_cast<void*>(pEAXSound));
		}
	}

	void Init()
	{
		//uintptr_t loc_522136 = 0x522136;
		uintptr_t loc_65DB0C = 0x65DB0C;
		//uintptr_t loc_65DB13 = 0x65DB13;
		//uintptr_t loc_51B38D = 0x51B38D;
		//uintptr_t loc_6699D2 = 0x6699D2;
		//uintptr_t loc_523549 = 0x523549;
		//uintptr_t loc_6B7C89 = 0x6B7C89;
		//uintptr_t loc_66C755 = 0x66C755;
		//uintptr_t loc_52328F = 0x52328F;
		uintptr_t loc_67DB9A = 0x67DB9A;
		//uintptr_t loc_522157 = 0x522157;
		//uintptr_t loc_522168 = 0x522168;

		//p_gb_DORESTART_RACE = *reinterpret_cast<uintptr_t*>(loc_522136 + 1);
		ppEAXSound = *reinterpret_cast<uintptr_t*>(loc_65DB0C + 2);
		//pRefreshNewGamePlay = static_cast<uintptr_t>(injector::GetBranchDestination(loc_65DB13));
		//pAttachPlayerCars = static_cast<uintptr_t>(injector::GetBranchDestination(loc_51B38D));
		//pStartNewGamePlay = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6699D2));
		//pLoadInGameSoundBanks = static_cast<uintptr_t>(injector::GetBranchDestination(loc_523549));
		//pCommitAssets = static_cast<uintptr_t>(injector::GetBranchDestination(loc_66C755));
		pReStartRace = static_cast<uintptr_t>(injector::GetBranchDestination(loc_67DB9A));
		//pInitializeInGame = static_cast<uintptr_t>(injector::GetBranchDestination(loc_522157));
		//pMixMapReadyCallback = static_cast<uintptr_t>(injector::GetBranchDestination(loc_522168));
		//pUnloadAemsInGame = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6B7C89));
		//pLoadCommonIngameFiles = static_cast<uintptr_t>(injector::GetBranchDestination(loc_52328F));
	}
}
