//*****************************************************************
//
//	This code is strictly reserved by SYBERA. It´s used only for
//	demonstration purposes. Any modification or integration
//	isn´t allowed without permission by SYBERA.
//
//  Copyright (c) 2012 SYBERA
//
//	Stationlist Configuration:
//
//	Station1:
//	Slot0:	FL IL 24 BK-PN-PAC
//	Slot1:	Inline-Master (intern)
//	Slot2:	IB IL 24 DO 4-ME
//	Slot3:	IB IL 24 DI 4-ME
//
//	Station2:
//	Slot0:	ILB PN 24 DI16 DIO16-2TX
//	Slot1:	16 digitale Ein-/Ausgänge
//	Slot2:	16 digitale Eingänge
//
//	Station3:
//	Slot0:	Anybus-S IRT
//	Slot1:	2 Byte Input
//	Slot2:	2 Byte Output
//
//*****************************************************************

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "c:\sha\sha64exp.h"
#include "c:\eth\Sha64EthCore.h"
#include "c:\pnt\Sha64PnioCore.h"
#include "c:\pnt\Pnio64Control.h"
#include "c:\pnt\Pnio64Macros.h"

#pragma comment( lib, "c:\\sha\\sha64dll.lib" )
#pragma comment( lib, "c:\\eth\\sha64ethcore.lib" )
#pragma comment( lib, "c:\\pnt\\sha64pniocore.lib" )

#pragma warning(disable:4996)
#pragma warning(disable:4748)

#define MAX_STATION			3
#define SEND_CYCLE_TIME		4000
#define REALTIME_PERIOD		200

//*******************************************************************************
//#define SEND_PLL		1	//!!! need to set SEND_MODE_PLL(0)  in registry) !!! 
#define SEND_CLOCKED	1	//!!! need to set SEND_MODE_USER(1) in registry) !!! 
//#define SEND_FLUSHED	1	//!!! need to set SEND_MODE_USER(1) in registry) !!! 
//*******************************************************************************

PETH_STACK		__pUserStack = NULL;
PETH_STACK		__pSystemStack = NULL;
PSTATION_INFO	__pUserList = 0;			//PSTATION_INFO structure for windows application task
PSTATION_INFO	__pSystemList = 0;			//PSTATION_INFO structure for realtime application task
ULONG			__StationNum = 0;
FP_PNIO_ENTER	__fpPnioEnter = NULL;		//Function pointer to PnioEnter
FP_PNIO_EXIT	__fpPnioExit = NULL;		//Function pointer to PnioExit
UCHAR			__InData[MAX_STATION][2] = { { 0 } };
UCHAR			__OutData[MAX_STATION][2] = { { 0 } };
ULONG			__UpdateCnt = 0;
BOOLEAN			__bInitDone = FALSE;		//Initialization flag
ULONG			__CycleCnt = 0;				//Cycle counter


void static AppTask(PVOID)
{
	PUCHAR pInData;
	PUCHAR pOutData;

	//Check if initialization is done
	if (__bInitDone == FALSE)
		return;

	//**************************************
	//Call PNIO enter function
	PSTATION_INFO pStation = __fpPnioEnter(NULL, NULL, 0);
	if (pStation)
	{
	//**************************************

		//Check for station 1 (FL IL 24 BK-PN-PAC)
		if (__PnioCheckStationName(pStation, "station1"))
		{
			//Get input  data from Slot3 (IB IL 24 DI 4-ME)
			//Set output data to   Slot2 (IB IL 24 DO 4-ME)
			PNIO_GET_INPUT_DATAPTR (pStation, 3, 0, &pInData);
			PNIO_GET_OUTPUT_DATAPTR(pStation, 2, 0, &pOutData);
			if ((pInData) &&
				(pOutData))
			{
				//Set input data to output data
				pOutData[0] = __InData[0][0] = pInData[0];
			}
		}

		//Check for station 2 (ILB PN 24 DI16 DIO16-2TX)
		if (__PnioCheckStationName(pStation, "station2"))
		{
			//Get input  data from Slot2 (16 digitale Eingänge)
			//Set output data to   Slot1 (16 digitale Ein-/Ausgänge)
			PNIO_GET_INPUT_DATAPTR (pStation, 2, 0, &pInData);
			PNIO_GET_OUTPUT_DATAPTR(pStation, 1, 0, &pOutData);
			if ((pInData) &&
				(pOutData))
			{
				//Set input data to output data
				pOutData[0] = __InData[1][0] = pInData[0];
				pOutData[1] = __InData[1][1] = pInData[1];
			}
		}

		//Check for station 3 (Anybus-S PRT/IRT)
		if (__PnioCheckStationName(pStation, "station3"))
		{
			//Get input  data from Slot1 (2 Bytes Input)
			//Set output data to   Slot2 (2 Bytes Output)
			PNIO_GET_INPUT_DATAPTR (pStation, 1, 0, &pInData);
			PNIO_GET_OUTPUT_DATAPTR(pStation, 2, 0, &pOutData);
			if ((pInData) &&
				(pOutData))
			{
				//Set input data to output data
				pOutData[0] = __InData[2][0] = pInData[0];
				pOutData[1] = __InData[2][1] = pInData[1];
			}
		}

		//Increase update counter
		__UpdateCnt++;
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


void main(void)
{
	char ActChar[4] = { '\\', '|', '/', '-' };
	ULONG c = 0;
	ULONG i;

	printf("\n*** ProfiNET Core Realtime Level2 Test ***\n\n");

	//Set ethernet mode
	__PntSetEthernetMode(0, REALTIME_PERIOD, FALSE);

	//Required PNIO parameters
	PNIO_PARAMS PnioParams;
	memset(&PnioParams, 0, sizeof(PNIO_PARAMS));
	PnioParams.EthParams.dev_num = 0;
	PnioParams.EthParams.period = 100;
	PnioParams.EthParams.fpAppTask = AppTask;

	//Set station list file path
	sprintf(PnioParams.szStationFile, "c:\\pnt\\stationlist.par");

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
		printf("\nPress any key ...\n");
		while (!kbhit())
		{
			//Print input payload information
			printf("InData(0):[%02x], InData(1):[%02x][%02x], InData(2):[%02x][%02x], UpdateCnt:%i [%c]\r",					
					__InData[0][0],
					__InData[1][0], __InData[1][1],
					__InData[2][0], __InData[2][1],
					__UpdateCnt, ActChar[++c%4]);

			//Do some delay
			Sleep(100);
		}

		//Disable stations
		for (i=0; i<PnioParams.StationNum; i++)
			Pnio64DisableStation(&__pUserList[i]);

		//Destroy PNIO realtime core
		Sha64PnioDestroy(&PnioParams);
	}
}
