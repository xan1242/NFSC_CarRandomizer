#pragma once

#include "framework.h"

#ifndef DEBUGVEHICLESELECTION_HPP
#define DEBUGVEHICLESELECTION_HPP

namespace DebugVehicleSelection
{
	uintptr_t Get();
	bool SwitchPlayerVehicle(uintptr_t instance, const char* attribname);
	void Init();
}

#endif