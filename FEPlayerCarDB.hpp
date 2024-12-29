#pragma once

#include "framework.h"

#ifndef FEPLAYERCARDB_HPP
#define FEPLAYERCARDB_HPP

namespace FEPlayerCarDB
{
	uintptr_t GetCarByIndex(uintptr_t instance, int index);
	size_t GetNumCarRecords();
	void Init();
}

#endif