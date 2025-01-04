#include "framework.h"
#include "UserProfile.hpp"
#include "injector/injector.hpp"
#include <iostream>

namespace UserProfile
{
	uintptr_t pFEManager;
	uintptr_t pGetUserProfile;

	uintptr_t Get(const int player)
	{
		if (!pFEManager)
			return 0;

		uintptr_t FEManager = *reinterpret_cast<uintptr_t*>(pFEManager);
		if (!FEManager)
			return 0;

		return reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, const int)>(pGetUserProfile)(FEManager, player);
	}

	uintptr_t GetCarStable(uintptr_t profile)
	{
		return profile + 0x234;
	}

	void Init()
	{
		uintptr_t loc_572B3D = 0x572B3D;
		uintptr_t loc_86B0FF = 0x86B0FF;

		pFEManager = *reinterpret_cast<uintptr_t*>(loc_572B3D + 2);
		pGetUserProfile = static_cast<uintptr_t>(injector::GetBranchDestination(loc_86B0FF));
	}
}
