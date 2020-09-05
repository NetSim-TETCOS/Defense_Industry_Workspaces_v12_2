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
* Author:    Kumar Gauarv		                                                    *
*										                                            *
* ----------------------------------------------------------------------------------*/
#include "main.h"
#include "RTP.h"

static FILE* fplog = NULL;

void init_rtp_log()
{
		char str[BUFSIZ];
		char pszIOlog[BUFSIZ];
		sprintf(pszIOlog, "%s/log", pszIOPath);
		int check = _mkdir(pszIOlog);
		if (check == 0)
			fprintf(stderr, "Created  %s directory\n", pszIOlog);
		else if (check == -1)
			fprintf(stderr, "%s directory already exists\n", pszIOlog);
		else
			fprintf(stderr, "Unable to create %s directory\n", pszIOlog);

		sprintf(str, "%s/%s", pszIOlog, "rtp.log");
		fplog = fopen(str, "w");
		if (!fplog)
		{
			perror(str);
			fnSystemError("Unable to open rtp.log file");
		}
}

void close_rtp_log()
{
	fclose(fplog);
}

void print_rtp_log(char* format, ...)
{
	if (fplog)
	{
		va_list l;
		va_start(l, format);
		vfprintf(fplog, format, l);
		fflush(fplog);
	}
}