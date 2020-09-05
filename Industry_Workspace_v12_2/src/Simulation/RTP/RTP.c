/************************************************************************************
* Copyright (C) 2020																*
* TETCOS, Bangalore. India															*
*																					*
* Tetcos owns the intellectual property rights in the Product and its content.		*
* The copying, redistribution, reselling or publication of any or all of the		*
* Product or its content without express prior written consent of Tetcos is			*
* prohibited. Ownership and / or any other right relating to the software and all	*
* intellectual property rights therein shall remain at all times with Tetcos.		*
*																					*
* This source code is licensed per the NetSim license agreement.					*
*																					*
* No portion of this source code may be used as the basis for a derivative work,	*
* or used, for any purpose other than its intended use per the NetSim license		*
* agreement.																		*
*																					*
* This source code and the algorithms contained within it are confidential trade	*
* secrets of TETCOS and may not be used as the basis for any other software,		*
* hardware, product or service.														*
*																					*
* Author:    Shashi Kant Suman	                                                    *
*										                                            *
* ----------------------------------------------------------------------------------*/
#include "main.h"
#include "RTP.h"

_declspec(dllexport) int fn_NetSim_RTP_Init()
{
	init_rtp_log();
	RTCP_INIT();
	return 0;
}

_declspec(dllexport) int fn_NetSim_RTP_Run()
{
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	if (pstruEventDetails->nSubEventType)
	{
		RTCP_SUBEVENT_CALL();
		return 0;
	}
	switch (pstruEventDetails->nEventType)
	{
		case APPLICATION_OUT_EVENT:
			rtp_handle_app_out();
			break;
		case APPLICATION_IN_EVENT:
		{
			switch (pstruEventDetails->pPacket->nControlDataType) {
				case RTCP_SR:
					ScheduleRTCPSRRECV();
					break;
				case RTCP_RR:
					Schedule_RTCP_RR_RECV();
					break;
				case RTCP_SDES:
					Schedule_RTCP_SDE_RECV();
					break;
				case RTCP_BYE:
					Schedule_RTCP_BYE_RECV();
					break;
				default:
					rtp_handle_app_in();
					break;
			}
		}
		break;
		default:
			fnNetSimError("Unknown event type %d for RTP protocol\n", pstruEventDetails->nEventType);
			break;
		
	}
	return 0;
}

_declspec(dllexport) char* fn_NetSim_RTP_Trace(NETSIM_ID id)
{
	return "";
}

_declspec(dllexport) int fn_NetSim_RTP_FreePacket(NetSim_PACKET* packet)
{
	if(packet->nPacketType == PacketType_Control)
		rtcp_hdr_free(packet->pstruAppData->Packet_AppProtocol);
	else
		rtp_hdr_free(packet->pstruAppData->Packet_AppProtocol);
	packet->pstruAppData->Packet_AppProtocol = NULL;
	packet->pstruAppData->nApplicationProtocol = 0;
	return 0;
}

_declspec(dllexport) int fn_NetSim_RTP_CopyPacket(NetSim_PACKET* destPacket, const NetSim_PACKET* srcPacket)
{
	if(srcPacket->nPacketType == PacketType_Control)
		destPacket->pstruAppData->Packet_AppProtocol = rtcp_hdr_copy(srcPacket->pstruAppData->Packet_AppProtocol);
	else
		destPacket->pstruAppData->Packet_AppProtocol = rtp_hdr_copy(srcPacket->pstruAppData->Packet_AppProtocol);
	return 0;
}

_declspec(dllexport) int fn_NetSim_RTP_Metrics(PMETRICSWRITER file)
{
	return 0;
}

_declspec(dllexport) int fn_NetSim_RTP_Configure(void** var)
{
	char* tag;
	void* xmlNetSimNode;
	NETSIM_ID nDeviceId;
	LAYER_TYPE nLayerType;

	tag = (char*)var[0];
	xmlNetSimNode = var[2];
	if (!strcmp(tag, "PROTOCOL_PROPERTY"))
	{
		NETWORK = (struct stru_NetSim_Network*)var[1];
		nDeviceId = *((NETSIM_ID*)var[3]);
		nLayerType = *((LAYER_TYPE*)var[5]);
		switch (nLayerType)
		{
			case APPLICATION_LAYER:
			{
				ptrRTPVAR var = DEVICE_APPVAR(nDeviceId, APP_PROTOCOL_RTP);
				if (!var)
				{
					var = calloc(1, sizeof * var);
					fn_NetSim_Stack_RegisterNewApplicationProtocol(nDeviceId, APP_PROTOCOL_RTP);
					fn_NetSim_Stack_SetAppProtocolData(nDeviceId, APP_PROTOCOL_RTP, var);
				}
			}
		}
	}
	return 0;
}

_declspec(dllexport) char* fn_NetSim_RTP_ConfigPacketTrace(void* xmlNetSimNode)
{
	return "";
}

_declspec(dllexport) int fn_NetSim_RTP_Finish()
{
	close_rtp_log();
	return 0;
}

_declspec(dllexport) int fn_NetSim_RTP_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return 0;
}