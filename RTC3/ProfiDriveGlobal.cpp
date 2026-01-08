//*****************************************************************
//	This code is strictly reserved by SYBERA GmbH
//  Copyright (c) 2025 SYBERA
//*****************************************************************

#pragma warning(disable:4748)

#include "ProfiDriveGlobal.h"


__inline void __ProfiDrive_UpdateSignOfLive(PPROFIDRIVE_STW2 pStw2)
{
	//Update sign of life
	++pStw2->s.sign_of_live;
	if (pStw2->s.sign_of_live == 0)
		pStw2->s.sign_of_live = 1;

	//Do cache operation
	_mm_mfence();
}
