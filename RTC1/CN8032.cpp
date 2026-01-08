//*****************************************************************
//
//	This code is strictly reserved by SYBERA. It´s used only for
//	demonstration purposes. Any modification or integration
//	isn´t allowed without permission by SYBERA.
//
//  Copyright (c) 2021 SYBERA
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

#define SEND_CYCLE_TIME		1000
#define REALTIME_PERIOD		100

//*******************************************************************************
#define SEND_PLL		1	//!!! need to set SEND_MODE_PLL(0)  in registry) !!! 
//#define SEND_CLOCKED	1	//!!! need to set SEND_MODE_USER(1) in registry) !!! 
//#define SEND_FLUSHED	1	//!!! need to set SEND_MODE_USER(1) in registry) !!! 
//*******************************************************************************

//Define payload data for module CT121F
typedef union _CT121F
{
	USHORT Di;

} CT121F, *PCT121F;


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
ULONG			__InData[3] = { 0 };
ULONG			__UpdateCnt = 0;
BOOLEAN			__bInitDone = FALSE;		//Initialization flag
CT121F			__CT121FData[2] = { 0 };


__inline void __DoLogic(
					PVOID pCT121FData0,
					PVOID pCT121FData1)
{
	if ((pCT121FData0) &&
		(pCT121FData1))
	{

	//*****************************************************
	SYSTEM_SEQ_CAPTURE("APP", __LINE__, ((PUSHORT)pCT121FData0)[0], FALSE);
	SYSTEM_SEQ_CAPTURE("APP", __LINE__, ((PUSHORT)pCT121FData1)[0], FALSE);
	//*****************************************************

		ETH_GET_BIGENDIAN_USHORT(pCT121FData0, &__CT121FData[0].Di);
		ETH_GET_BIGENDIAN_USHORT(pCT121FData1, &__CT121FData[1].Di);
	}
}


void static AppTask(PVOID)
{
	PVOID  pCT121FData0  =  NULL;
	PVOID  pCT121FData1  =  NULL;
	//PUCHAR  pCn8032Output = NULL;

	//Check if initialization is done
	if (__bInitDone == FALSE)
		return;

	//**************************************
	//Call PNIO enter function and
	PSTATION_INFO pStation = __fpPnioEnter(NULL, NULL, 0);
	//**************************************

	if (pStation)
	{
		//Check station name
		if (__PnioCheckStationName(pStation, "station1"))
		{
			//Get input  data from Slot1 and Slot2 (2 Byte Input)
			PNIO_GET_INPUT_DATAPTR (pStation, 1, 0, &pCT121FData0);
			PNIO_GET_INPUT_DATAPTR (pStation, 2, 0, &pCT121FData1);
			//PNIO_GET_OUTPUT_DATAPTR(pStation, 2, 0, &pCn8032Output);

			//Do Logic
			__DoLogic(pCT121FData0, pCT121FData1);

			//Increase update counter
			__UpdateCnt++;
		}
	}

	//**************************************
	//Call PNIO exit function
	#ifdef SEND_PLL
	__fpPnioExit(pStation);
	#endif

	#ifdef SEND_CLOCKED
	PNIO_EXIT_CLOCKED(__pUserList, __StationNum, SEND_CYCLE_TIME, REALTIME_PERIOD, __CycleCnt);
	#endif

	#ifdef SEND_FLUSHED
	PNIO_EXIT_FLUSHED(__pUserList, __StationNum, SEND_CYCLE_TIME, REALTIME_PERIOD, __CycleCnt);
	#endif
	//**************************************
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

	//**********************************
	//Attach to sequence memory
	//Reset sequence memory (bRun, bRingMode, szFilterSign, FilterLine)
	SEQ_ATTACH(); //Just comment this line to disable sequencing
	SEQ_RESET(TRUE, TRUE, NULL, 0);
	//**********************************

	printf("\n*** ProfiNET Core Realtime Test ***\n\n");

	//Set ethernet mode
	__PntSetEthernetMode(0, REALTIME_PERIOD, TRUE);

	//Required PNIO parameters
	PNIO_PARAMS PnioParams;
	memset(&PnioParams, 0, sizeof(PNIO_PARAMS));
	PnioParams.EthParams.dev_num = 0;
	PnioParams.EthParams.period = REALTIME_PERIOD;
	PnioParams.EthParams.fpAppTask = AppTask;

	//*********************************************
	//!!! Set correct path to station list file !!!
	sprintf(PnioParams.szStationFile, "StationList (CN8032).par");
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

		//Do some delay
		Sleep(1000);

		//Get version information
		Sha64PnioGetVersion(&PnioParams);
		printf("PNIOCORE-DLL: %.2f\nPNIOCORE-DRV: %.2f\n", PnioParams.core_dll_ver / (double)100,	PnioParams.core_drv_ver / (double)100);
		printf("ETHCORE-DLL : %.2f\nETHCORE-DRV : %.2f\n", PnioParams.EthParams.core_dll_ver / (double)100, PnioParams.EthParams.core_drv_ver / (double)100);
		printf("SHA-LIB     : %.2f\nSHA-DRV     : %.2f\n", PnioParams.EthParams.sha_lib_ver / (double)100, PnioParams.EthParams.sha_drv_ver / (double)100);
		printf("\n");

		//Set station names and IPs from file
		Pnio64SetFromStationFile(PnioParams.szStationFile);

		//Enable stations
		for (ULONG i=0; i<PnioParams.StationNum; i++)
			Pnio64EnableStation(&__pUserList[i]);

		//Wait for key pressed
		printf("Number of Stations: %i\n", PnioParams.StationNum);
		printf("\nPress any key ...\n");
		while (!kbhit())
		{
			//Print input payload information
			printf("InData:[%04x] [%04x], UpdateCnt:%i [%c]\r",	
					__CT121FData[0].Di,
					__CT121FData[1].Di, __UpdateCnt, ActChar[++c%4]);

			//Check/Read diagnosics
			if (__pUserList)
			if (__pUserList[0].State == PNIO_STATE_ERROR)
				__ReadDiagnostics(&__pUserList[0]);

			//Do some delay
			Sleep(100);
		}

		//Disable stations (reverse direction)
		for (int i = PnioParams.StationNum - 1; i >= 0; i--)
			Pnio64DisableStation(&__pUserList[i]);

		//Destroy PNIO realtime core
		Sha64PnioDestroy(&PnioParams);
	}

//******************
SEQ_DETACH(); //Detach from sequence memory (only for debugging)
//******************
}
