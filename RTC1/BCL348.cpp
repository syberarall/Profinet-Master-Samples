//*****************************************************************
//
//	This code is strictly reserved by SYBERA. It´s used only for
//	demonstration purposes. Any modification or integration
//	isn´t allowed without permission by SYBERA.
//
//  Copyright (c) 2010 SYBERA
//
//	Stationlist Configuration:
//
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

#pragma warning (disable:4996)
#pragma warning(disable:4748)


//Define Control Command Flags
#define CTRLCMD_PRM_END				0x0001
#define CTRLCMD_APP_READY			0x0002
#define CTRLCMD_APP_RELEASE			0x0004
#define CTRLCMD_APP_DONE			0x0008

#define M22_DATA_LEN	10
#define M10_DATA_LEN	1

#define SEND_CYCLE_TIME		4000
#define REALTIME_PERIOD		200

//*******************************************************************************
//#define SEND_PLL		1	//!!! need to set SEND_MODE_PLL(0)  in registry) !!! 
#define SEND_CLOCKED	1	//!!! need to set SEND_MODE_USER(1) in registry) !!! 
//#define SEND_FLUSHED	1	//!!! need to set SEND_MODE_USER(1) in registry) !!! 
//*******************************************************************************

//**************************************
//Init sequencing elements (only for debugging)
//SEQ_INIT_ELEMENTS();
//**************************************

PETH_STACK		__pUserStack = NULL;
PETH_STACK		__pSystemStack = NULL;
PSTATION_INFO	__pUserList = 0;			//PSTATION_INFO structure for windows application task
PSTATION_INFO	__pSystemList = 0;			//PSTATION_INFO structure for realtime application task
ULONG			__StationNum = 0;
FP_PNIO_ENTER	__fpPnioEnter = NULL;		//Function pointer to PnioEnter
FP_PNIO_EXIT	__fpPnioExit = NULL;		//Function pointer to PnioExit
ULONG			__UpdateCnt = 0;
ULONG			__CycleCnt = 0;				//Cycle counter
BOOLEAN			__bInitDone = FALSE;		//Initialization flag
UCHAR			__M22Data[10] = { 0 };
UCHAR			__M10Data[1]  = { 0 };

#define	RECORD1_INDEX		0
#define	RECORD1_LEN			33
#define RECORD1_DATA		"\x00\x01\x0a\x00\x00\x00\x00\x04"\
							"\x00\x00\x00\x00\x00\x00\x00\x04"\
							"\x00\x00\x00\x00\x00\x00\x00\x04"\
							"\x00\x00\x00\x00\x00\x00\x00\x04"\
							"\x00"


void static AppTask(PVOID)
{
	PUCHAR pM22Data;
	PUCHAR pM10Data;

	//Check if initialization is done
	if (__bInitDone == FALSE)
		return;

	//**************************************
	//Call PNIO enter function
	PSTATION_INFO pStation = __fpPnioEnter(NULL, NULL, 0);
	if (pStation)
	{
	//**************************************

		//Check station name
		if (__PnioCheckStationName(pStation, "station2"))
		{
			//Anybus PRT Modul on development board
			//Get input  data from Slot1 (2 Byte Input)
			//Set output data to   Slot2 (2 Byte Output)
			PNIO_GET_INPUT_DATAPTR (pStation, 2, 0, &pM22Data);
			PNIO_GET_OUTPUT_DATAPTR(pStation, 1, 0, &pM10Data);

			if ((pM10Data) &&
				(pM22Data))
			{
				//Set input data to output data
				memcpy(__M22Data, pM22Data, M22_DATA_LEN);
				memcpy(pM10Data, __M10Data, M10_DATA_LEN);

				//*****************************************************
				//SYSTEM_SEQ_CAPTURE("APP", __LINE__, pOutData[0], TRUE);
				//SYSTEM_SEQ_CAPTURE("APP", __LINE__, pInData[0], TRUE);
				//*****************************************************
			}

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


__inline ULONG __EnableStation(PSTATION_INFO pStation)
{

	//Set station state
	pStation->State = PNIO_STATE_INIT;
	pStation->Error = 0;

	//***********************
	//Connect station
	//***********************
	if (ERROR_SUCCESS != (pStation->Error = Pnio64ServiceConnect(pStation)))
	{
		//Set error condition
		pStation->State = PNIO_STATE_ERROR;
		return -1;
	}

	//Set station state
	pStation->State = PNIO_STATE_CONNECTED;

	//**************************************
	//Write record data of each submodule
	//**************************************
	if (ERROR_SUCCESS != (pStation->Error = Pnio64ServiceWrite(
														pStation,		//Station pointer
														0,				//Api ID
														0,				//Slot number
														1,				//SubSlot Number (SubModule + 1)
														RECORD1_INDEX,	//Record index
														RECORD1_LEN,	//Record length
														(PUCHAR)RECORD1_DATA)))	//Record data
	{
		//Set error condition
		pStation->State = PNIO_STATE_ERROR;
		return -1;
	}

	//Set station state
	pStation->State = PNIO_STATE_WRITTEN;

	//***********************
	//Control station
	//***********************
	if (ERROR_SUCCESS != (pStation->Error = Pnio64ServiceCtrl(pStation, CTRLCMD_PRM_END)))
	{
		//Set error condition
		pStation->State = PNIO_STATE_ERROR;
		return -1;
	}

	//Set station state
	pStation->State = PNIO_STATE_RUNNING;
	return ERROR_SUCCESS;
}


__inline ULONG __DisableStation(PSTATION_INFO pStation)
{

	//Check for station error
	if (pStation->State == PNIO_STATE_ERROR)
		return -1;

	//Set state to controlled, to disable module outputs first
	pStation->State = PNIO_STATE_CONTROLLED;

	//***********************
	//Release station
	//***********************
	if (ERROR_SUCCESS != Pnio64ServiceRelease(pStation))
		return -1;

	//Set station state
	pStation->State = PNIO_STATE_INIT;
	return ERROR_SUCCESS;
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

	printf("\n*** ProfiNET Core Realtime Level2 Test ***\n\n");

	//Set ethernet mode
	__PntSetEthernetMode(0, REALTIME_PERIOD, FALSE);

	//Required PNIO parameters
	PNIO_PARAMS PnioParams;
	memset(&PnioParams, 0, sizeof(PNIO_PARAMS));
	PnioParams.EthParams.dev_num = 0;
	PnioParams.EthParams.period = REALTIME_PERIOD;
	PnioParams.EthParams.fpAppTask = AppTask;

	//*****************************
	//!!! Set correct path to station list file !!!
	sprintf(PnioParams.szStationFile, "stationlist (BCL348).par");
	//*****************************

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
			__EnableStation(&__pUserList[i]);

		//Wait for key pressed
		printf("Number of Stations: %i\n", PnioParams.StationNum);
		printf("\nPress any key ...\n");

		//Read barcode
		__M10Data[0] = 1; Sleep(100);
		__M10Data[0] = 0; Sleep(100);

		while (!kbhit())
		{
			//Print input payload information
			printf("M22Data:[%02x][%02x][%02x][%02x][%02x][%02x][%02x][%02x][%02x][%02x], UpdateCnt:%i [%c]\r",					
					__M22Data[0], __M22Data[1], __M22Data[2], __M22Data[3], __M22Data[4],
					__M22Data[5], __M22Data[6], __M22Data[7], __M22Data[8],	__M22Data[9],
					__UpdateCnt, ActChar[++c%4]);

			//Do some delay
			Sleep(100);
		}

		//Disable stations
		for (i=0; i<PnioParams.StationNum; i++)
			__DisableStation(&__pUserList[i]);

		//Destroy PNIO realtime core
		Sha64PnioDestroy(&PnioParams);
	}

//******************
//SEQ_DETACH(); //Detach from sequence memory (only for debugging)
//******************
}
