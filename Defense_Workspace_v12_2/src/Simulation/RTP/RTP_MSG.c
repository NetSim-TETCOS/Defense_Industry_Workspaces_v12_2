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
#pragma region HEADER_FILE
#include "main.h"
#include "RTP.h"
#include "RTP_MSG.h"
#pragma endregion

#pragma region RTP_HDR_ADD_AND_CREATE
static ptrRTPHDR_FIXED rtp_hdr_new(ptrRTP_SESSION s)
{
	ptrRTPHDR_FIXED h = calloc(1, sizeof * h);
	h->V = RTP_VERSION;
	h->PT = 8;
	h->sequenceNumber = ++(s->seqNumber);
	h->SSRCIndentifier = s->ssrc;
	h->timeStamp = (UINT)pstruEventDetails->dEventTime;
	return h;
}

void rtp_hdr_add(ptrRTP_SESSION s, NetSim_PACKET* packet)
{
	ptrRTPHDR_FIXED h = rtp_hdr_new(s);

	h->appProtocol = packet->pstruAppData->nApplicationProtocol;
	h->appData = packet->pstruAppData->Packet_AppProtocol;
	packet->pstruAppData->nApplicationProtocol = APP_PROTOCOL_RTP;
	packet->pstruAppData->Packet_AppProtocol = h;
	packet->pstruAppData->dOverhead += RTPHDR_FIXED_LEN(0);
	packet->pstruAppData->dPacketSize = packet->pstruAppData->dPayload +
		packet->pstruAppData->dOverhead;
}

void* rtp_hdr_copy(void* hdr)
{
	ptrRTPHDR_FIXED h = hdr;
	ptrRTPHDR_FIXED ret = calloc(1, sizeof * ret);
	memcpy(ret, h, sizeof * ret);
	return ret;
}

void rtp_hdr_free(void* hdr)
{
	ptrRTPHDR_FIXED h = hdr;
	free(h);
}
#pragma endregion

#pragma region GET_SEQ_AND_SSRC
UINT16 getSeq(NetSim_PACKET* packet)
{
	ptrRTPHDR_FIXED h = packet->pstruAppData->Packet_AppProtocol;
	return h->sequenceNumber;
}

UINT getSSRC(NetSim_PACKET* packet)
{
	ptrRTPHDR_FIXED h = packet->pstruAppData->Packet_AppProtocol;
	return h->SSRCIndentifier;
}
#pragma endregion

#pragma region RTP_HDR_REMOVE
void rtp_hdr_remove(ptrRTP_SESSION s, NetSim_PACKET* packet)
{
	ptrRTPHDR_FIXED h = packet->pstruAppData->Packet_AppProtocol;

	packet->pstruAppData->nApplicationProtocol = h->appProtocol;
	packet->pstruAppData->Packet_AppProtocol = h->appData;

	packet->pstruAppData->dOverhead -= RTPHDR_FIXED_LEN(0);
	packet->pstruAppData->dPacketSize = packet->pstruAppData->dPayload +
		packet->pstruAppData->dOverhead;

	free(h);
}
#pragma endregion
