//*****************************************************************
//
//	This code is strictly reserved by SYBERA. It´s used only for
//	demonstration purposes. Any modification or integration
//	isn´t allowed without permission by SYBERA.
//
//  Copyright (c) 2021 SYBERA
//
//*****************************************************************
//
// !!! Note: This sample requires an INTEL I210 ethernet Adapter !!!
//
//*****************************************************************

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "c:\sha\sha64exp.h"
#include "c:\sha\sha64debug.h"
#include "c:\eth\Sha64EthCore.h"
#include "c:\pnt\Sha64PnioCore.h"
#include "c:\pnt\Pnio64Control.h"
#include "c:\pnt\Pnio64Macros.h"

#pragma comment( lib, "c:\\sha\\sha64dll.lib" )
#pragma comment( lib, "c:\\eth\\sha64ethcore.lib" )
#pragma comment( lib, "c:\\pnt\\sha64pniocore.lib" )

#pragma warning(disable:4996)
#pragma warning(disable:4748)

#define REALTIME_PERIOD		1	//IRT is clock base sceduled by the I210 adapter (no period is required)


//**************************************
//Init sequencing elements (only for debugging)
SEQ_INIT_ELEMENTS();
//**************************************

PETH_STACK		__pUserStack = NULL;
PETH_STACK		__pSystemStack = NULL;
PSTATION_INFO	__pUserList = 0;			//PSTATION_INFO structure for windows application task
PSTATION_INFO	__pSystemList = 0;			//PSTATION_INFO structure for realtime application task
ULONG			__StationNum = 0;
FP_PNIO_ENTER	__fpPnioEnter = NULL;		//Function pointer to PnioEnter
FP_PNIO_EXIT	__fpPnioExit = NULL;		//Function pointer to PnioExit
ULONG			__CV582M_Position = 0;
ULONG			__CV582M_Velocity = 0;
ULONG			__UpdateCnt = { 0 };
BOOLEAN			__bInitDone = FALSE;		//Initialization flag


void static AppTask(PVOID)
{
	PSTATION_INFO pRxStationList[10] = { 0 };
	ULONG RxStationNum = 0;
	PULONG pCV582M_Position;
	PULONG pCV582M_Velocity;
	//Check if initialization is done
	if (__bInitDone == FALSE)
		return;

//*****************************************************
SYSTEM_SEQ_CAPTURE("APP", __LINE__, 0, TRUE);
//*****************************************************

	//**************************************
	//Call PNIO enter function and
	//get current returned stations within this RTC3 cycle
	PSTATION_INFO pStation = __fpPnioEnter(
										pRxStationList,
										&RxStationNum, 0);

	//*****************************************************
	if (RxStationNum)
	{
		//if (RxStationNum == __StationNum) { ... cycle completed  }
	}
	//*****************************************************

	if (pStation)
	{
		//Check station name
		if (__PnioCheckStationName(pStation, "station1"))
		{
			//Get input  data from Slot1 (4 Byte Input)
			PNIO_GET_INPUT_DATAPTR (pStation, 1, 1, &pCV582M_Position);
			PNIO_GET_INPUT_DATAPTR (pStation, 1, 2, &pCV582M_Velocity);
			if (pCV582M_Position) { __CV582M_Position = *pCV582M_Position; }
			if (pCV582M_Velocity) { __CV582M_Velocity = *pCV582M_Velocity; }

			//*****************************************************
			SYSTEM_SEQ_CAPTURE("APP", __LINE__, __CV582M_Position, FALSE);
			SYSTEM_SEQ_CAPTURE("APP", __LINE__, __CV582M_Velocity, FALSE);
			//*****************************************************

			//Increase update counter
			__UpdateCnt++;
		}
	}

	//**************************************
	//Call PNIO exit function
	__fpPnioExit(NULL);
	//**************************************

//*****************************************************
SYSTEM_SEQ_CAPTURE("APP", __LINE__, 0, TRUE);
//*****************************************************
}


#define INDEX_DIAG_MAIN_QUAL_STAT_ONE_AR		0xF00C	//Diagnosis, Maintenance, Qualified and Status for one AR
__inline VOID __ReadDiagnostics(PSTATION_INFO pStation)
{
	USHORT RecordLen = 0x1000;
	UCHAR Buffer[0x1000] = { 0 };

	//Read diagnostics data for one AR
	Pnio64ServiceRead(
				pStation,		//Station pointer
				0,              //Api ID
				0,				//Slot number
				1,				//SubSlot Number (SubModule + 1)
				INDEX_DIAG_MAIN_QUAL_STAT_ONE_AR,	//Record index
				&RecordLen,							//Record length
				Buffer);							//Record data
}


void main(void)
{
	char ActChar[4] = { '\\', '|', '/', '-' };
	ULONG c = 0;
	ULONG i;

	//**********************************
	//Attach to sequence memory
	//Reset sequence memory (bRun, bRingMode, szFilterSign, FilterLine)
	//SEQ_ATTACH(); //Just comment this line to disable sequencing
	//SEQ_RESET(TRUE, TRUE, NULL, 0);
	//**********************************

	printf("\n*** ProfiNET IRT Core Test ***\n\n");

	//Set ethernet mode
	__IrtSetEthernetMode(0);

	ETH_PARAMS_INFO InfoParams = { 0 };
	Sha64EthGetInfo(&InfoParams);

	//Required PNIO parameters
	PNIO_PARAMS PnioParams;
	memset(&PnioParams, 0, sizeof(PNIO_PARAMS));
	PnioParams.EthParams.dev_num = 0;
	PnioParams.EthParams.period = REALTIME_PERIOD;
	PnioParams.EthParams.fpAppTask = AppTask;

	//*********************************************
	//!!! Set correct path to station list file !!!
	sprintf(PnioParams.szStationFile, "c:\\pnt\\StationList (C582M).par");
	//*********************************************

	//Enable PNIO realtime core
	if (ERROR_SUCCESS == Sha64PnioCreate(&PnioParams))
	{


		//Init global elements
		__pUserStack	= PnioParams.EthParams.pUserStack;
		__pSystemStack	= PnioParams.EthParams.pSystemStack;
		__pUserList		= PnioParams.pUserList;
		__pSystemList	= PnioParams.pSystemList;
		__StationNum	= PnioParams.StationNum;
		__fpPnioEnter	= PnioParams.fpPnioEnter;
		__fpPnioExit	= PnioParams.fpPnioExit;

		//Set initialization flag
		__bInitDone = TRUE;

		//Get version information
		Sha64PnioGetVersion(&PnioParams);
		printf("PNIOCORE-DLL: %.2f\nPNIOCORE-DRV: %.2f\n", PnioParams.core_dll_ver / (double)100,	PnioParams.core_drv_ver / (double)100);
		printf("ETHCORE-DLL : %.2f\nETHCORE-DRV : %.2f\n", PnioParams.EthParams.core_dll_ver / (double)100, PnioParams.EthParams.core_drv_ver / (double)100);
		printf("SHA-LIB     : %.2f\nSHA-DRV     : %.2f\n", PnioParams.EthParams.sha_lib_ver / (double)100, PnioParams.EthParams.sha_drv_ver / (double)100);
		printf("\n");

		//Set station names and IPs from file
		Pnio64SetFromStationFile(PnioParams.szStationFile);

		//Enable stations
		for (i=0; i<PnioParams.StationNum; i++)
			Pnio64EnableStation(&__pUserList[i]);

		//Wait for key pressed
		printf("Number of Stations: %i\n", PnioParams.StationNum);
		printf("\nPress any key ...\n");
		while (!kbhit())
		{
			//Print input payload information
			printf("CV582M_Pos:[%08x], CV582M_Vel:[%08x], UpdateCnt: [%i] [%c]\r",
					__CV582M_Position, __CV582M_Velocity,
					__UpdateCnt, ActChar[++c%4]);

			//Check/Read diagnosics
			if (__pUserList[0].State == PNIO_STATE_ERROR)
				__ReadDiagnostics(&__pUserList[0]);

			//Do some delay
			Sleep(100);
		}

		//Disable stations
		for (i=0; i<PnioParams.StationNum; i++)
			Pnio64DisableStation(&__pUserList[i]);

		//Destroy PNIO realtime core
		Sha64PnioDestroy(&PnioParams);
	}

//******************
SEQ_DETACH(); //Detach from sequence memory (only for debugging)
//******************
}
