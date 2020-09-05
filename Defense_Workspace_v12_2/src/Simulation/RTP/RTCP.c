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
* Author:    Kumar Gaurav		                                                    *
*										                                            *
* ----------------------------------------------------------------------------------*/
#pragma region HEADER_FILE
#include "main.h"
#include "RTP.h"
#include "RTP_MSG.h"
#pragma endregion

#pragma region SUBEVENT_REGISTERTION
static ptrLTENR_SUBEVENT subEventDatabase[10];
void RTCP_SUBEVENT_REGISTER(rtcp_type_t type,
	char* name,
	void(*fun)())
{
	UINT index = type % (APP_PROTOCOL_RTP * 100);
	subEventDatabase[index] = calloc(1, sizeof * subEventDatabase[index]);
	subEventDatabase[index]->fnSubEventfunction = fun;
	subEventDatabase[index]->subEvent = type;
	subEventDatabase[index]->subeventName = _strdup(name);
}

char* RTCP_SUBEVNET_NAME(NETSIM_ID id)
{
	UINT index = id % (APP_PROTOCOL_RTP * 100);
	if (subEventDatabase[index])
		return subEventDatabase[index]->subeventName;
	else
		return "Unknown";
}

void RTCP_SUBEVENT_CALL()
{
	rtcp_type_t type = pstruEventDetails->nSubEventType;
	UINT index = type % (APP_PROTOCOL_RTP * 100);
	if (subEventDatabase[index] && subEventDatabase[index]->fnSubEventfunction)
		subEventDatabase[index]->fnSubEventfunction();
	else
	{
		fnNetSimError("RTP Callback function for subevnet id %d is not registered.\n",
			type);
	}
}
#pragma endregion

#pragma region RTCP_INIT_AND_HDR
void RTCP_INIT() {
	RTCP_SUBEVENT_REGISTER(RTCP_SUBEVENT_SR,"RTCP_SUBEVENT_SR",ScheduleRTCPSR);
	RTCP_SUBEVENT_REGISTER(RTCP_SUBEVENT_RR,"RTCP_SUBEVENT_RR", Schedule_RTCP_RR);
	RTCP_SUBEVENT_REGISTER(RTCP_SUBEVENT_SDES, "RTCP_SUBEVENT_SDES", Schedule_RTCP_SDE);
	RTCP_SUBEVENT_REGISTER(RTCP_SUBEVENT_APP, "RTCP_SUBEVENT_APP", NULL);
	RTCP_SUBEVENT_REGISTER(RTCP_SUBEVENT_BYE, "RTCP_SUBEVENT_BYE", Schedule_RTCP_BYE);
}

//based on type size should come 
void rtcp_hdr_add(ptrrtcp_t sr, RTCP_MSG_HDR_SIZE size) {
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	sr->appProtocol = packet->pstruAppData->nApplicationProtocol;
	sr->appData = packet->pstruAppData->Packet_AppProtocol;
	packet->pstruAppData->nApplicationProtocol = APP_PROTOCOL_RTP;
	packet->pstruAppData->Packet_AppProtocol = sr;
	packet->pstruAppData->dOverhead += (RTCP_MSG_HDR_SIZE_UINT[size] + RTCP_MSG_HDR_SIZE_UINT[RTCP_HDR_SIZE]);
	packet->pstruAppData->dPacketSize = packet->pstruAppData->dPayload +
		packet->pstruAppData->dOverhead;
}

void rtcp_hdr_remove(NetSim_PACKET* packet, RTCP_MSG_HDR_SIZE size)
{
	ptrrtcp_t h = packet->pstruAppData->Packet_AppProtocol;

	packet->pstruAppData->nApplicationProtocol = h->appProtocol;
	packet->pstruAppData->Packet_AppProtocol = h->appData;

	packet->pstruAppData->dOverhead -= (RTCP_MSG_HDR_SIZE_UINT[size] + RTCP_MSG_HDR_SIZE_UINT[RTCP_HDR_SIZE]);
	packet->pstruAppData->dPacketSize = packet->pstruAppData->dPayload +
		packet->pstruAppData->dOverhead;

	free(h);
}

void scheduleRTCP(ptrRTP_SESSION s) {
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	NETSIM_ID r = packet->nSourceId;//only for RR
	double time = pstruEventDetails->dEventTime;
	if (!IP_COMPARE(s->localAdderss, packet->pstruNetworkData->szSourceIP)) {
		scheduleRTCPSR(s);
		scheduleRTCPSDE(s);
	}
	else {
		scheduleRTCPRR(s, d, in, r, time);
		//RR msg 
	}
		
}

void* rtcp_hdr_copy(void* hdr)
{
	ptrrtcp_t h = hdr;
	ptrrtcp_t ret = calloc(1, sizeof * ret);
	memcpy(ret, h, sizeof * ret);
	return ret;
}

void rtcp_hdr_free(void* hdr)
{
	ptrrtcp_t h = hdr;
	free(h);
}
#pragma endregion

#pragma region RTCP_PACKET_CREATE
NetSim_PACKET* RTCP_PACKET_CREATE(NETSIM_ID src, NETSIM_ID dest,
	double time, rtcp_type_t type)
{
	NetSim_PACKET* packet = fn_NetSim_Packet_CreatePacket(APPLICATION_LAYER);
	packet->nSourceId = src;
	add_dest_to_packet(packet,dest);
	packet->dEventTime = time;
	packet->nControlDataType = type;
	packet->nPacketPriority = Priority_Low;
	packet->nPacketType = PacketType_Control;
	packet->nQOS = QOS_BE;
	strcpy(packet->szPacketType, strRTCP_MSGTYPE[type % 200]);
	return packet;
}

void RTCP_PACKET_INFO_ADD(ptrRTP_SESSION s) {
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	double time = pstruEventDetails->dEventTime;

	s->rtcpRemotePort = s->remotePort + 1;
	s->rtcpLocalPort = s->localPort + 1;
	packet->pstruNetworkData->szSourceIP = s->localAdderss;
	packet->pstruNetworkData->szDestIP = s->remoteAddress;
	packet->pstruNetworkData->nTTL = MAX_TTL;
	packet->pstruTransportData->nSourcePort = s->rtcpLocalPort;
	packet->pstruTransportData->nDestinationPort = s->rtcpRemotePort;

	packet->pstruAppData->dArrivalTime = time;
	packet->pstruAppData->dEndTime = time;
	packet->pstruAppData->dStartTime = time;

	ptrSOCKETINTERFACE socket = fn_NetSim_Socket_GetSocketInterface(d, APP_PROTOCOL_RTP, TX_PROTOCOL_UDP, s->rtcpLocalPort, s->rtcpRemotePort);
	if (!socket)
		socket = fn_NetSim_Socket_CreateNewSocket(d, APP_PROTOCOL_RTP, TX_PROTOCOL_UDP, s->rtcpLocalPort, s->rtcpRemotePort);
	pstruEventDetails->szOtherDetails = socket;
}
#pragma endregion

#pragma region RTCP_SR
void scheduleRTCPSR(ptrRTP_SESSION s) {
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = pstruEventDetails->dEventTime;
	pevent.nDeviceId = pstruEventDetails->nDeviceId;
	pevent.nDeviceType = DEVICE_TYPE(pstruEventDetails->nDeviceId);
	pevent.nEventType = TIMER_EVENT;
	pevent.nInterfaceId = pstruEventDetails->nInterfaceId;
	pevent.nProtocolId = APP_PROTOCOL_RTP;
	pevent.nSubEventType = RTCP_SUBEVENT_SR;
	pevent.pPacket = RTCP_PACKET_CREATE(pstruEventDetails->nDeviceId, get_first_dest_from_packet(pstruEventDetails->pPacket), pstruEventDetails->dEventTime ,RTCP_SR);
	pevent.szOtherDetails = s;
	fnpAddEvent(&pevent);
	
}

void ScheduleRTCPSR() {

	NETSIM_ID d = pstruEventDetails->nDeviceId;
	double time = pstruEventDetails->dEventTime;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	ptrRTP_SESSION s = pstruEventDetails->szOtherDetails;

	pstruEventDetails->dEventTime += RTCP_PACKET_GENERATION;
	scheduleRTCPSR(s);
	pstruEventDetails->dEventTime -= RTCP_PACKET_GENERATION;
	
	ptrrtcp_t sr = calloc(1, sizeof * sr);
	sr->hdr.version = RTCP_VERSION;
	sr->hdr.pt = RTCP_SR;
	sr->hdr.length = RTCP_SR_SIZE;
	sr->hdr.p = 0;
	sr->hdr.count = 1;

	sr->r.sr.rtp_ts = (UINT)time;
	sr->r.sr.ssrc = s->ssrc;
	sr->r.sr.osent = s->txStat->osent;
	sr->r.sr.psent = s->txStat->psent;

	rtcp_hdr_add(sr,RTCP_SR_SIZE);
	RTCP_PACKET_INFO_ADD(s);
	send_to_transport(d, pstruEventDetails->szOtherDetails, packet, time);
}

void ScheduleRTCPSRRECV() {
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	rtcp_hdr_remove(pstruEventDetails->pPacket, RTCP_SR_SIZE);	
}
#pragma endregion

#pragma region RTCP_RR
void scheduleRTCPRR(ptrRTP_SESSION s,NETSIM_ID d,NETSIM_ID in, NETSIM_ID r,double time) {
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = time;
	pevent.nDeviceId = d;
	pevent.nDeviceType = DEVICE_TYPE(d);
	pevent.nEventType = TIMER_EVENT;
	pevent.nInterfaceId = in;
	pevent.nProtocolId = APP_PROTOCOL_RTP;
	pevent.nSubEventType = RTCP_SUBEVENT_RR;
	pevent.pPacket = RTCP_PACKET_CREATE(d, r, time, RTCP_RR);
	pevent.szOtherDetails = s;
	fnpAddEvent(&pevent);

}

void Schedule_RTCP_RR() {

	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	double time = pstruEventDetails->dEventTime;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	NETSIM_ID r = get_first_dest_from_packet(packet);
	ptrRTP_SESSION s = pstruEventDetails->szOtherDetails;

	scheduleRTCPRR(s, d, in, r, time+ RTCP_PACKET_GENERATION);

	ptrrtcp_t sr = calloc(1, sizeof * sr);
	sr->hdr.version = RTCP_VERSION;
	sr->hdr.pt = RTCP_RR;
	sr->hdr.length = RTCP_RR_SIZE;
	sr->hdr.p = 0;
	sr->hdr.count = 1;

	sr->r.rr.ssrc = s->ssrc;
	sr->r.rr.rrv = calloc(1, sizeof * sr->r.rr.rrv);
	sr->r.rr.rrv->ssrc = s->ssrc;
	sr->r.rr.rrv->lost = source_calc_lost(s->source);
	sr->r.rr.rrv->fraction = source_calc_fraction_lost(s->source);
	sr->r.rr.rrv->jitter = s->source->jitter;

	rtcp_hdr_add(sr, RTCP_RR_SIZE);
	RTCP_PACKET_INFO_ADD(s);
	send_to_transport(d, pstruEventDetails->szOtherDetails, packet, time);
}

void Schedule_RTCP_RR_RECV() {
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	rtcp_hdr_remove(pstruEventDetails->pPacket, RTCP_RR_SIZE);
}
#pragma endregion

#pragma region RTCP_SDE
void scheduleRTCPSDE(ptrRTP_SESSION s) {
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = pstruEventDetails->dEventTime;
	pevent.nDeviceId = pstruEventDetails->nDeviceId;
	pevent.nDeviceType = DEVICE_TYPE(pstruEventDetails->nDeviceId);
	pevent.nEventType = TIMER_EVENT;
	pevent.nInterfaceId = pstruEventDetails->nInterfaceId;
	pevent.nProtocolId = APP_PROTOCOL_RTP;
	pevent.nSubEventType = RTCP_SUBEVENT_SDES;
	pevent.pPacket = RTCP_PACKET_CREATE(pstruEventDetails->nDeviceId, get_first_dest_from_packet(pstruEventDetails->pPacket), pstruEventDetails->dEventTime, RTCP_SDES);
	pevent.szOtherDetails = s;
	fnpAddEvent(&pevent);

}

void Schedule_RTCP_SDE() {

	NETSIM_ID d = pstruEventDetails->nDeviceId;
	double time = pstruEventDetails->dEventTime;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	ptrRTP_SESSION s = pstruEventDetails->szOtherDetails;

	pstruEventDetails->dEventTime += RTCP_PACKET_GENERATION;
	scheduleRTCPSDE(s);
	pstruEventDetails->dEventTime -= RTCP_PACKET_GENERATION;

	ptrrtcp_t sr = calloc(1, sizeof * sr);
	sr->hdr.version = RTCP_VERSION;
	sr->hdr.pt = RTCP_SDES;
	sr->hdr.length = RTCP_MAX_SDES;
	sr->hdr.p = 0;
	sr->hdr.count = 1;

	sr->r.sdesv = calloc(1, sizeof * sr->r.sdesv);
	sr->r.sdesv->itemv = calloc(1, sizeof * sr->r.sdesv->itemv);
	sr->r.sdesv->src = s->ssrc;
	sr->r.sdesv->itemv->type = RTCP_SDES_CNAME;
	sr->r.sdesv->itemv->data = DEVICE_NAME(get_first_dest_from_packet(packet));
	sr->r.sdesv->itemv->length = 4;

	rtcp_hdr_add(sr, RTCP_MAX_SDES);
	RTCP_PACKET_INFO_ADD(s);
	send_to_transport(d, pstruEventDetails->szOtherDetails, packet, time);
}

void Schedule_RTCP_SDE_RECV() {
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	rtcp_hdr_remove(pstruEventDetails->pPacket, RTCP_MAX_SDES);
}
#pragma endregion

#pragma region RTCP_BYE
void scheduleRTCPBYE(ptrRTP_SESSION s) {
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = pstruEventDetails->dEventTime;
	pevent.nDeviceId = pstruEventDetails->nDeviceId;
	pevent.nDeviceType = DEVICE_TYPE(pstruEventDetails->nDeviceId);
	pevent.nEventType = TIMER_EVENT;
	pevent.nInterfaceId = pstruEventDetails->nInterfaceId;
	pevent.nProtocolId = APP_PROTOCOL_RTP;
	pevent.nSubEventType = RTCP_SUBEVENT_BYE;
	pevent.pPacket = RTCP_PACKET_CREATE(pstruEventDetails->nDeviceId, get_first_dest_from_packet(pstruEventDetails->pPacket), pstruEventDetails->dEventTime, RTCP_BYE);
	pevent.szOtherDetails = s;
	fnpAddEvent(&pevent);

}

void Schedule_RTCP_BYE() {

	NETSIM_ID d = pstruEventDetails->nDeviceId;
	double time = pstruEventDetails->dEventTime;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	ptrRTP_SESSION s = pstruEventDetails->szOtherDetails;

	pstruEventDetails->dEventTime += RTCP_PACKET_GENERATION;
	scheduleRTCPBYE(s);
	pstruEventDetails->dEventTime -= RTCP_PACKET_GENERATION;

	ptrrtcp_t sr = calloc(1, sizeof * sr);
	sr->hdr.version = RTCP_VERSION;
	sr->hdr.pt = RTCP_BYE;
	sr->hdr.length = RTCP_MAX_BYE;
	sr->hdr.p = 0;
	sr->hdr.count = 1;

	sr->r.bye.srcv = d;

	rtcp_hdr_add(sr, RTCP_MAX_BYE);
	RTCP_PACKET_INFO_ADD(s);
	send_to_transport(d, pstruEventDetails->szOtherDetails, packet, time);
}

void Schedule_RTCP_BYE_RECV() {
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	rtcp_hdr_remove(pstruEventDetails->pPacket, RTCP_MAX_BYE);
}
#pragma endregion
