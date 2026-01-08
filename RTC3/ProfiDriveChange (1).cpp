//*****************************************************************
//	This code is strictly reserved by SYBERA GmbH
//  Copyright (c) 2025 SYBERA
//*****************************************************************

#pragma warning(disable:4748)

#include "ProfiDriveGlobal.h"
#include "ProfiDriveChange.h"


//Get externals
extern UCHAR __ProfiDriveReqRef;

extern __inline USHORT __SetProfiDriveRequestHeader(UCHAR, UCHAR, UCHAR, UCHAR, PUCHAR);
extern __inline USHORT __GetProfiDriveResponseHeader(PUCHAR, PUCHAR, PUCHAR, PUCHAR, PUCHAR);


//********************************************************************************
// Set request functions
//********************************************************************************

__inline USHORT __SetProfiDriveChangeRequestParam(
										PPROFIDRIVE_PARAM_CHANGE_REQ pParamReq,
										PUCHAR pBuffer)
{
	PPROFIDRIVE_PARAM_CHANGE_REQ pBlock = (PPROFIDRIVE_PARAM_CHANGE_REQ)pBuffer;
	UCHAR Format     = pParamReq->param_hdr.Format;
	UCHAR ValueNum   = pParamReq->param_hdr.ValueNum;
	USHORT ValueSize = 0;

	//Set block header elements
	pBlock->hdr.Attr       = pParamReq->hdr.Attr;
	pBlock->hdr.ElementNum = pParamReq->hdr.ElementNum;
	ETH_SET_BIGENDIAN_USHORT(&pBlock->hdr.ParamID,  pParamReq->hdr.ParamID);
	ETH_SET_BIGENDIAN_USHORT(&pBlock->hdr.SubIndex, pParamReq->hdr.SubIndex);

	//Set parameter block header elements
	pBlock->param_hdr.Format   = pParamReq->param_hdr.Format;
	pBlock->param_hdr.ValueNum = pParamReq->param_hdr.ValueNum;

	switch (Format)
	{
	case DATA_TYPE_BOOLEAN		: ValueSize = sizeof(UCHAR);   break;
	case DATA_TYPE_INT8			: ValueSize = sizeof(UCHAR);   break;
	case DATA_TYPE_INT16		: ValueSize = sizeof(USHORT);  break;
	case DATA_TYPE_INT32		: ValueSize = sizeof(ULONG);   break;
	case DATA_TYPE_INT64		: ValueSize = sizeof(ULONG64); break;
	case DATA_TYPE_UNSIGNED8	: ValueSize = sizeof(UCHAR);   break;	
	case DATA_TYPE_UNSIGNED16	: ValueSize = sizeof(USHORT);  break;
	case DATA_TYPE_UNSIGNED32	: ValueSize = sizeof(ULONG);   break;
	case DATA_TYPE_UNSIGNED64	: ValueSize = sizeof(ULONG64); break;
	case DATA_TYPE_FLOAT32		: ValueSize = sizeof(ULONG);   break;
	case DATA_TYPE_FLOAT64		: ValueSize = sizeof(ULONG64); break;
	case DATA_TYPE_OCTET_STR	: ValueSize = sizeof(UCHAR);   break;	
	case DATA_TYPE_UNICODE		: ValueSize = sizeof(USHORT);  break;
	case DATA_TYPE_DATE			: ValueSize = sizeof(GUID);    break;
	case DATA_TYPE_ERROR		: ValueSize = sizeof(USHORT);  break;
	default                     : ValueSize = sizeof(UCHAR);   break;
	};

	//Copy value block
	memcpy(&pBlock->u.Buffer, pParamReq->u.Buffer, ValueSize * ValueNum);

	//Change endian mode
	for (int i=0; i<ValueNum; i++)
	{
		switch (Format)
		{
		case DATA_TYPE_INT16		: ETH_SET_BIGENDIAN_USHORT (&pBlock->u.ValueBit16List[i], pBlock->u.ValueBit16List[i]);  break;
		case DATA_TYPE_INT32		: ETH_SET_BIGENDIAN_ULONG  (&pBlock->u.ValueBit32List[i], pBlock->u.ValueBit32List[i]);  break;
		case DATA_TYPE_INT64		: ETH_SET_BIGENDIAN_ULONG64(&pBlock->u.ValueBit64List[i], pBlock->u.ValueBit64List[i]);  break;
		case DATA_TYPE_UNSIGNED16	: ETH_SET_BIGENDIAN_USHORT (&pBlock->u.ValueBit16List[i], pBlock->u.ValueBit16List[i]);  break;
		case DATA_TYPE_UNSIGNED32	: ETH_SET_BIGENDIAN_ULONG  (&pBlock->u.ValueBit32List[i], pBlock->u.ValueBit32List[i]);  break;
		case DATA_TYPE_UNSIGNED64	: ETH_SET_BIGENDIAN_ULONG64(&pBlock->u.ValueBit64List[i], pBlock->u.ValueBit64List[i]);  break;
		case DATA_TYPE_FLOAT32		: ETH_SET_BIGENDIAN_ULONG  (&pBlock->u.ValueBit32List[i], pBlock->u.ValueBit32List[i]);  break;
		case DATA_TYPE_FLOAT64		: ETH_SET_BIGENDIAN_ULONG64(&pBlock->u.ValueBit64List[i], pBlock->u.ValueBit64List[i]);  break;
		case DATA_TYPE_ERROR		: ETH_SET_BIGENDIAN_USHORT (&pBlock->u.ValueBit16List[i], pBlock->u.ValueBit16List[i]);  break;
		};
	}

	//Return buffer length
	return (sizeof(PROFIDRIVE_PARAM_CHANGE_REQ::hdr) + 
			sizeof(PROFIDRIVE_PARAM_CHANGE_REQ::param_hdr) +
			ValueSize * ValueNum);
}



USHORT __SetProfiDriveChangeRequest(
							UCHAR ParamNum,
							PPROFIDRIVE_PARAM_CHANGE_REQ  pParamReqList,
							PUCHAR pBuffer)
{
	USHORT BufferOffs = 0;

	//Set parameter read request header
	BufferOffs +=  __SetProfiDriveRequestHeader(
										++__ProfiDriveReqRef,
										PROFIDRIVE_REQ_CHANGE,
										PROFIDRIVE_AXIS_DO_REPRESENT,
										ParamNum,
										&pBuffer[BufferOffs]);

	//Loop through all requested parameters
	for (UCHAR i=0; i<ParamNum; i++)
	{
		//Set parameter block
		BufferOffs += __SetProfiDriveChangeRequestParam(
												&pParamReqList[i],
												&pBuffer[BufferOffs]);
	}
		
	//Return request length	
	return BufferOffs;
}

//********************************************************************************
// Get response functions
//********************************************************************************

__inline USHORT __GetProfiDriveChangeResponseParam(
										PPROFIDRIVE_PARAM_CHANGE_RESP pParamResp,
										PUCHAR pBuffer)
{
	PPROFIDRIVE_PARAM_CHANGE_RESP pBlock = (PPROFIDRIVE_PARAM_CHANGE_RESP)pBuffer;
	UCHAR Format     = pBlock->param_hdr.Format;
	UCHAR ValueNum   = pBlock->param_hdr.ValueNum;

	//Get parameter block header elements
	pParamResp->param_hdr.Format   = pBlock->param_hdr.Format;
	pParamResp->param_hdr.ValueNum = pBlock->param_hdr.ValueNum;

	//Get all error values and change endian mode
	for (int i=0; i<ValueNum; i++)
		ETH_SET_BIGENDIAN_USHORT (&pParamResp->u.ValueBit16List[i], pBlock->u.ValueBit16List[i]);

	//Return buffer length
	return (sizeof(PROFIDRIVE_PARAM_READ_RESP::param_hdr) + sizeof(USHORT) * ValueNum);
}


USHORT __GetProfiDriveChangeResponse(
							PPROFIDRIVE_PARAM_CHANGE_RESP pParamRespList,
							PUCHAR pBuffer)
{
	USHORT BufferOffs = 0;
	UCHAR  ParamNum = 0;
	UCHAR  RespID = 0;

	//Get change request header
	BufferOffs += __GetProfiDriveResponseHeader(
											NULL,
											&RespID,
											NULL,
											&ParamNum,
											pBuffer);

	//Check for negative response and get error list
	if (RespID == PROFIDRIVE_RESP_CHANGE_NEGATIVE)
	{
		//Loop through all requested parameters
		for (UCHAR i=0; i<ParamNum; i++)
		{
			//Get parameter block
			BufferOffs += __GetProfiDriveChangeResponseParam(
														&pParamRespList[i],
														&pBuffer[BufferOffs]);
		}
	}

	//Return request length	
	return BufferOffs;
}


//********************************************************************************
// Export functions
//********************************************************************************

ULONG ProfiDriveChangeParams(
					PSTATION_INFO pStation,
					USHORT Slot,
					USHORT SubSlot,
					UCHAR ParamNum,
					PPROFIDRIVE_PARAM_CHANGE_REQ  pParamReqList,
					PPROFIDRIVE_PARAM_CHANGE_RESP pParamRespList)
{
	PUCHAR  pBuffer = (PUCHAR)malloc(MAX_ETH_FRAME_STD_SIZE);
	USHORT  BufferLen = 0;
	ULONG   Result = -1;

	//Check for valid record data buffer
	if (pBuffer)
	{
		//Reset record buffer
		memset(pBuffer, 0, sizeof(PROFIDRIVE_PARAM_CHANGE_REQ) * ParamNum);

		//Set parameter list
		BufferLen = __SetProfiDriveChangeRequest(
											ParamNum, 
											pParamReqList,
											pBuffer);

		//Send ProfiDrive write request
		Result = Pnio64ServiceWrite(
								pStation,					//Station pointer
								PROFIDRIVE_API,				//Api ID
								Slot,						//Slot number
								SubSlot,					//SubSlot Number
								INDEX_PROFIDRIVE_ACCESS,	//Record index
								BufferLen,					//Record length
								pBuffer);					//Record data

		if (ERROR_SUCCESS == Result)
		{
			//Send ProfiDrive read request
			Result = Pnio64ServiceRead(
									pStation,					//Station pointer
									PROFIDRIVE_API,				//Api ID
									Slot,						//Slot number
									SubSlot,					//SubSlot Number
									INDEX_PROFIDRIVE_ACCESS,	//Record index
									&BufferLen,					//Record length
									pBuffer);					//Record data

			if (ERROR_SUCCESS == Result)
			{
				//Get parameter list
				__GetProfiDriveChangeResponse(
										pParamRespList,
										pBuffer);
			}
		}

		//Release record data
		free(pBuffer);
	}

	//Something failed
	return Result;
}




