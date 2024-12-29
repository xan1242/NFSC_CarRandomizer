#pragma once

#include "framework.h"

#ifndef CARRANDOMIZER_HPP
#define CARRANDOMIZER_HPP

namespace CarRandomizer
{
	struct Attrib_Collection
	{
		char mTable[12];
		uintptr_t* mParent;
		unsigned int mKey;
		uintptr_t* mClass;
		void* mLayout;
		uintptr_t* mSource;
	};

	struct Attrib_Instance
	{
		Attrib_Collection* mCollection;
		void* mLayoutPtr;
		unsigned int mMsgPort;
		unsigned int mFlags;
	};

	void Init();
}

#endif