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
//	Slot0:	ILB_PN24_DI16_DIO16
//	Slot1:	16 digitale Ein-/Ausgänge
//	Slot2:	16 digitale Eingänge
//
//*****************************************************************

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "c:\eth\Sha64EthCore.h"
#include "c:\pnt\Sha64PnioCore.h"
#include "c:\sha\sha64exp.h"
#include "c:\pnt\Pnio64Control.h"
#include "c:\pnt\Pnio64Macros.h"

#pragma comment( lib, "c:\\sha\\sha64dll.lib" )
#pragma comment( lib, "c:\\eth\\sha64ethcore.lib" )
#pragma comment( lib, "c:\\pnt\\sha64pniocore.lib" )

#pragma warning(disable:4996)
#pragma warning(disable:4748)

//Index (API specific)
#define INDEX_REAL_ID_DATA4				0xF000	//RealIdentificationData for one API Expression 4 applies.
#define INDEX_DIAG_CH_CODING4			0xF00A	//Diagnosis in channel coding for one API Expression 4 applies.
#define INDEX_DIAG_ALL_CH_CODING4		0xF00B	//Diagnosis in all codings for one API Expression 4 applies.
#define INDEX_DIAG_MAINT_QUAL_STAT4		0xF00C	//Diagnosis, Maintenance, Qualified and Status for one API Expression 4 applies.
#define INDEX_CH_MAINT_REQ4				0xF010	//Maintenance required in channel coding for one API Expression 4 applies.
#define INDEX_CH_MAINT_DEMAND4			0xF011	//Maintenance demanded in channel coding for one API Expression 4 applies.
#define INDEX_ALL_CH_MAINT_REQ4			0xF012	//Maintenance required in all codings for one API Expression 4 applies.
#define INDEX_ALL_CH_MAINT_DEMAND4		0xF013	//Maintenance demanded in all codings for one API Expression 4 applies.
#define INDEX_AR_DATA4					0xF020	//ARData for one API Expression 4 applies.

//Index (device specific)
#define INDEX_DIAG_MAINT_QUAL_STAT5		0xF80C	//Diagnosis, Maintenance, Qualified and Status for one device Expression 5 applies.
#define INDEX_AR_DATA5					0xF820	//ARData Expression 5 applies.
#define INDEX_API_DATA5					0xF821	//APIData Expression 5 applies.
#define INDEX_LOG_DATA5					0xF830	//LogData Expression 5 applies.
#define INDEX_PDEV_DATA5				0xF831	//PDevData Expression 5 applies.
#define INDEX_IM0_FILTER_DATA5			0xF840	//I&M0FilterData Expression 5 applies.
#define INDEX_PD_REAL_DATA5				0xF841	//PDRealData Expression 5 applies.
#define INDEX_PD_EXP_DATA5				0xF842	//PDExpectedData Expression 5 applies.


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
PSTATION_INFO	__pStation = NULL;
ULONG			__UpdateCnt = 0;
ULONG			__CycleCnt = 0;				//Cycle counter
BOOLEAN			__bInitDone = FALSE;		//Initialization flag


void static AppTask(PVOID)
{
	//Check if initialization is done
	if (__bInitDone == FALSE)
		return;

	//**************************************
	//Call PNIO enter function
	__pStation = __fpPnioEnter(NULL, NULL, 0);
	if (__pStation)
	{
	//**************************************

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
	ULONG i, Result;

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
	sprintf(PnioParams.szStationFile, "c:\\pnt\\stationlist.par");
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
			Pnio64EnableStation(&__pUserList[i]);

	//******************************
		USHORT RecordLen = 0x1000;
		UCHAR Buffer[0x1000];
		memset(Buffer, 0 ,0x1000);
/*
		Result = Pnio64ServiceWrite(
							&__pUserList[0],			//Station pointer
							0,							//Api ID
							2,							//Slot number
							1,							//SubSlot Number (SubModule + 1)
							0xB100,						//Record index
							3,							//Record length
							(PUCHAR)"\xBD\x00\x04");	//Record data
*/								
		Result = Pnio64ServiceRead(
							&__pUserList[0],		//Station pointer
							0,                      //Api ID
							0,						//Slot number
							0,						//SubSlot Number (SubModule + 1)
							0xE000,					//Record index
							&RecordLen,				//Record length
							Buffer);				//Record data
	//******************************

		//Wait for key pressed
		printf("\nPress any key ...\n");
		while (!kbhit())
		{
			//Print input payload information
			printf("UpdateCnt:%i [%c]\r", __UpdateCnt, ActChar[++c%4]);

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
