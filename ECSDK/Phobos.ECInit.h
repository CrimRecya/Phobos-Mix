#pragma once

#include "EC.h"
#include <Utilities/Debug.h>
#include <Phobos.version.h>
#include <Phobos.h>

#define EC_LIBRARY_VERSION (BUILD_NUMBER * 1000 + MERGE_PATCH)
#define EC_LIBRARY_LOWEST_VERSION (BUILD_NUMBER * 1000)
#define WIC_LOWEST_VERSION 10
#define SYRINGEIH_LOWEST_VERSION 20300 //0.2.3

inline bool ECIsComplete()
{
	return
		HasWIC() &&
		HasIH() &&
		HasSyringeIH() &&
		WICVer() >= WIC_LOWEST_VERSION &&
		SyringeIHVer() >= SYRINGEIH_LOWEST_VERSION;
}

inline void ECInitialize()
{
	ECInitLibrary(
		"Phobos",                     //Library Name
		EC_LIBRARY_VERSION,           //Library Version
		EC_LIBRARY_LOWEST_VERSION,    //Lowest Supported Version
		u8"" PRODUCT_VERSION,         //Description
		[] { },                       //Initialize Function
		[]                            //After Initialize Function
		{
			Phobos::PoweredByEC = ECIsComplete();
			if (Phobos::PoweredByEC)
			{
				Debug::Log("Full EC Detected!\n");
			}
			else if (HasWIC() && HasSyringeIH() && (WICVer() < WIC_LOWEST_VERSION || SyringeIHVer() < SYRINGEIH_LOWEST_VERSION))
			{
				Debug::Log("EC Detected - Component Version Too Low\n");
			}
			else
			{
				Debug::Log("EC Detected - Missing Components\n");
			}
		}
	);
}


