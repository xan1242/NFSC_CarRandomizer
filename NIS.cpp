#include "framework.h"
#include "NIS.hpp"

namespace NIS
{
	uintptr_t pNISInstance;

	bool bIsInNIS()
	{
		if (pNISInstance)
			return *reinterpret_cast<uintptr_t*>(pNISInstance) != 0;

		return false;
	}

	void Init()
	{
		uintptr_t loc_519391 = 0x519391;
		pNISInstance = *reinterpret_cast<uintptr_t*>(loc_519391 + 2);
	}
}
