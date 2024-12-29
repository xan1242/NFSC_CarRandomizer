#pragma once

#include "framework.h"

#ifndef USERPROFILE_HPP
#define USERPROFILE_HPP

namespace UserProfile
{
	uintptr_t Get(const int player);
	uintptr_t GetCarStable(uintptr_t profile);
	void Init();
}

#endif