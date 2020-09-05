/************************************************************************************
* Copyright (C) 2019																*
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
* Author:    Dilip Kumar Kundrapu													*
* Standard:  TR 38.901(Release 15)													*
* Updated:   30th November, 2019											        *
* ----------------------------------------------------------------------------------*/

#include "main.h"
#include "stdafx.h"
#include "LTENR_PHY.h"

#define PI 3.14159265

/** Error inverse Function for Normal Distribution */
static long double ErrorInverseFunc(double p)
{
	return (1.0968 * (sqrt(PI) * 0.5 * (p + (PI * pow(p, 3)) / 12 + (7 * pow(PI, 2) * pow(p, 5)) / 480 +
			(127 * pow(PI, 3) * pow(p, 7)) / 40320 + (4369 * pow(PI, 4) * pow(p, 9)) / 5806080 +
			(34807 * pow(PI, 5) * pow(p, 11)) / 182476800)));
}


//normal random variable
static double normal_distribution(double args1, double args2)
{
	double fFirstArg, fSecondArg, fRandomNumber, fDistOut;
	fFirstArg = args1;
	fSecondArg = args2;
	fRandomNumber = NETSIM_RAND_01();
	fDistOut = (double)(fFirstArg + fSecondArg * sqrt(2) * ErrorInverseFunc(2 * fRandomNumber - 1));
	return fDistOut;
}


//uniform random variable
static double uniform_distribution(double args1, double args2)
{
	double fFirstArg, fSecondArg, fRandomNumber, fDistOut;
	fFirstArg = args1;
	fSecondArg = args2;
	fRandomNumber = NETSIM_RAND_01();
	fDistOut = fFirstArg + (fSecondArg - fFirstArg) * fRandomNumber;
	return fDistOut;
}


//Step-D of Blockage Model-A
void spatialTemporalBlockerConsistency(ptrLTENR_FASTFADINGPARAMS chanParams, ptrLTENR_PROPAGATIONINFO info)
{
	double DELTA_dist_x = sqrt(pow(chanParams->lastChannelUeLoc.X - chanParams->ueLoc.X, 2) +
						  pow(chanParams->lastChannelUeLoc.Y - chanParams->ueLoc.Y, 2));
	double DELTA_time_t = ldEventTime * pow(10, 6);

	if (DELTA_dist_x != 0 || info->blockageModel->blockerSpeed != 0)
	{
		double distCorr = 0;
		switch (info->currentScenario)
		{
		case LTENR_SCENARIO_RMA:
			if (info->uePosition == LTENR_POSITION_INDOOR)
				distCorr = 5;
			else //LOS or NLOS
				distCorr = 10;
			break;
		case LTENR_SCENARIO_UMA:
			if (info->uePosition == LTENR_POSITION_INDOOR)
				distCorr = 5;
			else //LOS or NLOS
				distCorr = 10;
			break;
		case LTENR_SCENARIO_UMI:
			if (info->uePosition == LTENR_POSITION_INDOOR)
				distCorr = 5;
			else //LOS or NLOS
				distCorr = 10;
			break;
		case LTENR_SCENARIO_INH:
			distCorr = 5;
			break;
		default:
			fnNetSimError("Unknown Scenario %d in function %s\n", info->currentScenario, __FUNCTION__);
			break;
		}

		double autoCorr_R;
		if (info->blockageModel->blockerSpeed == 0)
			autoCorr_R = exp(-1 * (DELTA_dist_x / distCorr));
		else
		{
			double timeCorr = distCorr / info->blockageModel->blockerSpeed;
			autoCorr_R = exp(-1 * ((DELTA_dist_x / distCorr) + (DELTA_time_t / timeCorr)));
		}

		//Formula was obtained from MATLAB numerical simulation.
		if (autoCorr_R * autoCorr_R * (-0.069) + autoCorr_R * 1.074 - 0.002 < 1)
			autoCorr_R = autoCorr_R * autoCorr_R * (-0.069) + autoCorr_R * 1.074 - 0.002;

		for (int index = 0; index < info->blockageModel->nonSelfBlockingRegions; index++)
		{
			chanParams->nonSelfBlockageParams[index].PHI_k = autoCorr_R * 
															 chanParams->nonSelfBlockageParams[index].PHI_k +
															 sqrt(1 - autoCorr_R * autoCorr_R) * 
															 normal_distribution(0, 1);
		}
	}
	else
		fnNetSimError("The autocorrelation is 1 since Delta_X & Blocker speed are both 0 in function %s\n", 
					  __FUNCTION__);
}


//Step-B of Blockage Model-A
void generate_blocker_parameters(ptrLTENR_FASTFADINGPARAMS chanParams, ptrLTENR_PROPAGATIONINFO info,
								 ptrLTENR_SELFBLOCKPARAMS selfBlockParams)
{
	//Self-blocking parameters
	switch (info->blockageModel->orientationMode)
	{
	case LTENR_PORTRAIT_MODE:
		selfBlockParams->PHI_sb = 260;
		selfBlockParams->X_sb = 120;
		selfBlockParams->THETA_sb = 100;
		selfBlockParams->Y_sb = 80;
		break;

	case LTENR_LANDSCAPE_MODE:
		selfBlockParams->PHI_sb = 40;
		selfBlockParams->X_sb = 160;
		selfBlockParams->THETA_sb = 110;
		selfBlockParams->Y_sb = 75;
		break;

	default:
		fnNetSimError("Unknown Orientation %d in function %s\n", info->blockageModel->orientationMode,
					  __FUNCTION__);
		break;
	}

	//Non-self blocking parameters
	if (chanParams->nonSelfBlockageParams == NULL)
	{
		chanParams->nonSelfBlockageParams = calloc(info->blockageModel->nonSelfBlockingRegions,
												   sizeof * chanParams->nonSelfBlockageParams);
		for (int index = 0; index < info->blockageModel->nonSelfBlockingRegions; index++)
		{
			switch(info->currentScenario)
			{
			case LTENR_SCENARIO_RMA:
			case LTENR_SCENARIO_UMA:
			case LTENR_SCENARIO_UMI:
				chanParams->nonSelfBlockageParams[index].PHI_k = normal_distribution(0, 1);
				chanParams->nonSelfBlockageParams[index].X_k = uniform_distribution(5, 15);
				chanParams->nonSelfBlockageParams[index].THETA_k = 90;
				chanParams->nonSelfBlockageParams[index].Y_k = 5;
				chanParams->nonSelfBlockageParams[index].r = 10;
				break;

			case LTENR_SCENARIO_INH:
				chanParams->nonSelfBlockageParams[index].PHI_k = normal_distribution(0, 1);
				chanParams->nonSelfBlockageParams[index].X_k = uniform_distribution(15, 45);
				chanParams->nonSelfBlockageParams[index].THETA_k = 90;
				chanParams->nonSelfBlockageParams[index].Y_k = uniform_distribution(5, 15);
				chanParams->nonSelfBlockageParams[index].r = 2;
				break;

			default:
				fnNetSimError("Unknown Scenario %d in function %s\n", info->currentScenario, __FUNCTION__);
				break;
			}
		}
	}
	else
		spatialTemporalBlockerConsistency(chanParams, info);
}


//Using the correlation transform normal random variable into uniform random variable
void transformRandVar_normalToUniform(ptrLTENR_FASTFADINGPARAMS chanParams, ptrLTENR_PROPAGATIONINFO info)
{
	for (int index = 0; index < info->blockageModel->nonSelfBlockingRegions; index++)
	{
		chanParams->nonSelfBlockageParams[index].PHI_k = 0.5 * erfc(-1 *
															   chanParams->nonSelfBlockageParams[index].PHI_k /
															   sqrt(2)) * 360;

		while (chanParams->nonSelfBlockageParams[index].PHI_k > 360)
			chanParams->nonSelfBlockageParams[index].PHI_k = chanParams->nonSelfBlockageParams[index].PHI_k -
															 360;

		while (chanParams->nonSelfBlockageParams[index].PHI_k < 0)
			chanParams->nonSelfBlockageParams[index].PHI_k = chanParams->nonSelfBlockageParams[index].PHI_k +
															 360;
	}
}


void attenuation_nonSelfBlocks(int cluster, double* clusterAttenuation, ptrLTENR_FASTFADINGPARAMS chanParams,
							   ptrLTENR_PROPAGATIONINFO info, double* angleAOA, double* angleZOA)
{
	for (int index = 0; index < info->blockageModel->nonSelfBlockingRegions; index++)
	{
		if ((fabs(angleAOA[cluster] - chanParams->nonSelfBlockageParams[index].PHI_k) < 
			 chanParams->nonSelfBlockageParams[index].X_k) &&
			(fabs(angleZOA[cluster] - chanParams->nonSelfBlockageParams[index].THETA_k) < 
			 chanParams->nonSelfBlockageParams[index].Y_k))
		{
			double A_1 = angleAOA[cluster] - (chanParams->nonSelfBlockageParams[index].PHI_k + 
											 (chanParams->nonSelfBlockageParams[index].X_k / 2));
			double A_2 = angleAOA[cluster] - (chanParams->nonSelfBlockageParams[index].PHI_k -
											 (chanParams->nonSelfBlockageParams[index].X_k / 2));
			double Z_1 = angleZOA[cluster] - (chanParams->nonSelfBlockageParams[index].THETA_k +
											 (chanParams->nonSelfBlockageParams[index].Y_k / 2));
			double Z_2 = angleZOA[cluster] - (chanParams->nonSelfBlockageParams[index].THETA_k -
											 (chanParams->nonSelfBlockageParams[index].Y_k / 2));

			int A_1_symbol, A_2_symbol, Z_1_symbol, Z_2_symbol;
			if ((chanParams->nonSelfBlockageParams[index].X_k / 2 < 
				 angleAOA[cluster] - chanParams->nonSelfBlockageParams[index].PHI_k) &&
				(angleAOA[cluster] - chanParams->nonSelfBlockageParams[index].PHI_k <= 
				 chanParams->nonSelfBlockageParams[index].X_k))
				A_1_symbol = -1;
			else
				A_1_symbol = 1;

			if ((-chanParams->nonSelfBlockageParams[index].X_k < 
				 angleAOA[cluster] - chanParams->nonSelfBlockageParams[index].PHI_k) &&
				(angleAOA[cluster] - chanParams->nonSelfBlockageParams[index].PHI_k <= 
				 -chanParams->nonSelfBlockageParams[index].X_k / 2))
				A_2_symbol = -1;
			else
				A_2_symbol = 1;

			if ((chanParams->nonSelfBlockageParams[index].Y_k / 2 < 
				 angleZOA[cluster] - chanParams->nonSelfBlockageParams[index].THETA_k) &&
				(angleZOA[cluster] - chanParams->nonSelfBlockageParams[index].THETA_k <= 
				 chanParams->nonSelfBlockageParams[index].Y_k))
				Z_1_symbol = -1;
			else
				Z_1_symbol = 1;

			if ((-chanParams->nonSelfBlockageParams[index].Y_k <
				angleZOA[cluster] - chanParams->nonSelfBlockageParams[index].THETA_k) && 
				(angleZOA[cluster] - chanParams->nonSelfBlockageParams[index].THETA_k <= 
				 -chanParams->nonSelfBlockageParams[index].Y_k / 2))
				Z_2_symbol = -1;
			else
				Z_2_symbol = 1;

			double lamda = 3e8 / (info->centralFrequency_MHz * 1e6);
			double F_A_1 = atan(A_1_symbol * (PI / 2) * sqrt((PI / lamda) * chanParams->nonSelfBlockageParams->r *
														  (1 / cos(A_1 * (PI / 180)) - 1))) / PI;
			double F_A_2 = atan(A_2_symbol * (PI / 2) * sqrt((PI / lamda) * chanParams->nonSelfBlockageParams->r *
														  (1 / cos(A_2 * (PI / 180)) - 1))) / PI;
			double F_Z_1 = atan(Z_1_symbol * (PI / 2) * sqrt((PI / lamda) * chanParams->nonSelfBlockageParams->r *
														  (1 / cos(Z_1 * (PI / 180)) - 1))) / PI;
			double F_Z_2 = atan(Z_2_symbol * (PI / 2) * sqrt((PI / lamda) * chanParams->nonSelfBlockageParams->r *
														  (1 / cos(Z_2 * (PI / 180)) - 1))) / PI;

			double L_dB = -20 * log10(1 - (F_A_1 + F_A_2) * (F_Z_1 + F_Z_2));
			clusterAttenuation[cluster] = clusterAttenuation[cluster] + L_dB;
		}
	}
}


//Step-C of the Blockage Model-A
double* determine_clusterAttenuation(int clusterSize, ptrLTENR_SELFBLOCKPARAMS selfBlockParams, 
									 ptrLTENR_FASTFADINGPARAMS chanParams, ptrLTENR_PROPAGATIONINFO info, 
									 double* angleAOA, double* angleZOA)
{
	double* clusterAttenuation = calloc(clusterSize, sizeof * clusterAttenuation);

	for (int cluster = 0; cluster < clusterSize; cluster++)
	{
		//Attenuation due to Self-blocking regions
		if (fabs(angleAOA[cluster] - selfBlockParams->PHI_sb) < (selfBlockParams->X_sb / 2) &&
			fabs(angleZOA[cluster] - selfBlockParams->THETA_sb) < (selfBlockParams->Y_sb / 2))
			clusterAttenuation[cluster] = clusterAttenuation[cluster] + 30;
		else
			clusterAttenuation[cluster] = 0;

		transformRandVar_normalToUniform(chanParams, info);

		//Attenuation due to Non-self blocking regions
		attenuation_nonSelfBlocks(cluster, clusterAttenuation, chanParams, info, angleAOA, angleZOA);
	}
	return clusterAttenuation;
}


// TS-38.901: 7.6.4.1 Blockage Model-A
double* LTENR_Blockage_Attenuation(int clusterSize, ptrLTENR_FASTFADINGPARAMS chanParams,
								   ptrLTENR_PROPAGATIONINFO info, double* angleAOA, double* angleZOA)
{
	//Step-a: Determined the number of blockers in the Blockage Model of GUI

	//Step-b: Generate the size and location of each blocker
	ptrLTENR_SELFBLOCKPARAMS selfBlockParams = calloc(1, sizeof * selfBlockParams);
	generate_blocker_parameters(chanParams, info, selfBlockParams);

	//step c: Determine the attenuation of each blocker due to blockers
	double* attenuation = determine_clusterAttenuation(clusterSize, selfBlockParams, chanParams, info, 
													   angleAOA, angleZOA);
	free(selfBlockParams);
	return attenuation;
}