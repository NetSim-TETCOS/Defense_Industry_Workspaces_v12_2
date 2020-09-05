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

#pragma region SESSION_FIND_AND_CREATE
ptrRTP_SESSION find_rtp_session(ptrRTPVAR var, 
								NETSIM_IPAddress localAddress, NETSIM_IPAddress remoteAddress,
								UINT16 localPort,UINT16 remotePort)
{
	UINT i;
	for (i = 0; i < var->rtpSessionCount; i++)
	{
		if (!IP_COMPARE(var->rtpSessions[i]->localAdderss, localAddress) &&
			!IP_COMPARE(var->rtpSessions[i]->remoteAddress, remoteAddress) &&
			var->rtpSessions[i]->localPort == localPort &&
			var->rtpSessions[i]->remotePort == remotePort)
			return var->rtpSessions[i];
	}
	return NULL;
}

ptrRTP_SESSION create_rtp_session(ptrRTPVAR var,
								  NETSIM_IPAddress localAddress, NETSIM_IPAddress remoteAddress,
								  UINT16 localPort, UINT16 remotePort)
{
	ptrRTP_SESSION s = calloc(1, sizeof * s);
	s->localAdderss = IP_COPY(localAddress);
	s->localPort = localPort;
	s->remoteAddress = IP_COPY(remoteAddress);
	s->remotePort = remotePort;
	
	if (var->rtpSessionCount)
		var->rtpSessions = realloc(var->rtpSessions, var->rtpSessionCount * sizeof * var->rtpSessions);
	else
		var->rtpSessions = calloc(1, sizeof * var->rtpSessions);
	var->rtpSessions[var->rtpSessionCount] = s;
	var->rtpSessionCount++;
	return s;
}
#pragma endregion

#pragma region INIT_RTP_SESSION
/**
   Upon joining the session, the participant initializes tp to 0, tc to
   0, senders to 0, pmembers to 1, members to 1, we_sent to false,
   rtcp_bw to the specified fraction of the session bandwidth, initial
   to true, and avg_rtcp_size to the probable size of the first RTCP
   packet that the application will later construct.  The calculated
   interval T is then computed, and the first packet is scheduled for
   time tn = T.  This means that a transmission timer is set which
   expires at time T.  Note that an application MAY use any desired
   approach for implementing this timer.

   The participant adds its own SSRC to the member table.
*/
static void init_rtp_session(ptrRTP_SESSION s)
{
	s->tp = 0;
	s->tc = 0;
	s->senders = 0;
	s->pmembers = 1;
	s->members = 1;
	s->we_sent = false;
	s->initial = true;
	s->seqNumber = 0;
	s->ssrc = (UINT32)NETSIM_RAND();
	s->txStat = calloc(1, sizeof * s->txStat);
	s->txStat->osent = 0;
	s->txStat->psent = 0;
}
#pragma endregion

#pragma region SESSION_EXIST_FUN
ptrRTP_SESSION find_or_create_rtp_session(ptrRTPVAR var,
										  NETSIM_IPAddress localAddress, NETSIM_IPAddress remoteAddress,
										  UINT16 localPort, UINT16 remotePort)
{
	ptrRTP_SESSION s = find_rtp_session(var, localAddress, remoteAddress, localPort, remotePort);
	if (!s)
	{
		s = create_rtp_session(var, localAddress, remoteAddress, localPort, remotePort);
		init_rtp_session(s);
		rtcp_start(s);
	}
	return s;
}

int is_rtp_session_exist(ptrRTPVAR var,
	NETSIM_IPAddress localAddress, NETSIM_IPAddress remoteAddress,
	UINT16 localPort, UINT16 remotePort)
{
	ptrRTP_SESSION s = find_rtp_session(var, localAddress, remoteAddress, localPort, remotePort);
	if (!s)
	{
		return 0;
	}
	return 1;
}
#pragma endregion

#pragma region RTCP_START
void rtcp_start(ptrRTP_SESSION s) {
	if (!s)
		return;
	if (s->remoteAddress != NULL) {
		scheduleRTCP(s);
	}
}
#pragma endregion
