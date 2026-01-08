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
// !!! Note: This sample requires an INTEL IEEE1588 Adapter !!!
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
#include "ProfiDriveGlobal.h"
#include "ProfiDriveRead.h"
#include "ProfiDriveChange.h"

#pragma comment( lib, "c:\\sha\\sha64dll.lib" )
#pragma comment( lib, "c:\\eth\\sha64ethcore.lib" )
#pragma comment( lib, "c:\\pnt\\sha64pniocore.lib" )

#pragma warning(disable:4996)
#pragma warning(disable:4748)

//#define CN8032_SI6SINGLE_SI6DOUBLE	1
#define SI6SINGLE_SI6DOUBLE				1
//#define SI6SINGLE						1
#define ISOCHRONE_MODE	1


#define MEM_TAG				0x1234
#define REALTIME_PERIOD			1	//IRT is clock base sceduled by the I210 adapter (no period is required)

#ifdef CN8032_SI6SINGLE_SI6DOUBLE
	#define CN8032_INDEX			0	//0
	#define DRIVE1_INDEX			1	//0
	#define DRIVE2_INDEX			2	//1
#endif

#ifdef SI6SINGLE_SI6DOUBLE
	#define DRIVE1_INDEX			0	//0
	#define DRIVE2_INDEX			1	//1
#endif

#ifdef SI6SINGLE
	#define DRIVE1_INDEX			0	//0
#endif

#define	MAX_DRIVE_NUM			5
#define DRIVE_SET_INDEX0		0
#define DRIVE_SET_INDEX1		1
#define DRIVE_SET_INDEX2		2
#define ISOMODE_INDEX0			0
#define ISOMODE_INDEX1			1


typedef struct _DRIVE_SET_TEL5
{
	//Telegram5 inputs
	PROFIDRIVE_STW1	Stw1;
	PROFIDRIVE_STW2	Stw2;
	ULONG 			Kpc;
	ULONG 			Nsoll;
	ULONG 			Xerr;

	//Telegram5 outputs
	PROFIDRIVE_ZSW1	Zsw1;
	PROFIDRIVE_ZSW2	Zsw2;
	ULONG 			Nist;
	TYPE64			Xist;

	//Protocol elements
	USHORT			AppCycleCnt;
	USHORT			AppCycleMax;
	ULONG			UpdCnt;

} DRIVE_SET_TEL5, *PDRIVE_SET_TEL5;

//**************************************
//Init sequencing elements (only for debugging)
SEQ_INIT_ELEMENTS();
//**************************************

PVOID			__pUserMem = NULL;
PVOID			__pSystemMem = NULL;
PETH_STACK		__pUserStack = NULL;
PETH_STACK		__pSystemStack = NULL;
PSTATION_INFO	__pUserList = 0;			//PSTATION_INFO structure for windows application task
PSTATION_INFO	__pSystemList = 0;			//PSTATION_INFO structure for realtime application task
ULONG			__StationNum = 0;
FP_PNIO_ENTER	__fpPnioEnter = NULL;		//Function pointer to PnioEnter
FP_PNIO_EXIT	__fpPnioExit = NULL;		//Function pointer to PnioExit
BOOLEAN			__bInitDone = FALSE;		//Initialization flag


__inline BOOLEAN __CheckCtrlAppCycle(PDRIVE_SET_TEL5 pDriveSet)
{
	//Check IRT phase
	if (__pSystemStack)
	if (__pSystemStack->phase_num)
	if (__pSystemStack->phase_cnt == 0)
	{
		//Update application counter
		++pDriveSet->AppCycleCnt;
		if (pDriveSet->AppCycleCnt >= pDriveSet->AppCycleMax)
			pDriveSet->AppCycleCnt = 0;

		//*****************************************************
		SYSTEM_SEQ_CAPTURE("CAC", __LINE__, pDriveSet->AppCycleCnt, TRUE);
		//*****************************************************

		//Return app cycle flag
		return (pDriveSet->AppCycleCnt == 0) ? TRUE : FALSE;
	}

	//No cycle active
	return FALSE;
}


__inline void __UpdateTel5SignOfLife(
								PSTATION_INFO pStation,
								PPROFIDRIVE_STW2 pStw2)
{
	PTEL5_OUTPUT pTel5Output = NULL;

	//Get telegram 5 output pointer
	PNIO_GET_OUTPUT_DATAPTR(pStation, 1, 1, &pTel5Output);
	if (pTel5Output)
	{
#ifdef ISOCHRONE_MODE
		__ProfiDrive_UpdateSignOfLive(pStw2);
		ETH_SET_BIGENDIAN_USHORT(&pTel5Output->s.stw2.word, pStw2->word);

		//*****************************************************
		SYSTEM_SEQ_CAPTURE("SOF", __LINE__, pStw2->word, TRUE);
		//*****************************************************
#endif
	}
}


__inline void __DriveCtrlApp(
						PTEL5_INPUT  pTel5Input,
						PTEL5_OUTPUT pTel5Output,
						PDRIVE_SET_TEL5 pDriveSet)
{
	//Get drive data (little endian)
	ETH_GET_BIGENDIAN_USHORT(&pTel5Input->s.zsw1.word, &pDriveSet->Zsw1.word);
	ETH_GET_BIGENDIAN_ULONG(&pTel5Input->s.nist_b,     &pDriveSet->Nist);
	ETH_GET_BIGENDIAN_ULONG(&pTel5Input->s.g1_xist1,   &pDriveSet->Xist.bit32[0]);
	ETH_GET_BIGENDIAN_ULONG(&pTel5Input->s.g1_xist2,   &pDriveSet->Xist.bit32[1]);

	if (pDriveSet->Zsw1.s.op_enabled)
	{
		pDriveSet->Kpc = 150000;
		pDriveSet->Xerr = 10000;
		pDriveSet->Nsoll = 100000;
	}

	//Set drive data (big endian)
	ETH_SET_BIGENDIAN_USHORT(&pTel5Output->s.stw1.word, pDriveSet->Stw1.word);
	ETH_SET_BIGENDIAN_ULONG( &pTel5Output->s.kpc,       pDriveSet->Kpc);
	ETH_SET_BIGENDIAN_ULONG( &pTel5Output->s.xerr,      pDriveSet->Xerr);
	ETH_SET_BIGENDIAN_ULONG( &pTel5Output->s.nsoll_b,   pDriveSet->Nsoll);

	pDriveSet->UpdCnt++;
}


__inline void __DoLogic(void)
{
	PDRIVE_SET_TEL5 pDriveSetList = (PDRIVE_SET_TEL5 )__pSystemMem;

	//Check IRT phase
	if (__pSystemStack)
	if (__pSystemStack->phase_num)
	if (__pSystemStack->phase_cnt == 2)
	{
		PTEL5_INPUT  pTel5Input[MAX_DRIVE_NUM]  = { NULL };
		PTEL5_OUTPUT pTel5Output[MAX_DRIVE_NUM] = { NULL };

#ifdef SI6SINGLE
		//Get Drive1 input/output telegram pointer
		PNIO_GET_INPUT_DATAPTR ((&__pSystemList[DRIVE1_INDEX]), 1, 1, &pTel5Input[DRIVE_SET_INDEX0]);
		PNIO_GET_OUTPUT_DATAPTR((&__pSystemList[DRIVE1_INDEX]), 1, 1, &pTel5Output[DRIVE_SET_INDEX0]);
#else
		//Get Drive1 input/output telegram pointer
		PNIO_GET_INPUT_DATAPTR ((&__pSystemList[DRIVE1_INDEX]), 1, 1, &pTel5Input[DRIVE_SET_INDEX0]);
		PNIO_GET_OUTPUT_DATAPTR((&__pSystemList[DRIVE1_INDEX]), 1, 1, &pTel5Output[DRIVE_SET_INDEX0]);

		//Get Drive2 input/output telegram pointer
		PNIO_GET_INPUT_DATAPTR ((&__pSystemList[DRIVE2_INDEX]), 1, 1, &pTel5Input[DRIVE_SET_INDEX1]);
		PNIO_GET_OUTPUT_DATAPTR((&__pSystemList[DRIVE2_INDEX]), 1, 1, &pTel5Output[DRIVE_SET_INDEX1]);

		//Get Drive3 input/output telegram pointer
		PNIO_GET_INPUT_DATAPTR ((&__pSystemList[DRIVE2_INDEX]), 2, 1, &pTel5Input[DRIVE_SET_INDEX2]);
		PNIO_GET_OUTPUT_DATAPTR((&__pSystemList[DRIVE2_INDEX]), 2, 1, &pTel5Output[DRIVE_SET_INDEX2]);
#endif

#ifdef SI6SINGLE
		if ((pTel5Input[DRIVE_SET_INDEX0]) && (pTel5Output[DRIVE_SET_INDEX0]))
#else
		if ((pTel5Input[DRIVE_SET_INDEX0]) && (pTel5Output[DRIVE_SET_INDEX0]))
		if ((pTel5Input[DRIVE_SET_INDEX1]) && (pTel5Output[DRIVE_SET_INDEX1]))
		if ((pTel5Input[DRIVE_SET_INDEX2]) && (pTel5Output[DRIVE_SET_INDEX2]))
#endif
		{
			//*****************************************************
			SYSTEM_SEQ_CAPTURE("LOG", __LINE__, 0, TRUE);
			//*****************************************************

#ifdef SI6SINGLE
			__DriveCtrlApp(pTel5Input[DRIVE_SET_INDEX0], pTel5Output[DRIVE_SET_INDEX0], &pDriveSetList[DRIVE_SET_INDEX0]);
#else
			__DriveCtrlApp(pTel5Input[DRIVE_SET_INDEX0], pTel5Output[DRIVE_SET_INDEX0], &pDriveSetList[DRIVE_SET_INDEX0]);
			__DriveCtrlApp(pTel5Input[DRIVE_SET_INDEX1], pTel5Output[DRIVE_SET_INDEX1], &pDriveSetList[DRIVE_SET_INDEX1]);
			__DriveCtrlApp(pTel5Input[DRIVE_SET_INDEX2], pTel5Output[DRIVE_SET_INDEX2], &pDriveSetList[DRIVE_SET_INDEX2]);
#endif
		}
	}
}


void static AppTask(PVOID)
{
	PDRIVE_SET_TEL5 pDriveSetList = (PDRIVE_SET_TEL5 )__pSystemMem;
	PSTATION_INFO pRxStationList[10] = { 0 };
	ULONG RxStationNum = 0;

	//Check ProfiDrive CAC and do isochrone mode sign-of-life

#ifdef SI6SINGLE
	if (__CheckCtrlAppCycle(&pDriveSetList[DRIVE_SET_INDEX0])) { __UpdateTel5SignOfLife(&__pSystemList[DRIVE1_INDEX], &pDriveSetList[DRIVE_SET_INDEX0].Stw2); }
#else
	if (__CheckCtrlAppCycle(&pDriveSetList[DRIVE_SET_INDEX0])) { __UpdateTel5SignOfLife(&__pSystemList[DRIVE1_INDEX], &pDriveSetList[DRIVE_SET_INDEX0].Stw2); }
	if (__CheckCtrlAppCycle(&pDriveSetList[DRIVE_SET_INDEX1])) { __UpdateTel5SignOfLife(&__pSystemList[DRIVE2_INDEX], &pDriveSetList[DRIVE_SET_INDEX1].Stw2); }
	if (__CheckCtrlAppCycle(&pDriveSetList[DRIVE_SET_INDEX2])) { __UpdateTel5SignOfLife(&__pSystemList[DRIVE2_INDEX], &pDriveSetList[DRIVE_SET_INDEX2].Stw2); }
#endif

	//Check if initialization is done
	if (__bInitDone) 
	{ 
		//Call PNIO enter function and
		//get current returned stations within this RTC3 cycle
		__fpPnioEnter(pRxStationList, &RxStationNum, 0);

		//*****************************************************
		SYSTEM_SEQ_CAPTURE("LOG", __LINE__, RxStationNum, FALSE);
		//*****************************************************

		//Do logic
		__DoLogic();

		//Call PNIO exit function
		__fpPnioExit(NULL);
	}
}

//************************************************************************************
//************************************************************************************
//************************************************************************************

#define INDEX_DIAG_MAIN_QUAL_STAT_ONE_AR		0xF00C	//Diagnosis, Maintenance, Qualified and Status for one AR
void ReadDiagnostics(PSTATION_INFO pStation)
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


BOOLEAN ChangeDriveParameters(
						PSTATION_INFO pStation,
						USHORT Slot,
						USHORT SubSlot)
{
	PROFIDRIVE_PARAM_CHANGE_REQ  ChangeReqList[1];
	PROFIDRIVE_PARAM_CHANGE_RESP ChangeRespList[1];
	PROFIDRIVE_PARAM_READ_REQ	 ReadReqList[1];
	PROFIDRIVE_PARAM_READ_RESP	 ReadRespList[1];

	//Change parameter 925 (MAX_LS_FALURE_NUM)
	ChangeReqList[0].hdr.Attr = PROFIDRIVE_ATTR_VALUE;
	ChangeReqList[0].hdr.ElementNum = 0;
	ChangeReqList[0].hdr.ParamID = 925;
	ChangeReqList[0].hdr.SubIndex = 0;
	ChangeReqList[0].param_hdr.Format = DATA_TYPE_UNSIGNED16;
	ChangeReqList[0].param_hdr.ValueNum = 1;
	ChangeReqList[0].u.ValueBit16List[0] = MAX_LS_FALURE_NUM;
	ProfiDriveChangeParams(pStation, Slot, SubSlot, 1, &ChangeReqList[0], &ChangeRespList[0]);

	//Read parameter 925
	ReadReqList[0].param_hdr.Attr = PROFIDRIVE_ATTR_VALUE;
	ReadReqList[0].param_hdr.ElementNum = 0;
	ReadReqList[0].param_hdr.ParamID = 925;
	ReadReqList[0].param_hdr.SubIndex = 0;
	if (ERROR_SUCCESS != ProfiDriveReadParams(pStation, Slot, SubSlot, 1, &ReadReqList[0], &ReadRespList[0]))
		return FALSE;
}


#include "time.h"
__inline BOOLEAN __WaitForStatus(
							PUSHORT pStatus,
							USHORT BitMask,
							USHORT Flags,
							ULONG msec)
{
	//Do timeout loop
	clock_t TimeoutTime = clock() + msec;
	while (clock() < TimeoutTime)
	{
		//Check status with bitmask for flags set
		if ((*pStatus & BitMask) == Flags)
			return TRUE;

		//Do some delay
		Sleep(10);
	}

	//Timeout only, when time was set
	return (msec) ? FALSE : TRUE;
}


inline void InitDrive(
					PISOMODE_INFO pIsoModeInfo,
					PDRIVE_SET_TEL5 pDriveSet)
{
	//Set application cycle
	pDriveSet->AppCycleMax = pIsoModeInfo->Cacf;	Sleep(100);
}


__inline BOOLEAN __FaultReset(
						PDRIVE_SET_TEL5 pDriveSet,
						ULONG msec)
{
	//Do timeout loop
	clock_t TimeoutTime = clock() + msec;
	while (clock() < TimeoutTime)
	{
		//Check if fault is present
		if (pDriveSet->Zsw1.s.fault == 0)
			return TRUE;

		//Set fault acknowledge pulse (>20 msec)
		pDriveSet->Stw1.s.fault_ack = 1; Sleep(100);
		pDriveSet->Stw1.s.fault_ack = 0; Sleep(100);
	}

	//Could not reset fault
	return FALSE;
}


BOOLEAN EnableDrive(PDRIVE_SET_TEL5 pDriveSet)
{
	//Wait for control requested and set PLC control
	if (__WaitForStatus(&pDriveSet->Zsw1.word, ZSW1_CTRL_REQESTED, ZSW1_CTRL_REQESTED, 1000))
	pDriveSet->Stw1.s.plc_ctrl = 1;

	//Do some delay
	Sleep(100);

	//Do retry loop
	for (int retry = 0; retry < 5; retry++)
	{
		//Reset bits
		pDriveSet->Stw1.s.coast_stop_off = 0;
		pDriveSet->Stw1.s.quick_stop_off = 0;
		pDriveSet->Stw1.s.switch_on = 0;
		pDriveSet->Stw1.s.op_enable = 0;

		//Do some delay
		Sleep(100);

		//Check for fault and do acknowledgement
		if (__FaultReset(pDriveSet, 3000) == FALSE) { continue; }

		//Do some delay
		Sleep(100);

		//Switch on inhibited	
		pDriveSet->Stw1.s.coast_stop_off = 1;
		pDriveSet->Stw1.s.quick_stop_off = 1;

		//Do some delay
		Sleep(100);

		//Wait for ready switch on, then switch on
		if (__WaitForStatus(&pDriveSet->Zsw1.word, ZSW1_RDY_SWITCH_ON, ZSW1_RDY_SWITCH_ON, 1000) == FALSE) { continue; }
		pDriveSet->Stw1.s.switch_on = 1;

		//Do some delay
		Sleep(100);

		//Wait for ready OP, then enable OP
		if (__WaitForStatus(&pDriveSet->Zsw1.word, ZSW1_RDY_OP, ZSW1_RDY_OP, 1000) == FALSE) { continue; }
		pDriveSet->Stw1.s.op_enable = 1;


		//Do some delay
		Sleep(100);

		pDriveSet->Stw1.s.stop = 1;
		pDriveSet->Stw1.s.im_stop = 1;
		pDriveSet->Stw1.s.trav_on = 1;

		if (__WaitForStatus(&pDriveSet->Zsw1.word, ZSW1_OP_ENABLED, ZSW1_OP_ENABLED, 1000))
		{
			//Do some delay
			Sleep(100);

			//Check for fault and do acknowledgement
			if (pDriveSet->Zsw1.s.fault == 0)
				return TRUE;
		}

	}

	//Enable drive successful
	return FALSE;
}


void DisableDrive(PDRIVE_SET_TEL5 pDriveSet)
{
	//Disable drive
	pDriveSet->Stw1.s.switch_on = 0;		Sleep(100);
	pDriveSet->Stw1.s.quick_stop_off = 0;	Sleep(100);
	pDriveSet->Stw1.s.op_enable = 0;		Sleep(100);

	//Disable PLC control
	pDriveSet->Stw1.s.plc_ctrl = 0;			Sleep(100);
}


void ReferenceDrive(PDRIVE_SET_TEL5 pDriveSet)
{
	//Set target position to current position
//	pDriveSet->Nsoll = pDriveSet->Nist;
}


__inline BOOLEAN __CheckCyclicProtocol(void)
{
	//Get TX/RX count from ethernet stack
	float TxCnt = (float)__pUserStack->tx_cnt;
	float RxCnt = (float)__pUserStack->rx_cnt;

	//Check for lost frames
	if (TxCnt > (RxCnt * 1.2))
		return FALSE;

	//Everything is OK
	return TRUE;
}


void main(void)
{
	HANDLE	hMem = NULL;
	ULONG	MemSize = sizeof(DRIVE_SET_TEL5) * MAX_DRIVE_NUM;
	char	ActChar[4] = { '\\', '|', '/', '-' };
	ULONG	c = 0;

	printf("\n*** ProfiNET ProfiDrive SI6 Test ***\n\n");

	//**********************************
	//Attach to sequence memory
	//Reset sequence memory (bRun, bRingMode, szFilterSign, FilterLine)
	SEQ_ATTACH(); //Just comment this line to disable sequencing
	SEQ_RESET(TRUE, TRUE, NULL, 0);
	//**********************************

	//Allocate memory for drive sets
	if (ERROR_SUCCESS == Sha64AllocMemWithTag(
										TRUE,					//Cached memory
										0x1234,					//Memory tag
										MemSize,				//Memory size
										(void**)&__pUserMem,	//Pointer to memory for Windows access
										(void**)&__pSystemMem,	//Pointer to memory for Realtime access
										NULL,					//Physical memory address
										&hMem))					//Handle to memory device
	{
		PDRIVE_SET_TEL5 pDriveSetList = (PDRIVE_SET_TEL5 )__pUserMem;
		memset(pDriveSetList, 0, MemSize);

		//Set ethernet mode
		__IrtSetEthernetMode(0);

		//Required PNIO parameters
		PNIO_PARAMS PnioParams;
		memset(&PnioParams, 0, sizeof(PNIO_PARAMS));
		PnioParams.EthParams.dev_num = 0;
		PnioParams.EthParams.period = REALTIME_PERIOD;
		PnioParams.EthParams.fpAppTask = AppTask;

		//*********************************************
		//!!! Set correct path to station list file !!!
#ifdef CN8032_SI6SINGLE_SI6DOUBLE
		sprintf(PnioParams.szStationFile, "c:\\pnt\\StationList (CN8032_SI6SINGLE_SI6DOUBLE).par");
#endif
#ifdef SI6SINGLE_SI6DOUBLE
		sprintf(PnioParams.szStationFile, "c:\\pnt\\StationList (SI6Single_SI6Double).par");
#endif
#ifdef SI6SINGLE
		sprintf(PnioParams.szStationFile, "c:\\pnt\\StationList (SI6Single).par");
#endif

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

			//Enable station
#ifdef CN8032_SI6SINGLE_SI6DOUBLE
			Pnio64EnableStation(&__pUserList[CN8032_INDEX]);
			Pnio64EnableStation(&__pUserList[DRIVE1_INDEX]);
			Pnio64EnableStation(&__pUserList[DRIVE2_INDEX]);
#endif
#ifdef SI6SINGLE_SI6DOUBLE
			Pnio64EnableStation(&__pUserList[DRIVE1_INDEX]);
			Pnio64EnableStation(&__pUserList[DRIVE2_INDEX]);
#endif
#ifdef SI6SINGLE
			Pnio64EnableStation(&__pUserList[DRIVE1_INDEX]);
#endif

			//Change ProfiDrive parameters
#ifdef SI6SINGLE
			ChangeDriveParameters(&__pUserList[DRIVE1_INDEX], 1, 2);
#else
			ChangeDriveParameters(&__pUserList[DRIVE1_INDEX], 1, 2);
			ChangeDriveParameters(&__pUserList[DRIVE2_INDEX], 1, 2);
			ChangeDriveParameters(&__pUserList[DRIVE2_INDEX], 2, 2);
#endif

			//Init drive
#ifdef SI6SINGLE
			InitDrive(&__pUserList[DRIVE1_INDEX].IsoModeList[0], &pDriveSetList[DRIVE_SET_INDEX0]);
#else
			InitDrive(&__pUserList[DRIVE1_INDEX].IsoModeList[0], &pDriveSetList[DRIVE_SET_INDEX0]);
			InitDrive(&__pUserList[DRIVE2_INDEX].IsoModeList[0], &pDriveSetList[DRIVE_SET_INDEX1]);
			InitDrive(&__pUserList[DRIVE2_INDEX].IsoModeList[1], &pDriveSetList[DRIVE_SET_INDEX2]);
#endif

			//Enable drive
#ifdef SI6SINGLE
			if (EnableDrive(&pDriveSetList[DRIVE_SET_INDEX0]) == FALSE) { goto __EXIT; }
#else
			if (EnableDrive(&pDriveSetList[DRIVE_SET_INDEX0]) == FALSE) { goto __EXIT; }
			if (EnableDrive(&pDriveSetList[DRIVE_SET_INDEX1]) == FALSE) { goto __EXIT; }
			if (EnableDrive(&pDriveSetList[DRIVE_SET_INDEX2]) == FALSE) { goto __EXIT; }
#endif

			//Set drive reference point			
#ifdef SI6SINGLE
			ReferenceDrive(&pDriveSetList[DRIVE_SET_INDEX0]);
#else
			ReferenceDrive(&pDriveSetList[DRIVE_SET_INDEX0]);
			ReferenceDrive(&pDriveSetList[DRIVE_SET_INDEX1]);
			ReferenceDrive(&pDriveSetList[DRIVE_SET_INDEX2]);
#endif

			//Wait for key pressed
			printf("Number of Stations: %i\n", PnioParams.StationNum);
			printf("\nPress any key ...\n");
			while (!kbhit())
			{
				//Print input payload information
#ifdef SI6SINGLE
				printf("D1 ZSW1:[%04x] STW1:[%04x] Upd:%i [%c]\r",
						pDriveSetList[DRIVE_SET_INDEX0].Zsw1.word,
						pDriveSetList[DRIVE_SET_INDEX0].Stw1.word,
						pDriveSetList[DRIVE_SET_INDEX0].UpdCnt,
						ActChar[++c%4]);
#else
				printf("D1 ZSW1:[%04x] STW1:[%04x] Upd:%i D2 ZSW1:[%04x] STW1:[%04x] Upd:%i D3 ZSW1:[%04x] STW1:[%04x] Upd:%i [%c]\r",
						pDriveSetList[DRIVE_SET_INDEX0].Zsw1.word,
						pDriveSetList[DRIVE_SET_INDEX0].Stw1.word,
						pDriveSetList[DRIVE_SET_INDEX0].UpdCnt,
						pDriveSetList[DRIVE_SET_INDEX1].Zsw1.word,
						pDriveSetList[DRIVE_SET_INDEX1].Stw1.word,
						pDriveSetList[DRIVE_SET_INDEX1].UpdCnt,
						pDriveSetList[DRIVE_SET_INDEX2].Zsw1.word,
						pDriveSetList[DRIVE_SET_INDEX2].Stw1.word,
						pDriveSetList[DRIVE_SET_INDEX2].UpdCnt,
						ActChar[++c%4]);
#endif
				//Check/Read diagnosics
//				if (__pUserList)
//				if (__pUserList[0].State == PNIO_STATE_ERROR)
//					__ReadDiagnostics(&__pUserList[0]);

				//Check cyclic protocol
				if (__CheckCyclicProtocol() == FALSE) { printf("Cyclic protocol failed ...\n"); break; }

				//Do some delay
				Sleep(100);
			}

			//Disable all drives
#ifdef SI6SINGLE
			DisableDrive(&pDriveSetList[DRIVE_SET_INDEX0]);
#else
			DisableDrive(&pDriveSetList[DRIVE_SET_INDEX0]);
			DisableDrive(&pDriveSetList[DRIVE_SET_INDEX1]);
			DisableDrive(&pDriveSetList[DRIVE_SET_INDEX2]);
#endif

			//Do some delay
			Sleep(100);

__EXIT:

			//Disable stations (reverse direction)
			for (int i = PnioParams.StationNum - 1; i >= 0; i--)
				Pnio64DisableStation(&__pUserList[i]);

			//Destroy PNIO realtime core
			Sha64PnioDestroy(&PnioParams);
		}

		//Release memory
		Sha64FreeMem(hMem);
	}

	//******************
	SEQ_DETACH(); //Detach from sequence memory (only for debugging)
	//******************

	printf("Press any key ...\n");
	while (!kbhit()) { Sleep(100); }
}
