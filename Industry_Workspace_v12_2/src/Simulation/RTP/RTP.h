#pragma once
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
//RFC
//https://tools.ietf.org/html/rfc3550

#ifndef _NETSIM_RTP_H_
#define _NETSIM_RTP_H_

#include "RTP_MSG.h"
#ifdef  __cplusplus
extern "C" {
#endif

#pragma comment(lib,"Metrics.lib")
#pragma comment(lib,"NetworkStack.lib")

#define RTP_VERSION		2
#define RTCP_VERSION	2
#define RTP_SEQ_MOD		(1<<16)
#define RTP_MAX_SDES	255      /* maximum text length for SDES */
#define RTCP_PACKET_GENERATION 1*SECOND

	typedef struct stru_rtp_txStat {
		UINT32 psent;
		UINT32 osent;
	}rtp_txStat,*ptrrtp_txStat;

	typedef struct rtp_source {
		NETSIM_IPAddress localAdderss;
		NETSIM_IPAddress remoteAddress;
		UINT16 localPort;
		UINT16 remotePort;

		UINT16 rtcpLocalPort;
		UINT16 rtcpRemotePort;
		UINT16 max_seq;
		UINT32 cycles;
		UINT32 base_seq;
		UINT32 bad_seq;
		UINT32 probation;
		UINT32 received;
		UINT32 expected_prior;
		UINT32 received_prior;
		int transit;
		UINT32 jitter;
	}source, * ptrsource;

	typedef struct stru_rtp_session
	{
		NETSIM_IPAddress localAdderss;
		NETSIM_IPAddress remoteAddress;
		UINT16 localPort;
		UINT16 remotePort;

		UINT16 rtcpLocalPort;
		UINT16 rtcpRemotePort;

		double tp;			/// the last time an RTCP packet was transmitted;
		double tc;			/// the current time;
		double tn;			/// the next scheduled transmission time of an RTCP packet;
		UINT pmembers;		/// the estimated number of session members at the time tn was last recomputed;
		UINT members;		/// the most current estimate for the number of session members;
		UINT senders;		/// the most current estimate for the number of senders in the session;
		UINT rtcp_bw;		/** The target RTCP bandwidth, i.e., the total bandwidth
								that will be used for RTCP packets by all members of this session,
								in octets per second.This will be a specified fraction of the
								"session bandwidth" parameter supplied to the application at
								startup.
							*/

		bool we_sent;		/// Flag that is true if the application has sent data
							/// since the 2nd previous RTCP report was transmitted.

		UINT avg_rtcp_size;	/** The average compound RTCP packet size, in octets,
								over all RTCP packets sentand received by this participant.The
								size includes lower - layer transport and network protocol headers
								(e.g., UDP and IP) as explained in Section 6.2.
							*/
		bool initial;		/// Flag that is true if the application has not yet sent an RTCP packet.

		UINT16 seqNumber;
		UINT32 ssrc;
		char* cname;
		ptrrtp_txStat txStat;
		ptrsource source;
	}RTP_SESSION, * ptrRTP_SESSION;

	typedef struct stru_rtp_dev_var
	{
		UINT rtpSessionCount;
		ptrRTP_SESSION* rtpSessions;
	}RTPVAR, *ptrRTPVAR;
#define RTPVAR_GET(d) (DEVICE_APPVAR(d,APP_PROTOCOL_RTP))
#define RTPVAR_CURR() (RTPVAR_GET(pstruEventDetails->nDeviceId))



	//Function prototype
	//RTP session
	ptrRTP_SESSION find_rtp_session(ptrRTPVAR var,
									NETSIM_IPAddress localAddress, NETSIM_IPAddress remoteAddress,
									UINT16 localPort, UINT16 remotePort);
	ptrRTP_SESSION create_rtp_session(ptrRTPVAR var,
									  NETSIM_IPAddress localAddress, NETSIM_IPAddress remoteAddress,
									  UINT16 localPort, UINT16 remotePort);
	ptrRTP_SESSION find_or_create_rtp_session(ptrRTPVAR var,
											  NETSIM_IPAddress localAddress, NETSIM_IPAddress remoteAddress,
											  UINT16 localPort, UINT16 remotePort);
	int is_rtp_session_exist(ptrRTPVAR var,
		NETSIM_IPAddress localAddress, NETSIM_IPAddress remoteAddress,
		UINT16 localPort, UINT16 remotePort);
	void rtcp_start(ptrRTP_SESSION s);

	//Event handler
	void rtp_handle_app_out();
	void rtp_handle_app_in();

	//RTP APP HANDLER
	void send_to_transport(NETSIM_ID d, ptrSOCKETINTERFACE s,
		NetSim_PACKET* packet, double time);
	UINT16 getSeq(NetSim_PACKET* packet);
	UINT getSSRC(NetSim_PACKET* packet);

	//RTP MSG
	void rtp_hdr_add(ptrRTP_SESSION s, NetSim_PACKET* packet);
	void rtp_hdr_remove(ptrRTP_SESSION s, NetSim_PACKET* packet);
	void* rtp_hdr_copy(void* hdr);
	void rtp_hdr_free(void* hdr);

	//RTP_SOURCE
	void source_init_seq(ptrsource s, uint16_t seq);
	int source_update_seq(ptrsource s, UINT16 seq);
	void source_calc_jitter(ptrsource s, UINT32 rtp_ts, UINT32 arrival);
	int source_calc_lost(const ptrsource s);
	UINT8 source_calc_fraction_lost(ptrsource s);

	//RTCP
	void RTCP_INIT();
	void rtcp_hdr_add(ptrrtcp_t sr, RTCP_MSG_HDR_SIZE size);
	void rtcp_hdr_remove(NetSim_PACKET* packet, RTCP_MSG_HDR_SIZE size);
	void RTCP_PACKET_INFO_ADD(ptrRTP_SESSION s);
	void scheduleRTCP(ptrRTP_SESSION s);
	void* rtcp_hdr_copy(void* hdr);
	void rtcp_hdr_free(void* hdr);

	//SR
	void scheduleRTCPSR(ptrRTP_SESSION s);
	void ScheduleRTCPSR();
	void ScheduleRTCPSRRECV();

	//RR
	void scheduleRTCPRR(ptrRTP_SESSION s, NETSIM_ID d, NETSIM_ID in, NETSIM_ID r, double time);
	void Schedule_RTCP_RR();
	void Schedule_RTCP_RR_RECV();

	//SDE
	void scheduleRTCPSDE(ptrRTP_SESSION s);
	void Schedule_RTCP_SDE();
	void Schedule_RTCP_SDE_RECV();

	//BYE
	void scheduleRTCPBYE(ptrRTP_SESSION s);
	void Schedule_RTCP_BYE();
	void Schedule_RTCP_BYE_RECV();

	//Log
	void init_rtp_log();
	void close_rtp_log();
	void print_rtp_log(char* format, ...);

	//SUBEVENT
	typedef struct stru_RTP_SubeventDataBase
	{
		rtcp_type_t subEvent;
		char* subeventName;
		void(*fnSubEventfunction)();
	}LTENR_SUBEVENT, * ptrLTENR_SUBEVENT;

	void RTCP_SUBEVENT_REGISTER(rtcp_type_t type,
		char* name,
		void(*fun)());
	char* RTCP_SUBEVNET_NAME(NETSIM_ID id);
	void RTCP_SUBEVENT_CALL();
#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_RTP_H_
