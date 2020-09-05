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
#ifndef _NETSIM_RTP_MSG_H_
#define _NETSIM_RTP_MSG_H_
#ifdef  __cplusplus
extern "C" {
#endif

	/**
	12.1 RTCP Packet Types

   abbrev.  name                 value
   SR       sender report          200
   RR       receiver report        201
   SDES     source description     202
   BYE      goodbye                203
   APP      application-defined    204
   */
	typedef enum enum_rtcp_type_t
	{
		RTCP_SR = 200,
		RTCP_RR,
		RTCP_SDES,
		RTCP_BYE,
		RTCP_APP,
	}rtcp_type_t;
	static const char* strRTCP_MSGTYPE[] = { "RTCP_SR","RTCP_RR","RTCP_SDES","RTCP_BYE","RTCP_APP" };

	typedef enum enum_rtcp_type_SubEvent_t
	{
		RTCP_SUBEVENT_SR = APP_PROTOCOL_RTP * 100,
		RTCP_SUBEVENT_RR,
		RTCP_SUBEVENT_SDES,
		RTCP_SUBEVENT_BYE,
		RTCP_SUBEVENT_APP,
	}rtcp_type_SubEvent_t;
	static const char* strRTCP_SUBEVENTTYPE[] = { "RTCP_SUBEVENR_SR","RTCP_SUBEVENT_RR","RTCP_SUBEVENT_SDES",
												  "RTCP_SUBEVENT_BYE","RTCP_SUBEVENT_APP" };
	/**
	12.2 SDES Types

   abbrev.  name                            value
   END      end of SDES list                    0
   CNAME    canonical name                      1
   NAME     user name                           2
   EMAIL    user's electronic mail address      3
   PHONE    user's phone number                 4
   LOC      geographic user location            5
   TOOL     name of application or tool         6
   NOTE     notice about the source             7
   PRIV     private extensions                  8
   */
	enum rtcp_sdes_type {
		RTCP_SDES_END = 0,
		RTCP_SDES_CNAME = 1,
		RTCP_SDES_NAME = 2,
		RTCP_SDES_EMAIL = 3,
		RTCP_SDES_PHONE = 4,
		RTCP_SDES_LOC = 5,
		RTCP_SDES_TOOL = 6,
		RTCP_SDES_NOTE = 7,
		RTCP_SDES_PRIV = 8
	};

	/**
	The RTP header has the following format:

		0                   1                   2                   3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |                           timestamp                           |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |           synchronization source (SSRC) identifier            |
	   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	   |            contributing source (CSRC) identifiers             |
	   |                             ....                              |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	*/
	typedef struct stru_rtp_hdr_fixed
	{
		UINT V : 2;
		UINT P : 1;
		UINT X : 1;
		UINT CC : 4;
		UINT M : 1;
		UINT PT : 7;
		UINT16 sequenceNumber;
		UINT timeStamp;
		UINT SSRCIndentifier;
		UINT* CSRCIdentifier;

		//NetSim specific
		void* appData;
		UINT appProtocol;
	}RTPHDR_FIXED, * ptrRTPHDR_FIXED;
#define RTPHDR_FIXED_LEN(cc)	(12 + cc*4) //Bytes

	/*
	* Big-endian mask for version, padding bit and packet type pair
	*/
#define RTCP_VALID_MASK (0xc000 | 0x2000 | 0xfe)
#define RTCP_VALID_VALUE ((RTP_VERSION << 14) | RTCP_SR)


	typedef enum enum_RTCP_MSG_HDR_SIZE{
		RTCP_HDR_SIZE,
		RTCP_SRC_SIZE,
		RTCP_SR_SIZE,
		RTCP_RR_SIZE,
		RTCP_APP_SIZE,
		RTCP_FIR_SIZE,
		RTCP_NACK_SIZE,
		RTCP_FB_SIZE,
		RTCP_MAX_SDES,
		RTCP_HEADROOM,
		RTCP_MAX_BYE,
	}RTCP_MSG_HDR_SIZE;
	static const double RTCP_MSG_HDR_SIZE_UINT[] = { 4, 4, 20, 24, 8, 4, 8, 8, 255, 4, 255 };


	enum rtcp_rtpfb {
		RTCP_RTPFB_GNACK = 1
	};

	enum rtcp_psfb {
		RTCP_PSFB_PLI = 1,
		RTCP_PSFB_SLI = 2,
		RTCP_PSFB_AFB = 15,
	};

	/*
	* Reception report block
	*/
	struct rtcp_rr {
		UINT ssrc;
		UINT fraction : 8;
		int lost : 24;
		UINT last_seq;
		UINT jitter;
		UINT lsr;
		UINT dlsr;
	};

	/*
	* SDES item
	*/
	struct rtcp_sdes_item {
		enum rtcp_sdes_type type;
		UINT length;
		char* data;
	};

	/*
	* One RTCP packet
	*/
	typedef struct rtcp_msg {
		//NetSim specific
		void* appData;
		UINT appProtocol;
		struct rtcp_hdr {
			UINT version : 2;  /* protocol version */
			UINT p : 1;		   /* padding flag */
			UINT count : 5;	   /* varies by packet type */
			UINT pt : 8;       /* RTCP packet type */
			UINT16 length;            /* pkt len in words, w/o this word */
		} hdr;
		union {
			/* sender report (SR) */
			struct {
				UINT ssrc;     /* sender generating this report */
				UINT ntp_sec;  /* NTP timestamp */
				UINT ntp_frac;
				UINT rtp_ts;   /* RTP timestamp */
				UINT psent;    /* packets sent */
				UINT osent;    /* octets sent */
				struct rtcp_rr* rrv;  /* variable-length list */
			} sr;

			/* reception report (RR) */
			struct {
				UINT ssrc;			  /* receiver generating this report */
				struct rtcp_rr* rrv;  /* variable-length list */
			} rr;

			/* source description (SDES) */
			struct rtcp_sdes {
				UINT src;					   /* first SSRC/CSRC */
				struct rtcp_sdes_item* itemv; /* list of SDES items */
				UINT n;
			} *sdesv;

			/* BYE */
			struct {
				UINT srcv;   /* list of sources */
				char* reason;
			} bye;

			/** Application-defined (APP) */
			struct {
				UINT32 src;      /**< SSRC/CSRC                  */
				char name[4];    /**< Name (ASCII)               */
				UINT16 data_len;   /**< Number of data bytes       */
			} app;
		} r;
	}rtcp_t,*ptrrtcp_t;
#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_RTP_MSG_H_
