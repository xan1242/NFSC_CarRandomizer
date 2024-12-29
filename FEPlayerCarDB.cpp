#include "framework.h"
#include "FEPlayerCarDB.hpp"
#include "../includes/injector/injector.hpp"
#include <iostream>

namespace FEPlayerCarDB
{
	constexpr size_t NumCarRecords = 200;
	uintptr_t pGetCarByIndex;

	size_t GetNumCarRecords()
	{
		return NumCarRecords;
	}

	uintptr_t GetCarByIndex(uintptr_t instance, int index)
	{
		if (!pGetCarByIndex)
			return 0;
		return reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, int)>(pGetCarByIndex)(instance, index);
	}

	void Init()
	{
		uintptr_t loc_86B113 = 0x86B113;

		pGetCarByIndex = static_cast<uintptr_t>(injector::GetBranchDestination(loc_86B113));
	}
}
