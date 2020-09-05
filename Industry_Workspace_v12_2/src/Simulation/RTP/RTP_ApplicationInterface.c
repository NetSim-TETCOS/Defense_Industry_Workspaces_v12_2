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
#pragma endregion

#pragma region TRANSPORT_APPLICATION_HANDLER
void send_to_transport(NETSIM_ID d, ptrSOCKETINTERFACE s,
							  NetSim_PACKET* packet, double time)
{
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);

	packet->pstruTransportData->nTransportProtocol = s->trxProtocol;
	if (!fn_NetSim_Socket_GetBufferStatus(s))
	{
		//Socket buffer is NULL
		//Create event for transport out
		pevent.dEventTime = time;
		pevent.dPacketSize = packet->pstruAppData->dPacketSize;
		pevent.nApplicationId = packet->pstruAppData->nApplicationId;
		pevent.nDeviceId = d;
		pevent.nDeviceType = DEVICE_TYPE(d);
		pevent.nEventType = TRANSPORT_OUT_EVENT;
		pevent.nPacketId = packet->nPacketId;
		pevent.nProtocolId = packet->pstruTransportData->nTransportProtocol;
		pevent.nSegmentId = packet->pstruAppData->nSegmentId;
		pevent.szOtherDetails = s;
		fnpAddEvent(&pevent);
	}

	//Place the packet to socket buffer
	fn_NetSim_Socket_PassPacketToInterface(d, packet, s);
}

static void send_to_application(NETSIM_ID d, NetSim_PACKET* packet, double time)
{
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);

	pevent.dEventTime = time;
	pevent.dPacketSize = packet->pstruAppData->dPacketSize;
	pevent.nApplicationId = packet->pstruAppData->nApplicationId;
	pevent.nDeviceId = d;
	pevent.nDeviceType = DEVICE_TYPE(d);
	pevent.nEventType = APPLICATION_IN_EVENT;
	pevent.nPacketId = packet->nPacketId;
	pevent.nProtocolId = packet->pstruAppData->nApplicationProtocol;
	pevent.nSegmentId = packet->pstruAppData->nSegmentId;
	pevent.pPacket = packet;
	fnpAddEvent(&pevent);
}
#pragma endregion

#pragma region RTP_APP_OUT_AND_APP_IN
void rtp_handle_app_out()
{
	double time = pstruEventDetails->dEventTime;
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	ptrSOCKETINTERFACE s = pstruEventDetails->szOtherDetails;

	ptrRTPVAR var = RTPVAR_GET(d);
	if (!var)
	{
		fnNetSimError("RTP protocol is not initiallized for device %d\n", DEVICE_CONFIGID(d));
		return;
	}
	ptrRTP_SESSION session = find_or_create_rtp_session(RTPVAR_GET(d),
														packet->pstruNetworkData->szSourceIP,
														packet->pstruNetworkData->szDestIP,
														packet->pstruTransportData->nSourcePort,
														packet->pstruTransportData->nDestinationPort);

	rtp_hdr_add(session, packet);
	session->txStat->osent += (UINT32)packet->pstruAppData->dPayload;
	session->txStat->psent += 1;
#pragma message(__LOC__"Change below code")
	send_to_transport(d, s, packet, time);
}

void rtp_handle_app_in()
{
	double time = pstruEventDetails->dEventTime;
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;

	ptrRTPVAR var = RTPVAR_GET(d);
	if (!var)
	{
		fnNetSimError("RTP protocol is not initiallized for device %d\n", DEVICE_CONFIGID(d));
		return;
	}
	
	//define source init here if it is first packet other wise call source_update_seq from source file
	//calc jitter here and lost packet also and it will create RR packet
	ptrRTP_SESSION session = find_or_create_rtp_session(RTPVAR_GET(d),
														DEVICE_NWADDRESS(d,1),
														isMulticastIP(packet->pstruNetworkData->szDestIP)? packet->pstruNetworkData->szDestIP: packet->pstruNetworkData->szSourceIP,
														packet->pstruTransportData->nSourcePort,
														packet->pstruTransportData->nDestinationPort);
	if (!session->source) {
		session->source = calloc(1, sizeof * session->source);
		source_init_seq(session->source, getSeq(packet));
	}
	if (!source_update_seq(session->source, getSeq(packet))) {
		fnNetSimError("RTP_Source.c : rtp_update_seq() returned 0\n");
	}

	source_calc_jitter(session->source, packet->pstruAppData->dStartTime, time);
	int Packet_loss = source_calc_lost(session->source);
	UINT8 packet_frac_loss = source_calc_fraction_lost(session->source);

	print_rtp_log("Packet_Id: %d Seq No: %d SSRC: %d Packet loss: %d packet frac loss: %d Jitter: %d\n",
		packet->nPacketId, getSeq(packet), getSSRC(packet),Packet_loss, packet_frac_loss, session->source->jitter);

	rtp_hdr_remove(session, packet);
	send_to_application(d, packet, time);
}
#pragma endregion
