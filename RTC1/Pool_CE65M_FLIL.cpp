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
#include "c:\eth\Eth64Macros.h"
#include "c:\pnt\Sha64PnioCore.h"
#include "c:\pnt\Pnio64Control.h"
#include "c:\pnt\Pnio64Macros.h"

#pragma comment( lib, "c:\\sha\\sha64dll.lib" )
#pragma comment( lib, "c:\\eth\\sha64ethcore.lib" )
#pragma comment( lib, "c:\\pnt\\sha64pniocore.lib" )

#pragma warning(disable:4996)
#pragma warning(disable:4748)

#define SEND_CYCLE_TIME		4000
#define REALTIME_PERIOD		200

//*******************************************************************************
//#define SEND_PLL		1	//!!! need to set SEND_MODE_PLL(0)  in registry) !!! 
#define SEND_CLOCKED	1	//!!! need to set SEND_MODE_USER(1) in registry) !!! 
//#define SEND_FLUSHED	1	//!!! need to set SEND_MODE_USER(1) in registry) !!! 
//*******************************************************************************

//**************************************
//Init sequencing elements (only for debugging)
SEQ_INIT_ELEMENTS();
//**************************************

typedef struct _POOL_INPUT
{
	ULONG Position;		//Station0, Slot1
	UCHAR DigIn;		//Station1, Slot3

} POOL_INPUT, *PPOOL_INPUT;


typedef struct _POOL_OUTPUT
{
	UCHAR DigOut;		//Station1, Slot2

} POOL_OUTPUT, *PPOOL_OUTPUT;


PUCHAR			__pUserPool = NULL;
PUCHAR			__pSystemPool = NULL;
PETH_STACK		__pUserStack = NULL;
PETH_STACK		__pSystemStack = NULL;
PSTATION_INFO	__pUserList = 0;			//PSTATION_INFO structure for windows application task
PSTATION_INFO	__pSystemList = 0;			//PSTATION_INFO structure for realtime application task
ULONG			__StationNum = 0;
FP_PNIO_ENTER	__fpPnioEnter = NULL;		//Function pointer to PnioEnter
FP_PNIO_EXIT	__fpPnioExit = NULL;		//Function pointer to PnioExit
ULONG			__UpdateCnt1 = 0;
ULONG			__UpdateCnt2 = 0;
ULONG			__CycleCnt = 0;				//Cycle counter
BOOLEAN			__bInitDone = FALSE;		//Initialization flag


void static AppTask(PVOID)
{
	PPOOL_INPUT  pPoolInput  = (PPOOL_INPUT) &__pSystemPool[0];
	PPOOL_OUTPUT pPoolOutput = (PPOOL_OUTPUT)&__pSystemPool[0x1000];

	//Check if initialization is done
	if (__bInitDone == FALSE)
		return;

//*****************************************************
//SYSTEM_SEQ_CAPTURE("APP", __LINE__, 0, TRUE);
//*****************************************************

	//**************************************
	//Call PNIO enter function
	PSTATION_INFO pStation = __fpPnioEnter(NULL, NULL, 0);
	if (pStation)
	{
	//**************************************

//*****************************************************
//SYSTEM_SEQ_CAPTURE("APP", __LINE__, 0, TRUE);
//*****************************************************

		//Check station name
		if (__PnioCheckStationName(pStation, "station1"))
		{
			PULONG pPosition = NULL;

			//Get input  data from Slot1 (4 Byte Input)
			PNIO_GET_INPUT_DATAPTR (pStation, 1, 0, &pPosition);

			if (pPosition) { ETH_GET_BIGENDIAN_ULONG(pPosition, &pPoolInput->Position); }

			//Increase update counter
			__UpdateCnt1++;
		}

		//Check station name
		if (__PnioCheckStationName(pStation, "station2"))
		{
			PUCHAR pDigIn  = NULL;
			PUCHAR pDigOut = NULL;

			//Set output data to   Slot2 (1 Byte Output)
			//Get input  data from Slot3 (1 Byte Input)
			PNIO_GET_OUTPUT_DATAPTR(pStation, 2, 0, &pDigOut);
			PNIO_GET_INPUT_DATAPTR (pStation, 3, 0, &pDigIn);

			if (pDigOut) { *pDigOut = pPoolOutput->DigOut; }
			if (pDigIn)  { pPoolInput->DigIn = *pDigIn; }

			//Increase update counter
			__UpdateCnt2++;
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

//*****************************************************
//SYSTEM_SEQ_CAPTURE("APP", __LINE__, 0, TRUE);
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
	ULONG PoolSize = 0;
	HANDLE hPool = NULL;
	char ActChar[4] = { '\\', '|', '/', '-' };
	ULONG c = 0;
	ULONG i;

	//**********************************
	//Attach to sequence memory
	//Reset sequence memory (bRun, bRingMode, szFilterSign, FilterLine)
	//SEQ_ATTACH(); //Just comment this line to disable sequencing
	//SEQ_RESET(TRUE, TRUE, NULL, 0);
	//**********************************

	printf("\n*** ProfiNET Core Realtime Test ***\n\n");

	//Try to attach to the first memory
	if (ERROR_SUCCESS == Sha64AttachMemWithTag(
											BOOT_MEM_TAG,			//Memory tag
											(ULONG*)&PoolSize,		//Memory size
											(void**)&__pUserPool,	//Pointer to memory for Windows access
											(void**)&__pSystemPool,	//Pointer to memory for Realtime access
											NULL,					//Physical memory address
											&hPool))				//Handle to memory device
	{
		//Set ethernet mode
		__PntSetEthernetMode(0, REALTIME_PERIOD, FALSE);

		//Required PNIO parameters
		PNIO_PARAMS PnioParams;
		memset(&PnioParams, 0, sizeof(PNIO_PARAMS));
		PnioParams.EthParams.dev_num = 0;
		PnioParams.EthParams.period = REALTIME_PERIOD;
		PnioParams.EthParams.fpAppTask = AppTask;

		//*********************************************
		//!!! Set correct path to station list file !!!
		sprintf(PnioParams.szStationFile, "StationList(Pool_CE65M_FLIL).par");
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
				PPOOL_INPUT  pPoolInput  = (PPOOL_INPUT) &__pUserPool[0];
				PPOOL_OUTPUT pPoolOutput = (PPOOL_OUTPUT)&__pUserPool[0x1000];

				//Print input payload information
				printf("DigIn:[%02x] Position:[%08x], UpdateCnt:%i %i [%c]\r",
						pPoolInput->DigIn, pPoolInput->Position, __UpdateCnt1, __UpdateCnt2, ActChar[++c%4]);

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
		//Detach from pool memory
		Sha64DetachMem(hPool);
	}

//******************
SEQ_DETACH(); //Detach from sequence memory (only for debugging)
//******************
}
