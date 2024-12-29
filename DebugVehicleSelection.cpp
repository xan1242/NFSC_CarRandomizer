#include "framework.h"
#include "DebugVehicleSelection.hpp"
#include "../includes/injector/injector.hpp"
#include <iostream>

namespace DebugVehicleSelection
{
	uintptr_t ppThis;
	uintptr_t pSwitchPlayerVehicle;

	uintptr_t Get()
	{
		if (!ppThis)
			return 0;

		return *reinterpret_cast<uintptr_t*>(ppThis);
	}

	bool SwitchPlayerVehicle(uintptr_t instance, const char* attribname)
	{
		if (!pSwitchPlayerVehicle)
			return false;

		return reinterpret_cast<bool(__thiscall*)(uintptr_t, const char*)>(pSwitchPlayerVehicle)(instance, attribname);
	}

	void Init()
	{
		uintptr_t loc_7DCD55 = 0x7DCD55;
		uintptr_t loc_7DCD81 = 0x7DCD81;

		ppThis = *reinterpret_cast<uintptr_t*>(loc_7DCD55 + 1);
		pSwitchPlayerVehicle = static_cast<uintptr_t>(injector::GetBranchDestination(loc_7DCD81));
	}
}
