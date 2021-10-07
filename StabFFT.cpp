/*
	PC-FFT stabilization algorithm for warpped video
*/

#include "stdafx.h"
#include <windowsx.h>
#include <memory.h>
#include <mmsystem.h>
#include "StabFFT.h"

//////////////////////////////////////////////////////////////////
//
//  The functions below are based on NVidia's CUDA technologies
//  A fast video card is needed and the Fermi core is preferred
//
//////////////////////////////////////////////////////////////////


int CStabFFT::ChooseCUDADevice(int deviceID)
{
	return Call_CUDA_SetDevice(deviceID);
}


int CStabFFT::GetCUDADeviceCounts(int* counts)
{
	int devCount, ret;
	ret = Call_CUDA_GetDeviceCount(&devCount);
	*counts = devCount;
	return ret;
}


int CStabFFT::GetCUDADeviceNames(int deviceID, char *name) {
	return Call_CUDA_GetDeviceNames(deviceID, name);
}

int CStabFFT::CUDA_FFTinit(STAB_PARAMS stab_params) {
	return Call_CUDA_FFT_init(stab_params);
}

int CStabFFT::CUDA_FFTrelease() {
	return Call_CUDA_FFT_release();
}

int CStabFFT::ImagePatchCUDA_K(int ofsX, int ofsY, BOOL bGlobalRef, int patchIdx)
{
	bool ref = (bGlobalRef) ? true : false;

	return Call_CUDA_ImagePatchK(ofsX, ofsY, ref, patchIdx);
}

int CStabFFT::GetPeakCoefsCUDA(float *coef_peak256, float *coef_peak128, float *coef_peak064)
{
	return Call_CUDA_GetPeakCoefs(coef_peak256, coef_peak128, coef_peak064);
}

int CStabFFT::SaccadeDetectionCUDA_K(float coef_peak, int patchIdx, int *sx, int *sy) {
	return Call_CUDA_SaccadeDetectionK(coef_peak, patchIdx, sx, sy, 0.4f);
}

int CStabFFT::GetCenterCUDA(float coef_peak1, float coef_peak2, BOOL jumpFlag, int ofsXOld, int ofsYOld, int *ofsX, int *ofsY) {
	bool flag = (jumpFlag) ? true : false;

	return Call_CUDA_GetCenter(coef_peak1, coef_peak2, flag, ofsXOld, ofsYOld, ofsX, ofsY, 0.4f);
}

int CStabFFT::GetPatchXY(int BlockID, int centerX, int centerY, int *deltaX, int *deltaY) {
	return Call_CUDA_GetPatchXY(m_CorrPeak064, BlockID, centerX, centerY, deltaX, deltaY, 0.4f);
}

int CStabFFT::FastConvCUDA(unsigned char *imgO, unsigned char *imgI, int imgW, int imgH, int KernelID, BOOL ref, BOOL fil) {
	bool bGlobalRef = (ref) ? true : false;
	bool bFilteredImg = (fil) ? true : false;
	return Call_CUDA_FastConvF(imgO, imgI, KernelID, imgW, imgH, bGlobalRef, bFilteredImg);
}

int CStabFFT::FastConvCUDA(unsigned char *imgO, unsigned char *imgI, int imgW, int imgH, int KernelID, BOOL ref, BOOL eof, int pid, int pcnt, int pheight, BOOL fil) {
	if ((pid+1)*pheight > imgH) return -1;

	bool bGlobalRef = (ref) ? true : false;
	bool bFilteredImg = (fil) ? true : false;
	bool bEOF = (eof) ? true : false;
	return Call_CUDA_FastConvP(imgO, imgI, KernelID, imgW, imgH, bGlobalRef, bEOF, pid, pcnt, pheight, bFilteredImg);
}

int CStabFFT::SaccadeDetectionK(int frameIndex, int patchIdx, int *sx, int *sy) {
	BOOL     bGlobalRef;
	int      g_frameIndex, ret_code;

	g_frameIndex = frameIndex;

	// this is the case with global reference
	if (g_frameIndex == 0) {
		ret_code = STAB_SUCCESS;
	} else if (g_frameIndex == 1) {
		// initialization
		bGlobalRef = TRUE;
		if (ImagePatchCUDA_K(0, 0, bGlobalRef, patchIdx) != STAB_SUCCESS) return STAB_PATCH_FFT_ERR;
		if (GetPeakCoefsCUDA(&m_CorrPeak256, &m_CorrPeak128, &m_CorrPeak064) != STAB_SUCCESS) return STAB_PEAK_COEF_ERR;
		ret_code = STAB_SUCCESS;
		*sx = *sy = 0;
	} else {
		bGlobalRef = FALSE;
		if (ImagePatchCUDA_K(0, 0, bGlobalRef, patchIdx) != STAB_SUCCESS) return STAB_PATCH_FFT_ERR;
		ret_code = SaccadeDetectionCUDA_K(m_CorrPeak064, patchIdx, sx, sy);
	}

	return ret_code;
}

BOOL CStabFFT::CalcCentralMotion(int cxOld, int cyOld, BOOL wideOpen, int *cxNew, int *cyNew) {
	int cx, cy, ret_code;

	ret_code = GetCenterCUDA(m_CorrPeak128, m_CorrPeak256, wideOpen, cxOld, cyOld, &cx, &cy);

	if (ret_code != STAB_SUCCESS) {
		cx = cy = 0;
		return FALSE;
	}

	if (!wideOpen) {
		// the result of calculation probably is not consistent with 
		// actual central motion during eye drifting 
		if (abs(cxOld-cx)>=THRESHOLD_LX || abs(cyOld-cy)>=THRESHOLD_LY) return FALSE;
	}

	*cxNew = cx;
	*cyNew = cy;

	return TRUE;
}

