/*
	PC-FFT stabilization algorithm for warpped video
*/

#include "stdafx.h"
#include <windowsx.h>
#include <memory.h>
#include <mmsystem.h>
#include "StabFFT.h"

CStabFFT::CStabFFT()
{
	m_imgWidth  = 0;
	m_imgHeight = 0;
	m_fftGloRef128 = NULL;
	m_fftGloRef256 = NULL;
	m_fftTarget128 = NULL;
	m_fftTarget256 = NULL;

	m_fftGloRef64P = NULL;
	m_fftLocRef64F = NULL;
	m_fftTarget64P = NULL;
	m_fftTarget64F = NULL;
/*
	m_fftGloRef128Re = NULL;
	m_fftGloRef256Re = NULL;
	m_fftTarget128Re = NULL;
	m_fftTarget256Re = NULL;
	m_fftGloRef128Im = NULL;
	m_fftGloRef256Im = NULL;
	m_fftTarget128Im = NULL;
	m_fftTarget256Im = NULL;

	m_fftGloRef64PRe = NULL;
	m_fftLocRef64FRe = NULL;
	m_fftTarget64PRe = NULL;
	m_fftTarget64FRe = NULL;
	m_fftGloRef64PIm = NULL;
	m_fftLocRef64FIm = NULL;
	m_fftTarget64PIm = NULL;
	m_fftTarget64FIm = NULL;
*/
	m_coef_peak064 = m_coef_peak128 = m_coef_peak256 = 0;
	m_nIdxX128 = m_nIdxY128 = m_nIdxX256 = m_nIdxY256 = m_nIdxX064 = m_nIdxY064 = 0;
	m_nCenX128 = m_nCenY128 = m_nCenX256 = m_nCenY256 = m_nCenX064 = m_nCenY064 = m_nPatchCnt = 0;
	m_bGloRefFFT = FALSE;
	m_ofsY = NULL;

	m_imgRef = m_imgPre = NULL;
	m_imgIn = m_imgOut = NULL;

	m_bSaccadeBlink = FALSE;
	m_CorrPeak256 = 1.0f;
	m_CorrPeak128 = 1.0f;
	m_CorrPeak064 = 1.0f;
	m_dx_old      = 0;
	m_dy_old      = 0;
	m_refYsold    = 0;
	m_tarYsold    = 0;
}

CStabFFT::~CStabFFT()
{
	int i, j;

	// free all allocated memory for the previous video
	if (m_fftGloRef64P != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) {
			for (j = 0; j < m_nCenX064; j ++) {
				free(m_fftGloRef64P[i][j]);
			}
			free(m_fftGloRef64P[i]);
		}
		free(m_fftGloRef64P);
	}
	if (m_fftTarget64P != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) {
			for (j = 0; j < m_nCenX064; j ++) {
				free(m_fftTarget64P[i][j]);
			}
			free(m_fftTarget64P[i]);
		}
		free(m_fftTarget64P);
	}
	if (m_fftLocRef64F != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) {
			for (j = 0; j < m_nCenX064; j ++) {
				free(m_fftLocRef64F[i][j]);
			}
			free(m_fftLocRef64F[i]);
		}
		free(m_fftLocRef64F);
	}
	if (m_fftTarget64F != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) {
			for (j = 0; j < m_nCenX064; j ++) {
				free(m_fftTarget64F[i][j]);
			}
			free(m_fftTarget64F[i]);
		}
		free(m_fftTarget64F);
	}

	if (m_fftGloRef128 != NULL) {
		for (i = 0; i < m_nCenX128; i ++) free(m_fftGloRef128[i]);
		free(m_fftGloRef128);
	}
	if (m_fftGloRef256 != NULL) {
		for (i = 0; i < m_nCenX256; i ++) free(m_fftGloRef256[i]);
		free(m_fftGloRef256);
	}
	if (m_fftTarget128 != NULL) {
		for (i = 0; i < m_nCenX128; i ++) free(m_fftTarget128[i]);
		free(m_fftTarget128);
	}
	if (m_fftTarget256 != NULL) {
		for (i = 0; i < m_nCenX256; i ++) free(m_fftTarget256[i]);
		free(m_fftTarget256);
	}

/*
	if (m_fftGloRef64PRe != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) free(m_fftGloRef64PRe[i]);
		free(m_fftGloRef64PRe);
	}
	if (m_fftTarget64PRe != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) free(m_fftTarget64PRe[i]);
		free(m_fftTarget64PRe);
	}
	if (m_fftLocRef64FRe != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) free(m_fftLocRef64FRe[i]);
		free(m_fftLocRef64FRe);
	}
	if (m_fftTarget64FRe != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) free(m_fftTarget64FRe[i]);
		free(m_fftTarget64FRe);
	}

	if (m_fftGloRef64PIm != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) free(m_fftGloRef64PIm[i]);
		free(m_fftGloRef64PIm);
	}
	if (m_fftTarget64PIm != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) free(m_fftTarget64PIm[i]);
		free(m_fftTarget64PIm);
	}
	if (m_fftLocRef64FIm != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) free(m_fftLocRef64FIm[i]);
		free(m_fftLocRef64FIm);
	}
	if (m_fftTarget64FIm != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) free(m_fftTarget64FIm[i]);
		free(m_fftTarget64FIm);
	}

	if (m_fftGloRef128Re != NULL) free(m_fftGloRef128Re);
	if (m_fftGloRef256Re != NULL) free(m_fftGloRef256Re);
	if (m_fftTarget128Re != NULL) free(m_fftTarget128Re);
	if (m_fftTarget256Re != NULL) free(m_fftTarget256Re);

	if (m_fftGloRef128Im != NULL) free(m_fftGloRef128Im);
	if (m_fftGloRef256Im != NULL) free(m_fftGloRef256Im);
	if (m_fftTarget128Im != NULL) free(m_fftTarget128Im);
	if (m_fftTarget256Im != NULL) free(m_fftTarget256Im);
*/
	if (m_ofsY != NULL) delete [] m_ofsY;

	if (m_imgRef != NULL) delete [] m_imgRef;
	if (m_imgPre != NULL) delete [] m_imgPre;
	if (m_imgIn  != NULL) delete [] m_imgIn;
	if (m_imgOut != NULL) delete [] m_imgOut;
}

// set global reference
//int CStabFFT::StabInit(int imgWidth, int imgHeight, int cenFFTx1, int cenFFTy1, int cenFFTx2, int cenFFTy2, int fineX, int fineY, int patchCnt, int *osY) {
int CStabFFT::StabInit(STAB_PARAMS stab_params) {
	int i, j;

	if (m_imgRef != NULL) delete [] m_imgRef;
	if (m_imgPre != NULL) delete [] m_imgPre;

	m_imgRef = new BYTE [stab_params.imgWidth*stab_params.imgHeight];
	m_imgPre = new BYTE [stab_params.imgWidth*stab_params.imgHeight];

	m_imgIn  = new short [stab_params.imgWidth*stab_params.imgHeight];
	m_imgOut = new short [stab_params.imgWidth*stab_params.imgHeight];
	ZeroMemory(m_imgIn, stab_params.imgWidth*stab_params.imgHeight*sizeof(short));
	ZeroMemory(m_imgOut, stab_params.imgWidth*stab_params.imgHeight*sizeof(short));

	// free all allocated memory for the previous video
	if (m_fftGloRef64P != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) {
			for (j = 0; j < m_nCenX064; j ++) {
				free(m_fftGloRef64P[i][j]);
			}
			free(m_fftGloRef64P[i]);
		}
		free(m_fftGloRef64P);
	}
	if (m_fftTarget64P != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) {
			for (j = 0; j < m_nCenX064; j ++) {
				free(m_fftTarget64P[i][j]);
			}
			free(m_fftTarget64P[i]);
		}
		free(m_fftTarget64P);
	}
	if (m_fftLocRef64F != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) {
			for (j = 0; j < m_nCenX064; j ++) {
				free(m_fftLocRef64F[i][j]);
			}
			free(m_fftLocRef64F[i]);
		}
		free(m_fftLocRef64F);
	}
	if (m_fftTarget64F != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) {
			for (j = 0; j < m_nCenX064; j ++) {
				free(m_fftTarget64F[i][j]);
			}
			free(m_fftTarget64F[i]);
		}
		free(m_fftTarget64F);
	}
	if (m_fftGloRef128 != NULL) {
		for (i = 0; i < m_nCenX128; i ++) free(m_fftGloRef128[i]);
		free(m_fftGloRef128);
	}
	if (m_fftGloRef256 != NULL) {
		for (i = 0; i < m_nCenX256; i ++) free(m_fftGloRef256[i]);
		free(m_fftGloRef256);
	}
	if (m_fftTarget128 != NULL) {
		for (i = 0; i < m_nCenX128; i ++) free(m_fftTarget128[i]);
		free(m_fftTarget128);
	}
	if (m_fftTarget256 != NULL) {
		for (i = 0; i < m_nCenX256; i ++) free(m_fftTarget256[i]);
		free(m_fftTarget256);
	}


	m_imgWidth  = stab_params.imgWidth;
	m_imgHeight = stab_params.imgHeight;

	// check if FFT width is a power of 2
	if (CheckSizeFFT(stab_params.cenFFTx1, &m_nIdxX128) != 0) return STAB_INVALID_FFT_SIZE;
	// check if FFT height is a power of 2
	if (CheckSizeFFT(stab_params.cenFFTy1, &m_nIdxY128) != 0) return STAB_INVALID_FFT_SIZE;
	
	// check if FFT width is a power of 2
	if (CheckSizeFFT(stab_params.cenFFTx2, &m_nIdxX256) != 0) return STAB_INVALID_FFT_SIZE;
	// check if FFT height is a power of 2
	if (CheckSizeFFT(stab_params.cenFFTy2, &m_nIdxY256) != 0) return STAB_INVALID_FFT_SIZE;

	// check if FFT width is a power of 2
	if (CheckSizeFFT(stab_params.fineX, &m_nIdxX064) != 0) return STAB_INVALID_FFT_SIZE;
	// check if FFT height is a power of 2
	if (CheckSizeFFT(stab_params.fineY, &m_nIdxY064) != 0) return STAB_INVALID_FFT_SIZE;


	// reallocate memory for new video
	m_fftGloRef64P = (COMPLEX ***)malloc(stab_params.patchCnt*sizeof(COMPLEX));
	for (i = 0; i < stab_params.patchCnt; i ++) m_fftGloRef64P[i] = (COMPLEX **)malloc(stab_params.fineX*sizeof(COMPLEX));
	for (i = 0; i < stab_params.patchCnt; i ++) {
		for (j = 0; j < stab_params.fineX; j ++) {
			m_fftGloRef64P[i][j] = (COMPLEX *)malloc(stab_params.fineY*sizeof(COMPLEX));
		}
	}
	m_fftTarget64P = (COMPLEX ***)malloc(stab_params.patchCnt*sizeof(COMPLEX));
	for (i = 0; i < stab_params.patchCnt; i ++) m_fftTarget64P[i] = (COMPLEX **)malloc(stab_params.fineX*sizeof(COMPLEX));
	for (i = 0; i < stab_params.patchCnt; i ++) {
		for (j = 0; j < stab_params.fineX; j ++) {
			m_fftTarget64P[i][j] = (COMPLEX *)malloc(stab_params.fineY*sizeof(COMPLEX));
		}
	}
	m_fftLocRef64F = (COMPLEX ***)malloc(DETECTION_PATCH*sizeof(COMPLEX));
	for (i = 0; i < DETECTION_PATCH; i ++) m_fftLocRef64F[i] = (COMPLEX **)malloc(stab_params.fineX*sizeof(COMPLEX));
	for (i = 0; i < DETECTION_PATCH; i ++) {
		for (j = 0; j < stab_params.fineX; j ++) {
			m_fftLocRef64F[i][j] = (COMPLEX *)malloc(stab_params.fineY*sizeof(COMPLEX));
		}
	}
	m_fftTarget64F = (COMPLEX ***)malloc(DETECTION_PATCH*sizeof(COMPLEX));
	for (i = 0; i < DETECTION_PATCH; i ++) m_fftTarget64F[i] = (COMPLEX **)malloc(stab_params.fineX*sizeof(COMPLEX));
	for (i = 0; i < DETECTION_PATCH; i ++) {
		for (j = 0; j < stab_params.fineX; j ++) {
			m_fftTarget64F[i][j] = (COMPLEX *)malloc(stab_params.fineY*sizeof(COMPLEX));
		}
	}
	m_fftGloRef128 = (COMPLEX **)malloc(stab_params.cenFFTx1*sizeof(COMPLEX));
	for (i = 0; i < stab_params.cenFFTx1; i ++) 
		m_fftGloRef128[i] = (COMPLEX *)malloc(stab_params.cenFFTy1*sizeof(COMPLEX));
	m_fftGloRef256 = (COMPLEX **)malloc(stab_params.cenFFTx2*sizeof(COMPLEX));
	for (i = 0; i < stab_params.cenFFTx2; i ++) 
		m_fftGloRef256[i] = (COMPLEX *)malloc(stab_params.cenFFTy2*sizeof(COMPLEX));
	m_fftTarget128 = (COMPLEX **)malloc(stab_params.cenFFTx1*sizeof(COMPLEX));
	for (i = 0; i < stab_params.cenFFTx1; i ++) 
		m_fftTarget128[i] = (COMPLEX *)malloc(stab_params.cenFFTy1*sizeof(COMPLEX));
	m_fftTarget256 = (COMPLEX **)malloc(stab_params.cenFFTx2*sizeof(COMPLEX));
	for (i = 0; i < stab_params.cenFFTx2; i ++) 
		m_fftTarget256[i] = (COMPLEX *)malloc(stab_params.cenFFTy2*sizeof(COMPLEX));


	m_nCenX128 = stab_params.cenFFTx1;
	m_nCenY128 = stab_params.cenFFTy1;
	m_nCenX256 = stab_params.cenFFTx2;
	m_nCenY256 = stab_params.cenFFTy2;
	m_nCenX064 = stab_params.fineX;
	m_nCenY064 = stab_params.fineY;
	m_nPatchCnt= stab_params.patchCnt;

	if (m_ofsY != NULL) delete [] m_ofsY;
	m_ofsY = new int [stab_params.patchCnt];
	for (i = 0; i < stab_params.patchCnt; i ++) {
		m_ofsY[i] = stab_params.osY[i];
	}

	return STAB_SUCCESS;
}
/*
int CStabFFT::StabInitL(int imgWidth, int imgHeight, int cenFFTx1, int cenFFTy1, int cenFFTx2, int cenFFTy2, int fineX, int fineY, int patchCnt, int *osY) {
	int i;

	if (m_imgRef != NULL) delete [] m_imgRef;
	if (m_imgPre != NULL) delete [] m_imgPre;

	m_imgRef = new BYTE [imgWidth*imgHeight];
	m_imgPre = new BYTE [imgWidth*imgHeight];

	// free all allocated memory for the previous video
	if (m_fftGloRef64PRe != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) free(m_fftGloRef64PRe[i]);
		free(m_fftGloRef64PRe);
	}
	if (m_fftTarget64PRe != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) free(m_fftTarget64PRe[i]);
		free(m_fftTarget64PRe);
	}
	if (m_fftLocRef64FRe != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) free(m_fftLocRef64FRe[i]);
		free(m_fftLocRef64FRe);
	}
	if (m_fftTarget64FRe != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) free(m_fftTarget64FRe[i]);
		free(m_fftTarget64FRe);
	}

	if (m_fftGloRef64PIm != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) free(m_fftGloRef64PIm[i]);
		free(m_fftGloRef64PIm);
	}
	if (m_fftTarget64PIm != NULL) {
		for (i = 0; i < m_nPatchCnt; i ++) free(m_fftTarget64PIm[i]);
		free(m_fftTarget64PIm);
	}
	if (m_fftLocRef64FIm != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) free(m_fftLocRef64FIm[i]);
		free(m_fftLocRef64FIm);
	}
	if (m_fftTarget64FIm != NULL) {
		for (i = 0; i < DETECTION_PATCH; i ++) free(m_fftTarget64FIm[i]);
		free(m_fftTarget64FIm);
	}

	if (m_fftGloRef128Re != NULL) free(m_fftGloRef128Re);
	if (m_fftGloRef256Re != NULL) free(m_fftGloRef256Re);
	if (m_fftTarget128Re != NULL) free(m_fftTarget128Re);
	if (m_fftTarget256Re != NULL) free(m_fftTarget256Re);

	if (m_fftGloRef128Im != NULL) free(m_fftGloRef128Im);
	if (m_fftGloRef256Im != NULL) free(m_fftGloRef256Im);
	if (m_fftTarget128Im != NULL) free(m_fftTarget128Im);
	if (m_fftTarget256Im != NULL) free(m_fftTarget256Im);

	m_imgWidth  = imgWidth;
	m_imgHeight = imgHeight;

	// check if FFT width is a power of 2
	if (CheckSizeFFT(cenFFTx1, &m_nIdxX128) != 0) return STAB_INVALID_FFT_SIZE;
	// check if FFT height is a power of 2
	if (CheckSizeFFT(cenFFTy1, &m_nIdxY128) != 0) return STAB_INVALID_FFT_SIZE;
	
	// check if FFT width is a power of 2
	if (CheckSizeFFT(cenFFTx2, &m_nIdxX256) != 0) return STAB_INVALID_FFT_SIZE;
	// check if FFT height is a power of 2
	if (CheckSizeFFT(cenFFTy2, &m_nIdxY256) != 0) return STAB_INVALID_FFT_SIZE;

	// check if FFT width is a power of 2
	if (CheckSizeFFT(fineX, &m_nIdxX064) != 0) return STAB_INVALID_FFT_SIZE;
	// check if FFT height is a power of 2
	if (CheckSizeFFT(fineY, &m_nIdxY064) != 0) return STAB_INVALID_FFT_SIZE;


	// reallocate memory for new video
	// allocate memory for saccade/blink detection
	m_fftGloRef64PRe = (float **)malloc(patchCnt*sizeof(float));
	for (i = 0; i < patchCnt; i ++) m_fftGloRef64PRe[i] = (float *)malloc(fineX*fineY*sizeof(float));
	m_fftTarget64PRe = (float **)malloc(patchCnt*sizeof(float));
	for (i = 0; i < patchCnt; i ++) m_fftTarget64PRe[i] = (float *)malloc(fineX*fineY*sizeof(float));
	m_fftLocRef64FRe = (float **)malloc(DETECTION_PATCH*sizeof(float));
	for (i = 0; i < DETECTION_PATCH; i ++) m_fftLocRef64FRe[i] = (float *)malloc(fineX*fineY*sizeof(float));
	m_fftTarget64FRe = (float **)malloc(DETECTION_PATCH*sizeof(float));
	for (i = 0; i < DETECTION_PATCH; i ++) m_fftTarget64FRe[i] = (float *)malloc(fineX*fineY*sizeof(float));
	// allocate memory for target motion
	m_fftGloRef64PIm = (float **)malloc(patchCnt*sizeof(float));
	for (i = 0; i < patchCnt; i ++) m_fftGloRef64PIm[i] = (float *)malloc(fineX*fineY*sizeof(float));
	m_fftTarget64PIm = (float **)malloc(patchCnt*sizeof(float));
	for (i = 0; i < patchCnt; i ++) m_fftTarget64PIm[i] = (float *)malloc(fineX*fineY*sizeof(float));
	m_fftLocRef64FIm = (float **)malloc(DETECTION_PATCH*sizeof(float));
	for (i = 0; i < DETECTION_PATCH; i ++) m_fftLocRef64FIm[i] = (float *)malloc(fineX*fineY*sizeof(float));
	m_fftTarget64FIm = (float **)malloc(DETECTION_PATCH*sizeof(float));
	for (i = 0; i < DETECTION_PATCH; i ++) m_fftTarget64FIm[i] = (float *)malloc(fineX*fineY*sizeof(float));
	// allocate memory for central motion
	m_fftGloRef128Re = (float *)malloc(cenFFTx1*cenFFTy1*sizeof(float));
	m_fftGloRef256Re = (float *)malloc(cenFFTx2*cenFFTy2*sizeof(float));
	m_fftTarget128Re = (float *)malloc(cenFFTx1*cenFFTy1*sizeof(float));
	m_fftTarget256Re = (float *)malloc(cenFFTx2*cenFFTy2*sizeof(float));
	m_fftGloRef128Im = (float *)malloc(cenFFTx1*cenFFTy1*sizeof(float));
	m_fftGloRef256Im = (float *)malloc(cenFFTx2*cenFFTy2*sizeof(float));
	m_fftTarget128Im = (float *)malloc(cenFFTx1*cenFFTy1*sizeof(float));
	m_fftTarget256Im = (float *)malloc(cenFFTx2*cenFFTy2*sizeof(float));

	
	m_nCenX128 = cenFFTx1;
	m_nCenY128 = cenFFTy1;
	m_nCenX256 = cenFFTx2;
	m_nCenY256 = cenFFTy2;
	m_nCenX064 = fineX;
	m_nCenY064 = fineY;
	m_nPatchCnt= patchCnt;

	if (m_ofsY != NULL) delete [] m_ofsY;
	m_ofsY = new int [patchCnt];
	for (i = 0; i < patchCnt; i ++) {
		m_ofsY[i] = osY[i];
	}

	return STAB_SUCCESS;
}
*/
// do FFT on patch level of an image. All parameters are input
// 1. image     -- raw image
// 2. ofsX      -- central offset X on the raw image
// 3. ofsY      -- central offset Y on the raw image
// 4. bGlobalRef-- flag with global reference frame
// 5. jumpFlag  -- TRUE:saccade/blink, FALSE:drift zone
int CStabFFT::ImagePatchFFT(BYTE *image, int ofsX, int ofsY, BOOL bGlobalRef)
{
	int i, j, k, n, m, offset;

	// this is the global reference frame
	if (bGlobalRef == TRUE) {
		memcpy(m_imgRef, image, m_imgWidth*m_imgHeight);

		// -------------------------------------------------
		// calculate central patch with 256 pixels of height
		// -------------------------------------------------
		// copy raw images to two patches
		offset  = m_imgWidth*(m_imgHeight-m_nCenY256)/2 + (m_imgWidth-m_nCenX256)/2;
		for (j = 0; j < m_nCenY256; j ++) {
			n = offset + j*m_imgWidth;
			m = j * m_nCenX256;
			for (i = 0; i < m_nCenX256; i ++) {
				m_fftGloRef256[i][j].real = image[n+i];
				m_fftGloRef256[i][j].imag = 0;
			}
		}
		// do forward 2-D FFT on a patch of the global reference frame
		FFT2D(m_fftGloRef256, m_nCenX256, m_nCenY256, m_nIdxX256, m_nIdxY256, 1);

		// -------------------------------------------------
		// calculate central patch with 128 pixels of height
		// -------------------------------------------------
		// copy raw images to two patches
		offset  = m_imgWidth*(m_imgHeight-m_nCenY128)/2 + (m_imgWidth-m_nCenX128)/2;
		for (j = 0; j < m_nCenY128; j ++) {
			n = offset + j*m_imgWidth;
			m = j*m_nCenX128;
			for (i = 0; i < m_nCenX128; i ++) {
				m_fftGloRef128[i][j].real = image[n+i];
				m_fftGloRef128[i][j].imag = 0;
			}
		}

		// do forward 2-D FFT on a patch of the global reference frame
		FFT2D(m_fftGloRef128, m_nCenX128, m_nCenY128, m_nIdxX128, m_nIdxY128, 1);
/*
		FILE *fp;
		fp = fopen("pc_d.txt", "w");
		for (j = 0; j < m_nCenY128; j ++) {
			for (i = 0; i < m_nCenX128; i ++) {
				fprintf(fp, "(%f,%f)\n", m_fftGloRef128[i][j].real, m_fftGloRef128[i][j]);
			}
		}
		fclose(fp);
*/
		// -------------------------------------------------
		// calculate FFT of local reference patch for saccade/blink detection
		// -------------------------------------------------
		for (k = 0; k < DETECTION_PATCH; k ++) {
			offset = k*m_imgWidth*m_nCenY064 + (m_imgWidth-m_nCenX064)/2;
			for (j = 0; j < m_nCenY064; j ++) {
				n = offset + j*m_imgWidth;
				m = j*m_nCenX064;
				for (i = 0; i < m_nCenX064; i ++) {
					m_fftLocRef64F[k][i][j].real = image[n+i];
					m_fftLocRef64F[k][i][j].imag = 0;
				}
			}
			FFT2D(m_fftLocRef64F[k], m_nCenX064, m_nCenY064, m_nIdxX064, m_nIdxY064, 1);
		}

		m_bGloRefFFT = TRUE;
	// this is the target frame
	} else {
		if (m_bGloRefFFT == FALSE) return STAB_NO_REFERENCE;

		memcpy(m_imgPre, image, m_imgWidth*m_imgHeight);
/*
		if (jumpFlag == TRUE) {
			// -------------------------------------------------
			// calculate central patch of target with 256 pixels of height
			// -------------------------------------------------
			// copy raw images to two patches
			offset  = m_imgWidth*(m_imgHeight-m_nCenY256)/2 + (m_imgWidth-m_nCenX256)/2;
			for (j = 0; j < m_nCenY256; j ++) {
				n = offset + j*m_imgWidth;
				m = j*m_nCenX256;
				for (i = 0; i < m_nCenX256; i ++) {
					m_fftTarget256[i][j].real = image[n+i];
					m_fftTarget256[i][j].imag = 0;
				}
			}
			// do forward 2-D FFT on a patch of the target frame
			FFT2D(m_fftTarget256, m_nCenX256, m_nCenY256, m_nIdxX256, m_nIdxY256, 1);
		} else {
			// -------------------------------------------------
			// calculate central patch of target with 128 pixels of height
			// -------------------------------------------------
			// copy raw images to two patches
			offset  = m_imgWidth*((m_imgHeight-m_nCenY128)/2-ofsY) + (m_imgWidth-m_nCenX128)/2-ofsX;
			for (j = 0; j < m_nCenY128; j ++) {
				n = offset + j*m_imgWidth;
				m = j*m_nCenX128;
				for (i = 0; i < m_nCenX128; i ++) {
					m_fftTarget128[i][j].real = image[n+i];
					m_fftTarget128[i][j].imag = 0;
				}
			}
			// do forward 2-D FFT on a patch of the target frame
			FFT2D(m_fftTarget128, m_nCenX128, m_nCenY128, m_nIdxX128, m_nIdxY128, 1);
		}
*/
		// -------------------------------------------------
		// calculate FFT of target patch for saccade/blink detection
		// -------------------------------------------------
		for (k = 0; k < DETECTION_PATCH; k ++) {
			offset = k*m_imgWidth*m_nCenY064 + (m_imgWidth-m_nCenX064)/2;
			for (j = 0; j < m_nCenY064; j ++) {
				n = offset + j*m_imgWidth;
				m = j*m_nCenX064;
				for (i = 0; i < m_nCenX064; i ++) {
					m_fftTarget64F[k][i][j].real = image[n+i];
					m_fftTarget64F[k][i][j].imag = 0;
				}
			}
			FFT2D(m_fftTarget64F[k], m_nCenX064, m_nCenY064, m_nIdxX064, m_nIdxY064, 1);
		}
	}

	return STAB_SUCCESS;
}
/*
int CStabFFT::ImagePatchFFTL(BYTE *image, int ofsX, int ofsY, BOOL bGlobalRef, BOOL jumpFlag)
{
	int i, j, k, n, m, offset;

	// this is the global reference frame
	if (bGlobalRef == TRUE) {
		memcpy(m_imgRef, image, m_imgWidth*m_imgHeight);

		// -------------------------------------------------
		// calculate central patch with 256 pixels of height
		// -------------------------------------------------
		// copy raw images to two patches
		offset  = m_imgWidth*(m_imgHeight-m_nCenY256)/2 + (m_imgWidth-m_nCenX256)/2;
		for (j = 0; j < m_nCenY256; j ++) {
			n = offset + j*m_imgWidth;
			m = j * m_nCenX256;
			for (i = 0; i < m_nCenX256; i ++) {
				m_fftGloRef256Re[m+i] = image[n+i];
				m_fftGloRef256Im[m+i] = 0;
			}
		}
		// do forward 2-D FFT on a patch of the global reference frame
		FFT2D(m_fftGloRef256Re, m_fftGloRef256Im, m_nCenX256, m_nCenY256, m_nIdxX256, m_nIdxY256, 1);

		// -------------------------------------------------
		// calculate central patch with 128 pixels of height
		// -------------------------------------------------
		// copy raw images to two patches
		offset  = m_imgWidth*(m_imgHeight-m_nCenY128)/2 + (m_imgWidth-m_nCenX128)/2;
		for (j = 0; j < m_nCenY128; j ++) {
			n = offset + j*m_imgWidth;
			m = j*m_nCenX128;
			for (i = 0; i < m_nCenX128; i ++) {
				m_fftGloRef128Re[m+i] = image[n+i];
				m_fftGloRef128Im[m+i] = 0;
			}
		}
		// do forward 2-D FFT on a patch of the global reference frame
		FFT2D(m_fftGloRef128Re, m_fftGloRef128Im, m_nCenX128, m_nCenY128, m_nIdxX128, m_nIdxY128, 1);

		// -------------------------------------------------
		// calculate FFT of local reference patch for saccade/blink detection
		// -------------------------------------------------
		for (k = 0; k < DETECTION_PATCH; k ++) {
			offset = k*m_imgWidth*m_nCenY064 + (m_imgWidth-m_nCenX064)/2;
			for (j = 0; j < m_nCenY064; j ++) {
				n = offset + j*m_imgWidth;
				m = j*m_nCenX064;
				for (i = 0; i < m_nCenX064; i ++) {
					m_fftLocRef64FRe[k][m+i] = image[n+i];
					m_fftLocRef64FIm[k][m+i] = 0;
				}
			}
			FFT2D(m_fftLocRef64FRe[k], m_fftLocRef64FIm[k], m_nCenX064, m_nCenY064, m_nIdxX064, m_nIdxY064, 1);
		}

		m_bGloRefFFT = TRUE;
	// this is the target frame
	} else {
		if (m_bGloRefFFT == FALSE) return STAB_NO_REFERENCE;

		memcpy(m_imgPre, image, m_imgWidth*m_imgHeight);

		if (jumpFlag == TRUE) {
			// -------------------------------------------------
			// calculate central patch of target with 256 pixels of height
			// -------------------------------------------------
			// copy raw images to two patches
			offset  = m_imgWidth*(m_imgHeight-m_nCenY256)/2 + (m_imgWidth-m_nCenX256)/2;
			for (j = 0; j < m_nCenY256; j ++) {
				n = offset + j*m_imgWidth;
				m = j*m_nCenX256;
				for (i = 0; i < m_nCenX256; i ++) {
					m_fftTarget256Re[m+i] = image[n+i];
					m_fftTarget256Im[m+i] = 0;
				}
			}
			// do forward 2-D FFT on a patch of the target frame
			FFT2D(m_fftTarget256Re, m_fftTarget256Im, m_nCenX256, m_nCenY256, m_nIdxX256, m_nIdxY256, 1);

			// -------------------------------------------------
			// calculate FFT of target patch for saccade/blink detection
			// -------------------------------------------------
			for (k = 0; k < DETECTION_PATCH; k ++) {
				offset = k*m_imgWidth*m_nCenY064 + (m_imgWidth-m_nCenX064)/2;
				for (j = 0; j < m_nCenY064; j ++) {
					n = offset + j*m_imgWidth;
					m = j*m_nCenX064;
					for (i = 0; i < m_nCenX064; i ++) {
						m_fftTarget64FRe[k][m+i] = image[n+i];
						m_fftTarget64FIm[k][m+i] = 0;
					}
				}
				FFT2D(m_fftTarget64FRe[k], m_fftTarget64FIm[k], m_nCenX064, m_nCenY064, m_nIdxX064, m_nIdxY064, 1);
			}
			
		} else {
			// -------------------------------------------------
			// calculate central patch of target with 128 pixels of height
			// -------------------------------------------------
			// copy raw images to two patches
			offset  = m_imgWidth*((m_imgHeight-m_nCenY128)/2-ofsY) + (m_imgWidth-m_nCenX128)/2-ofsX;
			for (j = 0; j < m_nCenY128; j ++) {
				n = offset + j*m_imgWidth;
				m = j*m_nCenX128;
				for (i = 0; i < m_nCenX128; i ++) {
					m_fftTarget128Re[m+i] = image[n+i];
					m_fftTarget128Im[m+i] = 0;
				}
			}
			// do forward 2-D FFT on a patch of the target frame
			FFT2D(m_fftTarget128Re, m_fftTarget128Im, m_nCenX128, m_nCenY128, m_nIdxX128, m_nIdxY128, 1);

			// -------------------------------------------------
			// calculate FFT of target patch for saccade/blink detection
			// -------------------------------------------------
			for (k = 0; k < DETECTION_PATCH; k ++) {
				offset = k*m_imgWidth*m_nCenY064 + (m_imgWidth-m_nCenX064)/2;
				for (j = 0; j < m_nCenY064; j ++) {
					n = offset + j*m_imgWidth;
					m = j*m_nCenX064;
					for (i = 0; i < m_nCenX064; i ++) {
						m_fftTarget64FRe[k][m+i] = image[n+i];
						m_fftTarget64FIm[k][m+i] = 0;
					}
				}
				FFT2D(m_fftTarget64FRe[k], m_fftTarget64FIm[k], m_nCenX064, m_nCenY064, m_nIdxX064, m_nIdxY064, 1);
			}
		}
	}

	return STAB_SUCCESS;
}
*/
int CStabFFT::GetPeakCoefs(float *coef_peak256, float *coef_peak128, float *coef_peak064)
{
	int   i, j, k, fftWidth, fftHeight, nx, ny, max_x, max_y;
	float real, imag, dMax;

	// ------------------------------------------------------
	// calculate auto correlation coefficient of 256x64 patch
	// ------------------------------------------------------
	fftWidth  = m_nCenX064;
	fftHeight = m_nCenY064;
	nx        = m_nIdxX064;
	ny        = m_nIdxY064;
	k         = DETECTION_PATCH - 1; 

	for (j = 0; j < fftHeight; j ++) {
		for (i = 0; i < fftWidth; i ++) {
			real = m_fftLocRef64F[k][i][j].real*m_fftLocRef64F[k][i][j].real + m_fftLocRef64F[k][i][j].imag*m_fftLocRef64F[k][i][j].imag;
			imag = m_fftLocRef64F[k][i][j].imag*m_fftLocRef64F[k][i][j].real - m_fftLocRef64F[k][i][j].real*m_fftLocRef64F[k][i][j].imag;
			m_fftTarget64P[k][i][j].real = real;
			m_fftTarget64P[k][i][j].imag = imag;
		}
	}
	// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
	FFT2D(m_fftTarget64P[k], fftWidth, fftHeight, nx, ny, -1);

	// retrieve real part of    fft(x).*conj(fft(y))
	max_x = max_y = -1000000000;
	dMax = -1000000000;
	for (j = 0; j < fftHeight; j ++) {
		for (i = 0; i < fftWidth; i ++) {
			if (dMax < m_fftTarget64P[k][i][j].real) {
				dMax  = m_fftTarget64P[k][i][j].real;
				max_x = i;
				max_y = j;
			}
		}
	}

	if (max_x == 0 && max_y == 0)
		*coef_peak064 = dMax;
	else
		return STAB_AUTO_CORR_ERR;


	// ------------------------------------------------------
	// calculate auto correlation coefficient of 256x128 patch
	// ------------------------------------------------------
	fftWidth  = m_nCenX128;
	fftHeight = m_nCenY128;
	nx        = m_nIdxX128;
	ny        = m_nIdxY128;

	for (j = 0; j < fftHeight; j ++) {
		for (i = 0; i < fftWidth; i ++) {
			real = m_fftGloRef128[i][j].real*m_fftGloRef128[i][j].real + m_fftGloRef128[i][j].imag*m_fftGloRef128[i][j].imag;
			imag = m_fftGloRef128[i][j].imag*m_fftGloRef128[i][j].real - m_fftGloRef128[i][j].real*m_fftGloRef128[i][j].imag;
			m_fftTarget128[i][j].real = real;
			m_fftTarget128[i][j].imag = imag;
		}
	}
	// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
	FFT2D(m_fftTarget128, fftWidth, fftHeight, nx, ny, -1);

	// retrieve real part of    fft(x).*conj(fft(y))
	max_x = max_y = -1000000000;
	dMax = -1000000000;
	for (j = 0; j < fftHeight; j ++) {
		for (i = 0; i < fftWidth; i ++) {
			if (dMax < m_fftTarget128[i][j].real) {
				dMax  = m_fftTarget128[i][j].real;
				max_x = i;
				max_y = j;
			}
		}
	}

	if (max_x == 0 && max_y == 0)
		*coef_peak128 = dMax;
	else
		return STAB_AUTO_CORR_ERR;


	// ------------------------------------------------------
	// calculate auto correlation coefficient of 256x256 patch
	// ------------------------------------------------------
	fftWidth  = m_nCenX256;
	fftHeight = m_nCenY256;
	nx        = m_nIdxX256;
	ny        = m_nIdxY256;

	for (j = 0; j < fftHeight; j ++) {
		for (i = 0; i < fftWidth; i ++) {
			real = m_fftGloRef256[i][j].real*m_fftGloRef256[i][j].real + m_fftGloRef256[i][j].imag*m_fftGloRef256[i][j].imag;
			imag = m_fftGloRef256[i][j].imag*m_fftGloRef256[i][j].real - m_fftGloRef256[i][j].real*m_fftGloRef256[i][j].imag;
			m_fftTarget256[i][j].real = real;
			m_fftTarget256[i][j].imag = imag;
		}
	}
	// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
	FFT2D(m_fftTarget256, fftWidth, fftHeight, nx, ny, -1);

	// retrieve real part of    fft(x).*conj(fft(y))
	max_x = max_y = -1000000000;
	dMax = -1000000000;
	for (j = 0; j < fftHeight; j ++) {
		for (i = 0; i < fftWidth; i ++) {
			if (dMax < m_fftTarget256[i][j].real) {
				dMax  = m_fftTarget256[i][j].real;
				max_x = i;
				max_y = j;
			}
		}
	}

	if (max_x == 0 && max_y == 0)
		*coef_peak256 = dMax;
	else
		return STAB_AUTO_CORR_ERR;


	return STAB_SUCCESS;
}
/*
int CStabFFT::GetPeakCoefsL(float *coef_peak256, float *coef_peak128, float *coef_peak064)
{
	int   i, k, fftWidth, fftHeight, nx, ny, max_x, max_y;
	float real, imag, dMax;

	// ------------------------------------------------------
	// calculate auto correlation coefficient of 256x64 patch
	// ------------------------------------------------------
	fftWidth  = m_nCenX064;
	fftHeight = m_nCenY064;
	nx        = m_nIdxX064;
	ny        = m_nIdxY064;
	k         = DETECTION_PATCH - 1; 

	for (i = 0; i < fftWidth*fftHeight; i ++) {
		real = m_fftLocRef64FRe[k][i]*m_fftLocRef64FRe[k][i] + m_fftLocRef64FIm[k][i]*m_fftLocRef64FIm[k][i];
		imag = m_fftLocRef64FIm[k][i]*m_fftLocRef64FRe[k][i] - m_fftLocRef64FRe[k][i]*m_fftLocRef64FIm[k][i];
		m_fftTarget64PRe[k][i] = real;
		m_fftTarget64PIm[k][i] = imag;
	}

	// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
	FFT2D(m_fftTarget64PRe[k], m_fftTarget64PIm[k], fftWidth, fftHeight, nx, ny, -1);

	// retrieve real part of    fft(x).*conj(fft(y))
	max_x = max_y = -1000000000;
	dMax = -1000000000;
	for (i = 0; i < fftWidth*fftHeight; i ++) {
		if (dMax < m_fftTarget64PRe[k][i]) {
			dMax  = m_fftTarget64PRe[k][i];
			max_x = i%fftWidth;
			max_y = i/fftWidth;
		}
	}

	if (max_x == 0 && max_y == 0)
		*coef_peak064 = dMax;
	else
		return STAB_AUTO_CORR_ERR;


	// ------------------------------------------------------
	// calculate auto correlation coefficient of 256x128 patch
	// ------------------------------------------------------
	fftWidth  = m_nCenX128;
	fftHeight = m_nCenY128;
	nx        = m_nIdxX128;
	ny        = m_nIdxY128;

	for (i = 0; i < fftWidth*fftHeight; i ++) {
		real = m_fftGloRef128Re[i]*m_fftGloRef128Re[i] + m_fftGloRef128Im[i]*m_fftGloRef128Im[i];
		imag = m_fftGloRef128Im[i]*m_fftGloRef128Re[i] - m_fftGloRef128Re[i]*m_fftGloRef128Im[i];
		m_fftTarget128Re[i] = real;
		m_fftTarget128Im[i] = imag;
	}
	// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
	FFT2D(m_fftTarget128Re, m_fftTarget128Im, fftWidth, fftHeight, nx, ny, -1);

	// retrieve real part of    fft(x).*conj(fft(y))
	max_x = max_y = -1000000000;
	dMax = -1000000000;
	for (i = 0; i < fftWidth*fftHeight; i ++) {
		if (dMax < m_fftTarget128Re[i]) {
			dMax  = m_fftTarget128Re[i];
			max_x = i%fftWidth;
			max_y = i/fftWidth;
		}
	}

	if (max_x == 0 && max_y == 0)
		*coef_peak128 = dMax;
	else
		return STAB_AUTO_CORR_ERR;


	// ------------------------------------------------------
	// calculate auto correlation coefficient of 256x256 patch
	// ------------------------------------------------------
	fftWidth  = m_nCenX256;
	fftHeight = m_nCenY256;
	nx        = m_nIdxX256;
	ny        = m_nIdxY256;

	for (i = 0; i < fftWidth*fftHeight; i ++) {
		real = m_fftGloRef256Re[i]*m_fftGloRef256Re[i] + m_fftGloRef256Im[i]*m_fftGloRef256Im[i];
		imag = m_fftGloRef256Im[i]*m_fftGloRef256Re[i] - m_fftGloRef256Re[i]*m_fftGloRef256Im[i];
		m_fftTarget256Re[i] = real;
		m_fftTarget256Im[i] = imag;
	}
	// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
	FFT2D(m_fftTarget256Re, m_fftTarget256Im, fftWidth, fftHeight, nx, ny, -1);

	// retrieve real part of    fft(x).*conj(fft(y))
	max_x = max_y = -1000000000;
	dMax = -1000000000;
	for (i = 0; i < fftWidth*fftHeight; i ++) {
		if (dMax < m_fftTarget256Re[i]) {
			dMax  = m_fftTarget256Re[i];
			max_x = i%fftWidth;
			max_y = i/fftWidth;
		}
	}

	if (max_x == 0 && max_y == 0)
		*coef_peak256 = dMax;
	else
		return STAB_AUTO_CORR_ERR;


	return STAB_SUCCESS;
}
*/
int CStabFFT::SaccaBlinkDetection(float coef_peak) 
{
	int   i, j, k, fftWidth, fftHeight, nx, ny, max_x, max_y;
	float real, imag, dMax, coef;
	BOOL  bFlag;

	fftWidth  = m_nCenX064;
	fftHeight = m_nCenY064;
	nx        = m_nIdxX064;
	ny        = m_nIdxY064;
/*
	FILE *fp;
	fp = fopen("pc_d.txt", "w");
	for (k = 0; k < DETECTION_PATCH; k ++) {
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				fprintf(fp, "(%f,%f)\n", m_fftLocRef64F[k][i][j].real, m_fftLocRef64F[k][i][j].imag);
			}
		}
	}
	fprintf(fp, "\n\n\n\n\n\n");
	for (k = 0; k < DETECTION_PATCH; k ++) {
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				fprintf(fp, "(%f,%f)\n", m_fftTarget64F[k][i][j].real, m_fftLocRef64F[k][i][j].imag);
			}
		}
	}
	fclose(fp);
*/
	bFlag = FALSE;
	for (k = 0; k < DETECTION_PATCH; k ++) {
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				real = m_fftLocRef64F[k][i][j].real*m_fftTarget64F[k][i][j].real + m_fftLocRef64F[k][i][j].imag*m_fftTarget64F[k][i][j].imag;
				imag = m_fftLocRef64F[k][i][j].imag*m_fftTarget64F[k][i][j].real - m_fftLocRef64F[k][i][j].real*m_fftTarget64F[k][i][j].imag;
				m_fftLocRef64F[k][i][j].real = real;
				m_fftLocRef64F[k][i][j].imag = imag;
			}
		}
		// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
		FFT2D(m_fftLocRef64F[k], fftWidth, fftHeight, nx, ny, -1);

		// retrieve real part of    fft(x).*conj(fft(y))
		max_x = max_y = -1000000000;
		dMax = -1000000000;
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				if (dMax < m_fftLocRef64F[k][i][j].real) {
					dMax  = m_fftLocRef64F[k][i][j].real;
					max_x = i;
					max_y = j;
				}
			}
		}

		coef  = dMax / coef_peak;
		if (coef < COEF_THRESHOLD) {
			bFlag = TRUE;
		} else {
			max_x = (max_x>=fftWidth/2)  ? max_x-fftWidth  : max_x;
			max_y = (max_y>=fftHeight/2) ? max_y-fftHeight : max_y;
			if (abs(max_x) > THRESHOLD_LX || abs(max_y) > THRESHOLD_LY) bFlag = TRUE;
		}

		// copy the target FFT to the local reference frame
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				m_fftLocRef64F[k][i][j].real = m_fftTarget64F[k][i][j].real;
				m_fftLocRef64F[k][i][j].imag = m_fftTarget64F[k][i][j].imag;
			}
		}
	}

	if (bFlag == TRUE)
		return STAB_SACCADE_BLINK;
	else
		return STAB_SUCCESS;
}
/*
int CStabFFT::SaccaBlinkDetectionL(float coef_peak) 
{
	int   i, k, fftWidth, fftHeight, nx, ny, max_x, max_y;
	float real, imag, dMax, coef;
	BOOL  bFlag;

	fftWidth  = m_nCenX064;
	fftHeight = m_nCenY064;
	nx        = m_nIdxX064;
	ny        = m_nIdxY064;


	bFlag = FALSE;
	for (k = 0; k < DETECTION_PATCH; k ++) {
		for (i = 0; i < fftWidth*fftHeight; i ++) {
			real = m_fftLocRef64FRe[k][i]*m_fftTarget64FRe[k][i] + m_fftLocRef64FIm[k][i]*m_fftTarget64FIm[k][i];
			imag = m_fftLocRef64FIm[k][i]*m_fftTarget64FRe[k][i] - m_fftLocRef64FRe[k][i]*m_fftTarget64FIm[k][i];
			m_fftLocRef64FRe[k][i] = real;
			m_fftLocRef64FIm[k][i] = imag;
		}
		// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
		FFT2D(m_fftLocRef64FRe[k], m_fftLocRef64FIm[k], fftWidth, fftHeight, nx, ny, -1);

		// retrieve real part of    fft(x).*conj(fft(y))
		max_x = max_y = -1000000000;
		dMax = -1000000000;
		for (i = 0; i < fftWidth*fftHeight; i ++) {
			if (dMax < m_fftLocRef64FRe[k][i]) {
				dMax  = m_fftLocRef64FRe[k][i];
				max_x = i%fftWidth;
				max_y = i/fftWidth;
			}
		}

		coef  = dMax / coef_peak;
		if (coef < COEF_THRESHOLD) {
			bFlag = TRUE;
		} else {
			max_x = (max_x>=fftWidth/2)  ? max_x-fftWidth  : max_x;
			max_y = (max_y>=fftHeight/2) ? max_y-fftHeight : max_y;
			if (abs(max_x) > THRESHOLD_X || abs(max_y) > THRESHOLD_Y) bFlag = TRUE;
		}

		// copy the target FFT to the local reference frame
		memcpy(m_fftLocRef64FRe[k], m_fftTarget64FRe[k], fftWidth*fftHeight*sizeof(float));
		memcpy(m_fftLocRef64FIm[k], m_fftTarget64FIm[k], fftWidth*fftHeight*sizeof(float));
	}

	if (bFlag == TRUE)
		return STAB_SACCADE_BLINK;
	else
		return STAB_SUCCESS;
}
*/
int CStabFFT::GetCenterXY(float coef_peak1, float coef_peak2, BOOL jumpFlag, int ofsXOld, int ofsYOld, int *ofsX, int *ofsY)
{
	int    nx, ny, i, j, m, n, fftWidth, fftHeight, max_x, max_y, offset;
	float  dMax, coef, real, imag, coef_peak;

	if (jumpFlag == TRUE) {	
		// the previous frame is a saccade/blink frame, so the central patch height is doubled to accormadate possible target jump.
		coef_peak = coef_peak2;
		fftWidth  = m_nCenX256;
		fftHeight = m_nCenY256;
		nx        = m_nIdxX256;
		ny        = m_nIdxY256;

		// -------------------------------------------------
		// calculate central patch of target with 256 pixels of height
		// -------------------------------------------------
		// copy raw images to two patches
		offset  = m_imgWidth*(m_imgHeight-m_nCenY256)/2 + (m_imgWidth-m_nCenX256)/2;
		for (j = 0; j < m_nCenY256; j ++) {
			n = offset + j*m_imgWidth;
			m = j*m_nCenX256;
			for (i = 0; i < m_nCenX256; i ++) {
				m_fftTarget256[i][j].real = m_imgPre[n+i];
				m_fftTarget256[i][j].imag = 0;
			}
		}
		// do forward 2-D FFT on a patch of the target frame
		FFT2D(m_fftTarget256, m_nCenX256, m_nCenY256, m_nIdxX256, m_nIdxY256, 1);

		// do central patch calculation
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				real = m_fftGloRef256[i][j].real*m_fftTarget256[i][j].real + m_fftGloRef256[i][j].imag*m_fftTarget256[i][j].imag;
				imag = m_fftGloRef256[i][j].imag*m_fftTarget256[i][j].real - m_fftGloRef256[i][j].real*m_fftTarget256[i][j].imag;
				m_fftTarget256[i][j].real = real;
				m_fftTarget256[i][j].imag = imag;
			}
		}
		
		// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
		FFT2D(m_fftTarget256, fftWidth, fftHeight, nx, ny, -1);

		// retrieve real part of the result
		max_x = max_y = -1000000000;
		dMax = -1000000000;
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				if (dMax < m_fftTarget256[i][j].real) {
					dMax  = m_fftTarget256[i][j].real;
					max_x = i;
					max_y = j;
				}
			}
		}
	} else {
		// the previous frame is in drifting mode, and only small patch height is needed here to reduce calculation
		coef_peak = coef_peak1;
		fftWidth  = m_nCenX128;
		fftHeight = m_nCenY128;
		nx        = m_nIdxX128;
		ny        = m_nIdxY128;

		// -------------------------------------------------
		// calculate central patch of target with 128 pixels of height
		// -------------------------------------------------
		// copy raw images to two patches
		offset  = m_imgWidth*((m_imgHeight-m_nCenY128)/2-ofsYOld) + (m_imgWidth-m_nCenX128)/2-ofsXOld;
		for (j = 0; j < m_nCenY128; j ++) {
			n = offset + j*m_imgWidth;
			m = j*m_nCenX128;
			for (i = 0; i < m_nCenX128; i ++) {
				m_fftTarget128[i][j].real = m_imgPre[n+i];
				m_fftTarget128[i][j].imag = 0;
			}
		}
		// do forward 2-D FFT on a patch of the target frame
		FFT2D(m_fftTarget128, m_nCenX128, m_nCenY128, m_nIdxX128, m_nIdxY128, 1);


		// do central patch calculation
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				real = m_fftGloRef128[i][j].real*m_fftTarget128[i][j].real + m_fftGloRef128[i][j].imag*m_fftTarget128[i][j].imag;
				imag = m_fftGloRef128[i][j].imag*m_fftTarget128[i][j].real - m_fftGloRef128[i][j].real*m_fftTarget128[i][j].imag;
				m_fftTarget128[i][j].real = real;
				m_fftTarget128[i][j].imag = imag;
			}
		}
		
		// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
		FFT2D(m_fftTarget128, fftWidth, fftHeight, nx, ny, -1);

		// retrieve real part of the result
		max_x = max_y = -1000000000;
		dMax = -1000000000;
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				if (dMax < m_fftTarget128[i][j].real) {
					dMax  = m_fftTarget128[i][j].real;
					max_x = i;
					max_y = j;
				}
			}
		}
	}


	coef  = dMax / coef_peak;
	if (coef < COEF_THRESHOLD) {
		*ofsX = -m_imgWidth;
		*ofsY = -m_imgHeight;
	} else {
		max_x = (max_x>=fftWidth/2)  ? max_x-fftWidth  : max_x;
		max_y = (max_y>=fftHeight/2) ? max_y-fftHeight : max_y;
		*ofsX = max_x + ofsXOld;
		*ofsY = max_y + ofsYOld;
	}

	return STAB_SUCCESS;
}

/*
int CStabFFT::GetCenterXYL(float coef_peak1, float coef_peak2, BOOL jumpFlag, int ofsXOld, int ofsYOld, int *ofsX, int *ofsY)
{
	int    nx, ny, i, fftWidth, fftHeight, max_x, max_y;
	float  dMax, coef, real, imag, coef_peak;

	if (jumpFlag == TRUE) {
		coef_peak = coef_peak2;
		fftWidth  = m_nCenX256;
		fftHeight = m_nCenY256;
		nx        = m_nIdxX256;
		ny        = m_nIdxY256;

		// do central patch calculation
		for (i = 0; i < fftHeight*fftWidth; i ++) {
			real = m_fftGloRef256Re[i]*m_fftTarget256Re[i] + m_fftGloRef256Im[i]*m_fftTarget256Im[i];
			imag = m_fftGloRef256Im[i]*m_fftTarget256Re[i] - m_fftGloRef256Re[i]*m_fftTarget256Im[i];
			m_fftTarget256Re[i] = real;
			m_fftTarget256Im[i] = imag;
		}
		
		// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
		FFT2D(m_fftTarget256Re, m_fftTarget256Im, fftWidth, fftHeight, nx, ny, -1);

		// retrieve real part of the result
		max_x = max_y = -1000000000;
		dMax = -1000000000;
		for (i = 0; i < fftWidth*fftHeight; i ++) {
			if (dMax < m_fftTarget256Re[i]) {
				dMax  = m_fftTarget256Re[i];
				max_x = i%fftWidth;
				max_y = i/fftWidth;
			}
		}
	} else {
		coef_peak = coef_peak1;
		fftWidth  = m_nCenX128;
		fftHeight = m_nCenY128;
		nx        = m_nIdxX128;
		ny        = m_nIdxY128;

		// do central patch calculation
		for (i = 0; i < fftWidth*fftHeight; i ++) {
			real = m_fftGloRef128Re[i]*m_fftTarget128Re[i] + m_fftGloRef128Im[i]*m_fftTarget128Im[i];
			imag = m_fftGloRef128Im[i]*m_fftTarget128Re[i] - m_fftGloRef128Re[i]*m_fftTarget128Im[i];
			m_fftTarget128Re[i] = real;
			m_fftTarget128Im[i] = imag;
		}
		
		// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
		FFT2D(m_fftTarget128Re, m_fftTarget128Im, fftWidth, fftHeight, nx, ny, -1);

		// retrieve real part of the result
		max_x = max_y = -1000000000;
		dMax = -1000000000;
		for (i = 0; i < fftWidth*fftHeight; i ++) {
			if (dMax < m_fftTarget128Re[i]) {
				dMax  = m_fftTarget128Re[i];
				max_x = i%fftWidth;
				max_y = i/fftWidth;
			}
		}
	}


	coef  = dMax / coef_peak;
	if (coef < COEF_THRESHOLD) {
		*ofsX = -m_imgWidth;
		*ofsY = -m_imgHeight;
	} else {
		max_x = (max_x>=fftWidth/2)  ? max_x-fftWidth  : max_x;
		max_y = (max_y>=fftHeight/2) ? max_y-fftHeight : max_y;
		*ofsX = max_x + ofsXOld;
		*ofsY = max_y + ofsYOld;
	}

	return STAB_SUCCESS;
}
*/

int CStabFFT::GetTargetXY(float coef_peak, int centerX, int centerY, int *deltaX, int *deltaY, int patchCnt)
{
	int    nx, ny, i, j, k, n, fftWidth, fftHeight, max_x, max_y, ub, offset;
	float  dMax, coef, real, imag;

	fftWidth  = m_nCenX064;
	fftHeight = m_nCenY064;
	nx        = m_nIdxX064;
	ny        = m_nIdxY064;

//	BYTE *ref, *tar;
//	ref = new BYTE [m_nCenX064*m_nCenY064];
//	tar = new BYTE [m_nCenX064*m_nCenY064];

	// -------------------------------------------------
	// calculate FFT of target&reference patches for motion detection
	// -------------------------------------------------
	ub = m_imgWidth*m_imgHeight;
	for (k = 0; k < m_nPatchCnt; k ++) {
		// process target patch
		offset = m_imgWidth*m_ofsY[k] + (m_imgWidth-m_nCenX064)/2;
		for (j = 0; j < m_nCenY064; j ++) {
			n = offset + j*m_imgWidth;
			for (i = 0; i < m_nCenX064; i ++) {
				m_fftTarget64P[k][i][j].real = m_imgPre[n+i];
				m_fftTarget64P[k][i][j].imag = 0;
				//tar[j*m_nCenX064+i] = m_imgPre[n+i];
			}
		}
		FFT2D(m_fftTarget64P[k], m_nCenX064, m_nCenY064, m_nIdxX064, m_nIdxY064, 1);

		// process reference patch
		//centerX=centerY=0;
		offset = m_imgWidth*(m_ofsY[k]+centerY) + (m_imgWidth-m_nCenX064)/2+centerX;	// check boundary
		for (j = 0; j < m_nCenY064; j ++) {
			n = offset + j*m_imgWidth;
			for (i = 0; i < m_nCenX064; i ++) {
				if (n+i >= ub || n+i < 0)
					m_fftGloRef64P[k][i][j].real = 0;
				else
					m_fftGloRef64P[k][i][j].real = m_imgRef[n+i];
				m_fftGloRef64P[k][i][j].imag = 0;
				//ref[j*m_nCenX064+i] = m_imgRef[n+i];
			}
		}
		FFT2D(m_fftGloRef64P[k], m_nCenX064, m_nCenY064, m_nIdxX064, m_nIdxY064, 1);

		// do dot product of reference and target
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				real = m_fftGloRef64P[k][i][j].real*m_fftTarget64P[k][i][j].real + m_fftGloRef64P[k][i][j].imag*m_fftTarget64P[k][i][j].imag;
				imag = m_fftGloRef64P[k][i][j].imag*m_fftTarget64P[k][i][j].real - m_fftGloRef64P[k][i][j].real*m_fftTarget64P[k][i][j].imag;
				m_fftTarget64P[k][i][j].real = real;
				m_fftTarget64P[k][i][j].imag = imag;
			}
		}
		// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
		FFT2D(m_fftTarget64P[k], fftWidth, fftHeight, nx, ny, -1);

		// retrieve real part of    fft(x).*conj(fft(y))
		max_x = max_y = -1000000000;
		dMax = -1000000000;
		for (j = 0; j < fftHeight; j ++) {
			for (i = 0; i < fftWidth; i ++) {
				if (dMax < m_fftTarget64P[k][i][j].real) {
					dMax  = m_fftTarget64P[k][i][j].real;
					max_x = i;
					max_y = j;
				}
			}
		}

		coef  = dMax / coef_peak;
		if (coef < COEF_THRESHOLD) {
			deltaX[k] = -m_imgWidth;
			deltaY[k] = -m_imgHeight;
		} else {
			max_x = (max_x>=fftWidth/2)  ? max_x-fftWidth  : max_x;
			max_y = (max_y>=fftHeight/2) ? max_y-fftHeight : max_y;
			if (abs(max_x) > THRESHOLD_LX || abs(max_y) > THRESHOLD_LY) {
				deltaX[k] = -m_imgWidth;
				deltaY[k] = -m_imgHeight;
			} else {
				deltaX[k] = max_x + centerX;
				deltaY[k] = max_y + centerY;
			}
		}
	}

	return STAB_SUCCESS;
}
/*
int CStabFFT::GetTargetXYL(float coef_peak, int centerX, int centerY, int *deltaX, int *deltaY, int patchCnt)
{
	int    nx, ny, i, j, k, n, m, fftWidth, fftHeight, max_x, max_y, ub, offset;
	float  dMax, coef, real, imag;

	fftWidth  = m_nCenX064;
	fftHeight = m_nCenY064;
	nx        = m_nIdxX064;
	ny        = m_nIdxY064;

	// -------------------------------------------------
	// calculate FFT of target&reference patches for motion detection
	// -------------------------------------------------
	ub = m_imgWidth*m_imgHeight;
	for (k = 0; k < m_nPatchCnt; k ++) {
		// process target patch
		offset = m_imgWidth*m_ofsY[k] + (m_imgWidth-m_nCenX064)/2;
		for (j = 0; j < m_nCenY064; j ++) {
			n = offset + j*m_imgWidth;
			m = j*m_nCenX064;
			for (i = 0; i < m_nCenX064; i ++) {
				m_fftTarget64PRe[k][m+i] = m_imgPre[n+i];
				m_fftTarget64PIm[k][m+i] = 0;
			}
		}
		FFT2D(m_fftTarget64PRe[k], m_fftTarget64PIm[k], m_nCenX064, m_nCenY064, m_nIdxX064, m_nIdxY064, 1);

		// process reference patch
		offset = m_imgWidth*(m_ofsY[k]+centerY) + (m_imgWidth-m_nCenX064)/2+centerX;	// check boundary
		for (j = 0; j < m_nCenY064; j ++) {
			n = offset + j*m_imgWidth;
			m = j*m_nCenX064;
			for (i = 0; i < m_nCenX064; i ++) {
				if (n+i >= ub || n+i < 0)
					m_fftGloRef64PRe[k][m+i] = 0;
				else
					m_fftGloRef64PRe[k][m+i] = m_imgRef[n+i];
				m_fftGloRef64PIm[k][m+i] = 0;
			}
		}
		FFT2D(m_fftGloRef64PRe[k], m_fftGloRef64PIm[k], m_nCenX064, m_nCenY064, m_nIdxX064, m_nIdxY064, 1);

		// do dot product of reference and target
		for (i = 0; i < fftWidth*fftHeight; i ++) {
			real = m_fftGloRef64PRe[k][i]*m_fftTarget64PRe[k][i] + m_fftGloRef64PIm[k][i]*m_fftTarget64PIm[k][i];
			imag = m_fftGloRef64PIm[k][i]*m_fftTarget64PRe[k][i] - m_fftGloRef64PRe[k][i]*m_fftTarget64PIm[k][i];
			m_fftTarget64PRe[k][i] = real;
			m_fftTarget64PIm[k][i] = imag;
		}
		// do inverse 2D FFT on the result of fft(x).*conj(fft(y))
		FFT2D(m_fftTarget64PRe[k], m_fftTarget64PIm[k], fftWidth, fftHeight, nx, ny, -1);

		// retrieve real part of    fft(x).*conj(fft(y))
		max_x = max_y = -1000000000;
		dMax = -1000000000;
		for (i = 0; i < fftWidth*fftHeight; i ++) {
			if (dMax < m_fftTarget64PRe[k][i]) {
				dMax  = m_fftTarget64PRe[k][i];
				max_x = i%fftWidth;
				max_y = i/fftWidth;
			}
		}

		coef  = dMax / coef_peak;
		if (coef < COEF_THRESHOLD) {
			deltaX[k] = -m_imgWidth;
			deltaY[k] = -m_imgHeight;
		} else {
			max_x = (max_x>=fftWidth/2)  ? max_x-fftWidth  : max_x;
			max_y = (max_y>=fftHeight/2) ? max_y-fftHeight : max_y;
			if (abs(max_x) > THRESHOLD_X || abs(max_y) > THRESHOLD_Y) {
				deltaX[k] = -m_imgWidth;
				deltaY[k] = -m_imgHeight;
			} else {
				deltaX[k] = max_x + centerX;
				deltaY[k] = max_y + centerY;
			}
		}
	}

	return STAB_SUCCESS;
}
*/
/*--------------------------------------------------------------------*/
/*  This computes an in-place complex-to-complex FFT                  */
/*  x and y are the real and imaginary arrays of 2^m points.          */
/*  dir =  1 gives forward transform                                  */
/*  dir = -1 gives reverse transform                                  */
/*                                                                    */
/*    Formula: forward                                                */
/*                 N-1                                                */
/*                 ---                                                */
/*             1   \          - j k 2 pi n / N                        */
/*     X(n) = ---   >   x(k) e                    = forward transform */
/*             N   /                                n=0..N-1          */
/*                 ---                                                */
/*                 k=0                                                */
/*                                                                    */
/*     Formula: reverse                                               */
/*                 N-1                                                */
/*                 ---                                                */
/*                 \          j k 2 pi n / N                          */
/*     X(n) =       >   x(k) e                    = forward transform */
/*                 /                                n=0..N-1          */
/*                 ---                                                */
/*                 k=0                                                */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  1 dimensional Fast Fourier Transform                              */ 
/*                                                                    */
/*  parameters:                                                       */
/*    1. direction of FFT, integer, (IN),                             */
/*       dir=1 forward FFT, dir=-1 inverse FFT                        */
/*    2. length of the array, integer                                 */
/*    3. real part of the array, (I/O)                                */
/*    4. imaginary part of the array, (I/O)                           */
/*                                                                    */
/*--------------------------------------------------------------------*/
int CStabFFT::FFT(int dir,int m,float *x,float *y)
{
	long nn,i,i1,j,k,i2,l,l1,l2;
	float c1,c2,tx,ty,t1,t2,u1,u2,z;

	/* Calculate the number of points */
	nn = 1;
	for (i=0;i<m;i++)
		nn *= 2;

	/* Do the bit reversal */
	i2 = nn >> 1;
	j = 0;
	for (i=0;i<nn-1;i++) {
		if (i < j) {
			tx = x[i];
			ty = y[i];
			x[i] = x[j];
			y[i] = y[j];
			x[j] = tx;
			y[j] = ty;
		}
		k = i2;
		while (k <= j) {
			j -= k;
			k >>= 1;
		}
		j += k;
	}

	/* Compute the FFT */
	c1 = -1.0;
	c2 = 0.0;
	l2 = 1;
	for (l=0;l<m;l++) {
		l1 = l2;
		l2 <<= 1;
		u1 = 1.0;
		u2 = 0.0;
		for (j=0;j<l1;j++) {
			for (i=j;i<nn;i+=l2) {
				i1 = i + l1;
				t1 = u1 * x[i1] - u2 * y[i1];
				t2 = u1 * y[i1] + u2 * x[i1];
				x[i1] = x[i] - t1;
				y[i1] = y[i] - t2;
				x[i] += t1;
				y[i] += t2;
			}
			z =  u1 * c1 - u2 * c2;
			u2 = u1 * c2 + u2 * c1;
			u1 = z;
		}
		c2 = sqrt((1.0f - c1) / 2.0f);
		if (dir == 1)
			c2 = -c2;
		c1 = sqrt((1.0f + c1) / 2.0f);
	}

	/* Scaling for forward transform */
	if (dir == 1) {
		for (i=0;i<nn;i++) {
			x[i] /= (float)nn;
			y[i] /= (float)nn;
		}
	}

	return STAB_SUCCESS;
}


/*------------------------------------------------------------------*/
/*  2 dimensional Fast Fourier Transform                            */ 
/*                                                                  */
/*  parameters:                                                     */
/*    1. 2-D image array, (IN/OUT)                                  */
/*    2. width of the image, integer                                */
/*    3. height of the image, integer                               */
/*    4. power of dimension, integer                                */
/*    5. direction of FFT, integer, (IN),                           */
/*       dir=1 forward FFT, dir=-1 inverse FFT                      */
/*                                                                  */
/*------------------------------------------------------------------*/
int CStabFFT::FFT2D(COMPLEX **c, int nx, int ny, int m, int n, int dir)
{
	int i,j;
	float *real,*imag;

	/* Transform the rows */
	real = (float *)malloc(nx * sizeof(float));
	imag = (float *)malloc(nx * sizeof(float));
	if (real == NULL || imag == NULL) {
		return STAB_MEM_OVERFLOW;
	}
	
	for (j=0;j<ny;j++) {
		for (i=0;i<nx;i++) {
			real[i] = c[i][j].real;
			imag[i] = c[i][j].imag;
		}
		FFT(dir,m,real,imag);
		for (i=0;i<nx;i++) {
			c[i][j].real = real[i];
			c[i][j].imag = imag[i];
		}
	}
	free(real);
	free(imag);

	/* Transform the columns */
	real = (float *)malloc(ny * sizeof(float));
	imag = (float *)malloc(ny * sizeof(float));
	if (real == NULL || imag == NULL) {
		return STAB_MEM_OVERFLOW;
	}

	for (i=0;i<nx;i++) {
		for (j=0;j<ny;j++) {
			real[j] = c[i][j].real;
			imag[j] = c[i][j].imag;
		}
		FFT(dir,n,real,imag);
		for (j=0;j<ny;j++) {
			c[i][j].real = real[j];
			c[i][j].imag = imag[j];
		}
	}

	free(real);
	free(imag);

	return STAB_SUCCESS;
}

/*
int CStabFFT::FFT2D(float *imgRe, float *imgIm, int nx, int ny, int m, int n, int dir)
{
	int    i, j, k;
	float *real,*imag;

	// Transform the rows 
	real = (float *)malloc(nx * sizeof(float));
	imag = (float *)malloc(nx * sizeof(float));
	if (real == NULL || imag == NULL) {
		return STAB_MEM_OVERFLOW;
	}
	
	for (j=0;j<ny;j++) {
		k = nx*j;
		for (i=0;i<nx;i++) {
			real[i] = imgRe[i+k];
			imag[i] = imgIm[i+k];
		}
		FFT(dir,m,real,imag);
		for (i=0;i<nx;i++) {
			imgRe[i+k] = real[i];
			imgIm[i+k] = imag[i];
		}
	}
	free(real);
	free(imag);

	// Transform the columns 
	real = (float *)malloc(ny * sizeof(float));
	imag = (float *)malloc(ny * sizeof(float));
	if (real == NULL || imag == NULL) {
		return STAB_MEM_OVERFLOW;
	}

	for (i=0;i<nx;i++) {
		//k = ny*i;
		for (j=0;j<ny;j++) {
			real[j] = imgRe[nx*j+i];
			imag[j] = imgIm[nx*j+i];
		}
		FFT(dir,n,real,imag);
		for (j=0;j<ny;j++) {
			imgRe[nx*j+i] = real[j];
			imgIm[nx*j+i] = imag[j];
		}
	}

	free(real);
	free(imag);

	return STAB_SUCCESS;
}
*/


// check if FFT width is a power of 2
int CStabFFT::CheckSizeFFT(int size, int *power2) {
	int rem, idx;
	
	rem = size;
	idx = 0;
	do {
		rem = rem >> 1;
		idx ++;
	} while (rem > 0);
	*power2 = idx-1;
	rem = 1;
	do {
		rem = rem << 1;
		idx --;
	} while (idx > 1);
	if (rem != size) return STAB_INVALID_FFT_SIZE;

	return STAB_SUCCESS;
}


void CStabFFT::FastGauss(BYTE *imgDest, BYTE *imgSrc, int imgW, int imgH, int filterIdx)
{
	short *kernelX, *kernelY;
	int    knx, kny, i, imgSize, ksum, temp;

	if (filterIdx == 1 || filterIdx == 0 ) {
		imgSize = imgW*imgH;
		for (i = 0; i < imgSize; i ++) m_imgIn[i] = imgSrc[i];

		if (filterIdx == 0) {
			knx = kny = 3;		// 3x3 Gauss
			kernelX = new short[knx];
			kernelY = new short[kny];
			kernelX[0] = 1; kernelX[1] = 2; kernelX[2] = 1;
			kernelY[0] = 1; kernelY[1] = 2; kernelY[2] = 1;
			ksum = 16;
		} else {
			knx = kny = 5;		// 5x5 Gauss
			kernelX = new short[knx];
			kernelY = new short[kny];
			kernelX[0] = 1; kernelX[1] = 2; kernelX[2] = 4; kernelX[3] = 2; kernelX[4] = 1; 
			kernelY[0] = 1; kernelY[1] = 2; kernelY[2] = 4; kernelY[3] = 2; kernelY[4] = 1; 
			ksum = 100;
		}
		convolve2DSeparable(m_imgIn, m_imgOut, imgW, imgH, kernelX, knx, kernelY, kny);
		for (i = 0; i < imgSize; i ++) {
			temp = (int)(m_imgOut[i]*1.0/ksum);
			imgDest[i] = (temp>255) ? 255 : temp;
		}

		delete [] kernelX;
		delete [] kernelY;
	} else {
		memcpy(imgDest, imgSrc, imgW*imgH);
	}
}


void CStabFFT::FastGauss(BYTE *imgDest, BYTE *imgSrc, int imgW, int imgH, int filterIdx, int pid, int pcnt, int pheight)
{
	short *kernelX, *kernelY;
	int    knx, kny, i, imgSize, ksum, temp, offset, margin;

	if ((pid+1)*pheight > imgH) return;

	if (filterIdx == 0) {
		if (pid == 0) {
			margin = 1;
			offset  = 0;
		} else if (pid == pcnt-1) {
			margin = 1;
			offset  = (pid*pheight-1)*imgW;
		} else {
			margin = 2;
			offset  = (pid*pheight-1)*imgW;
		}
		imgSize = (pheight+margin)*imgW;
		for (i = offset; i < offset+imgSize; i ++) m_imgIn[i] = imgSrc[i];
		
		knx = kny = 3;		// 3x3 Gauss
		kernelX = new short[knx];
		kernelY = new short[kny];
		kernelX[0] = 1; kernelX[1] = 2; kernelX[2] = 1;
		kernelY[0] = 1; kernelY[1] = 2; kernelY[2] = 1;
		ksum = 16;
		convolve2DSeparable(m_imgIn+offset, m_imgOut+offset, imgW, pheight+margin, kernelX, knx, kernelY, kny);
		
		imgSize = pheight*imgW;
		offset  = pid*pheight*imgW;
		for (i = offset; i < offset+imgSize; i ++) {
			temp = (int)(m_imgOut[i]*1.0/ksum);
			imgDest[i] = (temp>255) ? 255 : temp;
		}
		delete [] kernelX;
		delete [] kernelY;
	} else if (filterIdx == 1) {
		if (pid == 0) {
			margin = 2;
			offset  = 0;
		} else if (pid == pcnt-1) {
			margin = 2;
			offset  = (pid*pheight-2)*imgW;
		} else {
			margin = 4;
			offset  = (pid*pheight-2)*imgW;
		}
		imgSize = (pheight+margin)*imgW;
		for (i = offset; i < offset+imgSize; i ++) m_imgIn[i] = imgSrc[i];
		
		knx = kny = 5;		// 5x5 Gauss
		kernelX = new short[knx];
		kernelY = new short[kny];
		kernelX[0] = 1; kernelX[1] = 2; kernelX[2] = 4; kernelX[3] = 2; kernelX[4] = 1; 
		kernelY[0] = 1; kernelY[1] = 2; kernelY[2] = 4; kernelY[3] = 2; kernelY[4] = 1; 
		ksum = 100;
		convolve2DSeparable(m_imgIn+offset, m_imgOut+offset, imgW, pheight+margin, kernelX, knx, kernelY, kny);
		
		imgSize = pheight*imgW;
		offset  = pid*pheight*imgW;
		for (i = offset; i < offset+imgSize; i ++) {
			temp = (int)(m_imgOut[i]*1.0/ksum);
			imgDest[i] = (temp>255) ? 255 : temp;
		}
		delete [] kernelX;
		delete [] kernelY;
	} else {
		imgSize = pheight*imgW;
		offset  = pid*pheight*imgW;
		memcpy(imgDest+offset, imgSrc+offset, imgSize);
	}
}


bool CStabFFT::convolve2DSeparable(short* in, short* out, int dataSizeX, int dataSizeY, 
                         short* kernelX, int kSizeX, short* kernelY, int kSizeY)
{
    int i, j, k, m, n;
    short *tmp, *sum;                               // intermediate data buffer
    short *inPtr, *outPtr;                            // working pointers
    short *tmpPtr, *tmpPtr2;                        // working pointers
    int kCenter, kOffset, endIndex;                 // kernel indice

    // check validity of params
    if(!in || !out || !kernelX || !kernelY) return false;
    if(dataSizeX <= 0 || kSizeX <= 0) return false;

    // allocate temp storage to keep intermediate result
    tmp = new short[dataSizeX * dataSizeY];
    if(!tmp) return false;  // memory allocation error

    // store accumulated sum
    sum = new short[dataSizeX];
    if(!sum) return false;  // memory allocation error

    // covolve horizontal direction ///////////////////////

    // find center position of kernel (half of kernel size)
    kCenter = kSizeX >> 1;                          // center index of kernel array
    endIndex = dataSizeX - kCenter;                 // index for full kernel convolution

    // init working pointers
    inPtr = in;
    tmpPtr = tmp;                                   // store intermediate results from 1D horizontal convolution

    // start horizontal convolution (x-direction)
    for(i=0; i < dataSizeY; ++i)                    // number of rows
    {

        kOffset = 0;                                // starting index of partial kernel varies for each sample

        // COLUMN FROM index=0 TO index=kCenter-1
        for(j=0; j < kCenter; ++j)
        {
            *tmpPtr = 0;                            // init to 0 before accumulation

            for(k = kCenter + kOffset, m = 0; k >= 0; --k, ++m) // convolve with partial of kernel
            {
                *tmpPtr += *(inPtr + m) * kernelX[k];
            }
            ++tmpPtr;                               // next output
            ++kOffset;                              // increase starting index of kernel
        }

        // COLUMN FROM index=kCenter TO index=(dataSizeX-kCenter-1)
        for(j = kCenter; j < endIndex; ++j)
        {
            *tmpPtr = 0;                            // init to 0 before accumulate

            for(k = kSizeX-1, m = 0; k >= 0; --k, ++m)  // full kernel
            {
                *tmpPtr += *(inPtr + m) * kernelX[k];
            }
            ++inPtr;                                // next input
            ++tmpPtr;                               // next output
        }

        kOffset = 1;                                // ending index of partial kernel varies for each sample

        // COLUMN FROM index=(dataSizeX-kCenter) TO index=(dataSizeX-1)
        for(j = endIndex; j < dataSizeX; ++j)
        {
            *tmpPtr = 0;                            // init to 0 before accumulation

            for(k = kSizeX-1, m=0; k >= kOffset; --k, ++m)   // convolve with partial of kernel
            {
                *tmpPtr += *(inPtr + m) * kernelX[k];
            }
            ++inPtr;                                // next input
            ++tmpPtr;                               // next output
            ++kOffset;                              // increase ending index of partial kernel
        }

        inPtr += kCenter;                           // next row
    }
    // END OF HORIZONTAL CONVOLUTION //////////////////////

    // start vertical direction ///////////////////////////

    // find center position of kernel (half of kernel size)
    kCenter = kSizeY >> 1;                          // center index of vertical kernel
    endIndex = dataSizeY - kCenter;                 // index where full kernel convolution should stop

    // set working pointers
    tmpPtr = tmpPtr2 = tmp;
    outPtr = out;

    // clear out array before accumulation
    for(i = 0; i < dataSizeX; ++i)
        sum[i] = 0;

    // start to convolve vertical direction (y-direction)

    // ROW FROM index=0 TO index=(kCenter-1)
    kOffset = 0;                                    // starting index of partial kernel varies for each sample
    for(i=0; i < kCenter; ++i)
    {
        for(k = kCenter + kOffset; k >= 0; --k)     // convolve with partial kernel
        {
            for(j=0; j < dataSizeX; ++j)
            {
                sum[j] += *tmpPtr * kernelY[k];
                ++tmpPtr;
            }
        }

        for(n = 0; n < dataSizeX; ++n)              // convert and copy from sum to out
        {
//            if(sum[n] >= 0)
  //              *outPtr = (int)(sum[n] + 0.5f);     // store final result to output array
    //        else
      //          *outPtr = (int)(sum[n] - 0.5f);     // store final result to output array
			*outPtr = sum[n];

            sum[n] = 0;                             // reset to zero for next summing
            ++outPtr;                               // next element of output
        }

        tmpPtr = tmpPtr2;                           // reset input pointer
        ++kOffset;                                  // increase starting index of kernel
    }

    // ROW FROM index=kCenter TO index=(dataSizeY-kCenter-1)
    for(i = kCenter; i < endIndex; ++i)
    {
        for(k = kSizeY -1; k >= 0; --k)             // convolve with full kernel
        {
            for(j = 0; j < dataSizeX; ++j)
            {
                sum[j] += *tmpPtr * kernelY[k];
                ++tmpPtr;
            }
        }

        for(n = 0; n < dataSizeX; ++n)              // convert and copy from sum to out
        {
            //if(sum[n] >= 0)
            //    *outPtr = (int)(sum[n] + 0.5f);     // store final result to output array
            //else
            //    *outPtr = (int)(sum[n] - 0.5f);     // store final result to output array
			*outPtr = sum[n];

            sum[n] = 0;                             // reset to 0 before next summing
            ++outPtr;                               // next output
        }

        // move to next row
        tmpPtr2 += dataSizeX;
        tmpPtr = tmpPtr2;
    }

    // ROW FROM index=(dataSizeY-kCenter) TO index=(dataSizeY-1)
    kOffset = 1;                                    // ending index of partial kernel varies for each sample
    for(i=endIndex; i < dataSizeY; ++i)
    {
        for(k = kSizeY-1; k >= kOffset; --k)        // convolve with partial kernel
        {
            for(j=0; j < dataSizeX; ++j)
            {
                sum[j] += *tmpPtr * kernelY[k];
                ++tmpPtr;
            }
        }

        for(n = 0; n < dataSizeX; ++n)              // convert and copy from sum to out
        {
            //if(sum[n] >= 0)
            //    *outPtr = (int)(sum[n] + 0.5f);     // store final result to output array
            //else
            //    *outPtr = (int)(sum[n] - 0.5f);     // store final result to output array
			*outPtr = sum[n];

            sum[n] = 0;                             // reset before next summing
            ++outPtr;                               // next output
        }

        // move to next row
        tmpPtr2 += dataSizeX;
        tmpPtr = tmpPtr2;                           // next input
        ++kOffset;                                  // increase ending index of kernel
    }
    // END OF VERTICAL CONVOLUTION ////////////////////////

    // deallocate temp buffers
    delete [] tmp;
    delete [] sum;
    return true;
}



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

int CStabFFT::GetCUDADeviceCounts(int *counts)
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
//	return 0;
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
	return Call_CUDA_SaccadeDetectionK(coef_peak, patchIdx, sx, sy);
}

int CStabFFT::GetCenterCUDA(float coef_peak1, float coef_peak2, BOOL jumpFlag, int ofsXOld, int ofsYOld, int *ofsX, int *ofsY) {
	bool flag = (jumpFlag) ? true : false;

	return Call_CUDA_GetCenter(coef_peak1, coef_peak2, flag, ofsXOld, ofsYOld, ofsX, ofsY);
}

int CStabFFT::GetPatchXY(int BlockID, int centerX, int centerY, int *deltaX, int *deltaY) {
	return Call_CUDA_GetPatchXY(m_CorrPeak064, BlockID, centerX, centerY, deltaX, deltaY);
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

