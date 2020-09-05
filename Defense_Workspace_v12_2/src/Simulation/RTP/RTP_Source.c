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
#pragma endregion

#pragma region INIT_SOURCE
void source_init_seq(ptrsource s, uint16_t seq)
{
	if (!s)
		return;

	s->base_seq = seq;
	s->max_seq = seq;
	s->bad_seq = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
	s->cycles = 0;
	s->received = 0;
	s->received_prior = 0;
	s->expected_prior = 0;
	/* other initialization */
}
#pragma endregion

#pragma region SOURCE_UPDATE_SEQ
/*
 * See RFC 3550 - A.1 RTP Data Header Validity Checks
 */
int source_update_seq(ptrsource s, UINT16 seq)
{
	UINT16 udelta = seq - s->max_seq;
	const int MAX_DROPOUT = 3000;
	const int MAX_MISORDER = 100;
	const int MIN_SEQUENTIAL = 2;

	/*
	 * Source is not valid until MIN_SEQUENTIAL packets with
	 * sequential sequence numbers have been received.
	 */
	if (s->probation) {

		/* packet is in sequence */
		if (seq == s->max_seq + 1) {
			s->probation--;
			s->max_seq = seq;
			if (s->probation == 0) {
				source_init_seq(s, seq);
				s->received++;
				return 1;
			}
		}
		else {
			s->probation = MIN_SEQUENTIAL - 1;
			s->max_seq = seq;
		}
		return 0;
	}
	else if (udelta < MAX_DROPOUT) {

		/* in order, with permissible gap */
		if (seq < s->max_seq) {
			/*
			 * Sequence number wrapped - count another 64K cycle.
			 */
			s->cycles += RTP_SEQ_MOD;
		}
		s->max_seq = seq;
	}
	else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {

		/* the sequence number made a very large jump */
		if (seq == s->bad_seq) {
			/*
			 * Two sequential packets -- assume that the other side
			 * restarted without telling us so just re-sync
			 * (i.e., pretend this was the first packet).
			 */
			source_init_seq(s, seq);
		}
		else {
			s->bad_seq = (seq + 1) & (RTP_SEQ_MOD - 1);
			return 0;
		}
	}
	else {
		/* duplicate or reordered packet */
	}

	s->received++;
	return 1;
}
#pragma endregion

#pragma region CALC_JITTER
/* RFC 3550 A.8
 *
 * The inputs are:
 *
 *     rtp_ts:  the timestamp from the incoming RTP packet
 *     arrival: the current time in the same units.
 */
void source_calc_jitter(ptrsource s, UINT32 rtp_ts,
	UINT32 arrival)
{
	const int transit = arrival - rtp_ts;
	int d = transit - s->transit;

	if (!s->transit) {
		s->transit = transit;
		return;
	}

	s->transit = transit;

	if (d < 0)
		d = -d;

	s->jitter += d - ((s->jitter + 8) >> 4);
}
#pragma endregion

#pragma region CALC_LOST_PACKETS
/* A.3 */
int source_calc_lost(const ptrsource s)
{
	int extended_max = s->cycles + s->max_seq;
	int expected = extended_max - s->base_seq + 1;
	int lost;

	lost = expected - s->received;

	/* Clamp at 24 bits */
	if (lost > 0x7fffff)
		lost = 0x7fffff;
	else if (lost < -0x7fffff)
		lost = -0x7fffff;

	return lost;
}


/* A.3 */
UINT8 source_calc_fraction_lost(ptrsource s)
{
	int extended_max = s->cycles + s->max_seq;
	int expected = extended_max - s->base_seq + 1;
	int expected_interval = expected - s->expected_prior;
	int received_interval;
	int lost_interval;
	UINT8 fraction;

	s->expected_prior = expected;

	received_interval = s->received - s->received_prior;

	s->received_prior = s->received;

	lost_interval = expected_interval - received_interval;

	if (expected_interval == 0 || lost_interval <= 0)
		fraction = 0;
	else
		fraction = (UINT8)((lost_interval << 8) / expected_interval);

	return fraction;
}
#pragma endregion
