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

#pragma comment( lib, "c:\\sha\\sha64dll.lib" )
#pragma comment( lib, "c:\\eth\\sha64ethcore.lib" )
#pragma comment( lib, "c:\\pnt\\sha64pniocore.lib" )

#pragma warning (disable:4996)
#pragma warning(disable:4748)

#define SEND_CYCLE_TIME		4000
#define REALTIME_PERIOD		 200

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
ULONG			__UpdateCnt = 0;
ULONG			__CycleCnt = 0;
BOOLEAN			__bInitDone = FALSE;		//Initialization flag
ULONG			__MasterInData[100]  = { 0 };


void static AppTask(PVOID)
{
	PULONG pMasterInData[100]  = { NULL };

	//Check if initialization is done
	if (__bInitDone == FALSE)
		return;

	//Check for buffer overflow
	if (__pSystemStack->hdr.err_flag == TRUE)
		__pSystemStack->hdr.err_flag = FALSE;

	//**************************************
	//Call PNIO enter function
	PSTATION_INFO pStation = __fpPnioEnter(NULL, NULL, 0);
	if (pStation)
	{
	//**************************************

		int Index = pStation->Hdr.Index;

		//Get input  data from Slot1 (4 Byte Input)
		PNIO_GET_INPUT_DATAPTR (pStation, 1, 1, &pMasterInData[Index]);

		//Set master input data
		if (pMasterInData[Index]) { __MasterInData[Index] = *pMasterInData[Index]; }

		//*****************************************************
		//SYSTEM_SEQ_CAPTURE("APP", __LINE__, __Position, FALSE);
		//*****************************************************

		//Increase update counter
		__UpdateCnt++;
	}

	//**************************************
	//Call PNIO exit function (need to set SEND_MODE_USER(1) in registry) 
	PNIO_EXIT_FLUSHED(__pUserList, __StationNum, SEND_CYCLE_TIME, REALTIME_PERIOD, __CycleCnt);
	//**************************************

	//*****************************************************
	SYSTEM_SEQ_CAPTURE("APP", __LINE__, __CycleCnt, TRUE);
	//*****************************************************
}


void main(void)
{
	char ActChar[4] = { '\\', '|', '/', '-' };
	ULONG c = 0;
	ULONG i;

	//**********************************
	//Attach to sequence memory
	//Reset sequence memory (bRun, bRingMode, szFilterSign, FilterLine)
	SEQ_ATTACH(); //Just comment this line to disable sequencing
	SEQ_RESET(TRUE, TRUE, "APP", 0);
	//**********************************

	printf("\n*** ProfiNET Core Realtime Level2 Test ***\n\n");

	//Required PNIO parameters
	PNIO_PARAMS PnioParams = { 0 };
	PnioParams.EthParams.dev_num = 0;
	PnioParams.EthParams.period = REALTIME_PERIOD;
	PnioParams.EthParams.fpAppTask = AppTask;

	//*****************************
	//!!! Set correct path to station list file !!!
	sprintf(PnioParams.szStationFile, "StationList (SendModeFlushed).par");		//Use SEND_PLL     with SendMode = 0 (registry)
	//*****************************

	//Set ethernet mode
	__PntSetEthernetMode(
					PnioParams.EthParams.dev_num,
					REALTIME_PERIOD,
					TRUE);

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

		printf("\nNumber of Stations: %i\n", PnioParams.StationNum);

		//Enable all stations
		for (i = 0; i<PnioParams.StationNum; i++)
		{
			//Enable station
			Pnio64EnableStation(&__pUserList[i]);

			//List stations
			printf("Station %i: %s [%i.%i.%i.%i] Frame-ID: [0x%04x]\n", i,
				   __pUserList[i].Hdr.szName,
				   __pUserList[i].Hdr.IpParams.ip_addr.bit8[0],
				   __pUserList[i].Hdr.IpParams.ip_addr.bit8[1],
				   __pUserList[i].Hdr.IpParams.ip_addr.bit8[2],
				   __pUserList[i].Hdr.IpParams.ip_addr.bit8[3],
				   __pUserList[i].InputFrameID);
		}


		//Wait for key pressed
		printf("\n\nPress any key ...\n");
		while (!kbhit())
		{
			//Print input payload information
			printf("Cycle:%03i Upd:%i MIn:", __CycleCnt, __UpdateCnt);
			for (ULONG i=0; i<__StationNum; i++)
				printf("[%02x]", (UCHAR)__MasterInData[i]);
			printf(" [%c]\r", ActChar[++c%4]);

			//Do some delay
			Sleep(100);
		}

		//Disable stations
		printf("\n\nCleanup, please wait ...\n");
		for (i = 0; i<PnioParams.StationNum; i++)
			Pnio64DisableStation(&__pUserList[i]);

		//Destroy PNIO realtime core
		Sha64PnioDestroy(&PnioParams);
	}

//******************
SEQ_DETACH(); //Detach from sequence memory (only for debugging)
//******************
}
