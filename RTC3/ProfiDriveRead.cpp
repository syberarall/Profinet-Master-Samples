//*****************************************************************
//	This code is strictly reserved by SYBERA GmbH
//  Copyright (c) 2025 SYBERA
//*****************************************************************

#pragma warning(disable:4748)

#include "ProfiDriveGlobal.h"
#include "ProfiDriveRead.h"


//Declare some globals
UCHAR __ProfiDriveReqRef = 0;


//********************************************************************************
// Set Request functions
//********************************************************************************

__inline USHORT __SetProfiDriveRequestHeader(
										UCHAR ReqRef,
										UCHAR ReqID,
										UCHAR DriveObj,
										UCHAR ParamNum,
										PUCHAR pBuffer)
{
	PPROFIDRIVE_REQ_HDR pBlock = (PPROFIDRIVE_REQ_HDR)pBuffer;

	//Set request header elements
	pBlock->ReqRef   = ReqRef;
	pBlock->ReqID	  =	ReqID; 
	pBlock->DriveObj =	DriveObj;
	pBlock->ParamNum = ParamNum;

	//Return data length
	return sizeof(PROFIDRIVE_REQ_HDR);
}


__inline USHORT __SetProfiDriveReadRequestParam(
											PPROFIDRIVE_PARAM_READ_REQ pParamReq,
											PUCHAR pBuffer)
{
	PPROFIDRIVE_PARAM_READ_REQ pBlock = (PPROFIDRIVE_PARAM_READ_REQ)pBuffer;

	//Set parameter block elements
	pBlock->param_hdr.Attr       = pParamReq->param_hdr.Attr;
	pBlock->param_hdr.ElementNum = pParamReq->param_hdr.ElementNum;
	ETH_SET_BIGENDIAN_USHORT(&pBlock->param_hdr.ParamID,  pParamReq->param_hdr.ParamID);
	ETH_SET_BIGENDIAN_USHORT(&pBlock->param_hdr.SubIndex, pParamReq->param_hdr.SubIndex);

	//Return data length
	return sizeof(PROFIDRIVE_PARAM_READ_REQ);
}


USHORT __SetProfiDriveReadRequest(
							UCHAR ParamNum,
							PPROFIDRIVE_PARAM_READ_REQ  pParamReqList,
							PUCHAR pBuffer)
{
	USHORT BufferOffs = 0;

	//Set parameter read request header
	BufferOffs +=  __SetProfiDriveRequestHeader(
										++__ProfiDriveReqRef,
										PROFIDRIVE_REQ_READ,
										PROFIDRIVE_AXIS_DO_REPRESENT,
										ParamNum,
										&pBuffer[BufferOffs]);

	//Loop through all requested parameters
	for (UCHAR i=0; i<ParamNum; i++)
	{
		//Set parameter block
		BufferOffs += __SetProfiDriveReadRequestParam(
												&pParamReqList[i],
												&pBuffer[BufferOffs]);
	}
		
	//Return request length	
	return BufferOffs;
}

//********************************************************************************
// Get Response functions
//********************************************************************************

__inline USHORT __GetProfiDriveResponseHeader(
										PUCHAR pReqRef,
										PUCHAR pRespID,
										PUCHAR pDriveObj,
										PUCHAR pParamNum,
										PUCHAR pBuffer)
{
	PPROFIDRIVE_RESP_HDR pBlock = (PPROFIDRIVE_RESP_HDR)pBuffer;

	//Set request header elements
	if (pReqRef)	{ *pReqRef   = pBlock->ReqRef; }
	if (pRespID)	{ *pRespID   = pBlock->RespID; }
	if (pDriveObj)	{ *pDriveObj = pBlock->DriveObj; }
	if (pParamNum)	{ *pParamNum = pBlock->ParamNum; }

	//Return data length
	return sizeof(PROFIDRIVE_RESP_HDR);
}


__inline USHORT __GetProfiDriveReadResponseParam(
										PPROFIDRIVE_PARAM_READ_RESP pParamResp,
										PUCHAR pBuffer)
{
	PPROFIDRIVE_PARAM_READ_RESP pBlock = (PPROFIDRIVE_PARAM_READ_RESP)pBuffer;
	UCHAR Format     = pBlock->param_hdr.Format;
	UCHAR ValueNum   = pBlock->param_hdr.ValueNum;
	USHORT ValueSize = 0;

	//Get parameter block header elements
	pParamResp->param_hdr.Format   = pBlock->param_hdr.Format;
	pParamResp->param_hdr.ValueNum = pBlock->param_hdr.ValueNum;

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
	memcpy(pParamResp->u.Buffer, pBlock->u.Buffer, ValueSize * ValueNum);

	//Change endian mode
	for (int i=0; i<ValueNum; i++)
	{
		switch (Format)
		{
		case DATA_TYPE_INT16		: ETH_SET_BIGENDIAN_USHORT (&pParamResp->u.ValueBit16List[i], pParamResp->u.ValueBit16List[i]);  break;
		case DATA_TYPE_INT32		: ETH_SET_BIGENDIAN_ULONG  (&pParamResp->u.ValueBit32List[i], pParamResp->u.ValueBit32List[i]);  break;
		case DATA_TYPE_INT64		: ETH_SET_BIGENDIAN_ULONG64(&pParamResp->u.ValueBit64List[i], pParamResp->u.ValueBit64List[i]);  break;
		case DATA_TYPE_UNSIGNED16	: ETH_SET_BIGENDIAN_USHORT (&pParamResp->u.ValueBit16List[i], pParamResp->u.ValueBit16List[i]);  break;
		case DATA_TYPE_UNSIGNED32	: ETH_SET_BIGENDIAN_ULONG  (&pParamResp->u.ValueBit32List[i], pParamResp->u.ValueBit32List[i]);  break;
		case DATA_TYPE_UNSIGNED64	: ETH_SET_BIGENDIAN_ULONG64(&pParamResp->u.ValueBit64List[i], pParamResp->u.ValueBit64List[i]);  break;
		case DATA_TYPE_FLOAT32		: ETH_SET_BIGENDIAN_ULONG  (&pParamResp->u.ValueBit32List[i], pParamResp->u.ValueBit32List[i]);  break;
		case DATA_TYPE_FLOAT64		: ETH_SET_BIGENDIAN_ULONG64(&pParamResp->u.ValueBit64List[i], pParamResp->u.ValueBit64List[i]);  break;
		case DATA_TYPE_ERROR		: ETH_SET_BIGENDIAN_USHORT (&pParamResp->u.ValueBit16List[i], pParamResp->u.ValueBit16List[i]);  break;
		};
	}

	//Return buffer length
	return (sizeof(PROFIDRIVE_PARAM_READ_RESP::param_hdr) + ValueSize * ValueNum);
}


USHORT __GetProfiDriveReadResponse(
							PPROFIDRIVE_PARAM_READ_RESP pParamRespList,
							PUCHAR pBuffer)
{
	USHORT BufferOffs = 0;
	UCHAR  ParamNum = 0;

	//Get read request header
	BufferOffs += __GetProfiDriveResponseHeader(
											NULL,
											NULL,
											NULL,
											&ParamNum,
											pBuffer);

	//Loop through all requested parameters
	for (UCHAR i=0; i<ParamNum; i++)
	{
		//Get parameter block
		BufferOffs += __GetProfiDriveReadResponseParam(
												&pParamRespList[i],
												&pBuffer[BufferOffs]);
	}
		
	//Return request length	
	return BufferOffs;
}

//********************************************************************************
// Export functions
//********************************************************************************

ULONG ProfiDriveReadParams(
					PSTATION_INFO pStation,
					USHORT Slot,
					USHORT SubSlot,
					UCHAR ParamNum,
					PPROFIDRIVE_PARAM_READ_REQ  pParamReqList,
					PPROFIDRIVE_PARAM_READ_RESP pParamRespList)
{
	PUCHAR  pBuffer = (PUCHAR)malloc(MAX_ETH_FRAME_STD_SIZE);
	USHORT  BufferLen = 0;
	ULONG   Result = -1;

	//Check for valid record data buffer
	if (pBuffer)
	{
		//Reset record buffer
		memset(pBuffer, 0, sizeof(PROFIDRIVE_PARAM_READ_RESP) * ParamNum);

		//Set parameter list
		BufferLen = __SetProfiDriveReadRequest(
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
				__GetProfiDriveReadResponse(
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
