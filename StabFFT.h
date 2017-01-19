#include <math.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include "windows.h"

#define STAB_OUT_OF_BOTTOM       2
#define STAB_OUT_OF_TOP          1
#define STAB_SUCCESS             0
#define STAB_INVALID_IMG_SIZE   -1
#define STAB_INVALID_FFT_SIZE   -2
#define STAB_INVALID_BUFFER     -3
#define STAB_MEM_OVERFLOW       -4
#define STAB_NO_REFERENCE       -5
#define STAB_SACCADE_BLINK      -6
#define STAB_AUTO_CORR_ERR      -7
#define STAB_PATCH_FFT_ERR      -8
#define STAB_PEAK_COEF_ERR      -9
#define STAB_PATCH_FAIL         -10
#define STAB_LOW_COEFF          -11

#define CUDA_FFT_SUCCESS         0
#define CUDA_FFT_SACCADE        -1
#define CUDA_FFT_CENT_FAIL      -2
#define CUDA_FFT_MEM_ERR        -3
#define CUDA_FFT_OTHER_ERR      -4

#define DETECTION_PATCH  4  // run four patches for saccade/blink detection
#define COEF_THRESHOLD   0.4
#define THRESHOLD_LX     100
#define THRESHOLD_LY      14
#define THRESHOLD_SX      10
#define THRESHOLD_SY      10
#define MOTION_TS_SX      8
#define MOTION_TS_SY      8

typedef struct {
	float real;
	float imag;
} COMPLEX;

typedef struct 		//active APERTURE struct
{
	int xmin;
	int xmax;
	int ymin;
	int ymax;
} APERTURE;

typedef struct {
	int  imgWidth;
	int  imgHeight;
	int  cenFFTx1;
	int  cenFFTy1;
	int  cenFFTx2;
	int  cenFFTy2;
	int  fineX;
	int  fineY;
	int  patchCnt;
	int *osY;
	int  convKernel;
	int  frameIndex;
	int  UseCUDA;
	int  slice_height;
} STAB_PARAMS;

#include "utils/TrackKernel.h"

class CStabFFT
{
private:
	COMPLEX ***m_fftGloRef64P;		// save FFT of PCTCH_CNT patches on global reference frame
	COMPLEX ***m_fftTarget64P;		// save FFT of PCTCH_CNT patches on target frame
	COMPLEX ***m_fftLocRef64F;		// save FFT of 4 patches on local reference frame for saccade/blink detection
	COMPLEX ***m_fftTarget64F;		// save FFT of 4 patches on target frame for saccade/blink detection
	
	COMPLEX **m_fftGloRef128;		
	COMPLEX **m_fftGloRef256;
	COMPLEX **m_fftTarget128;		
	COMPLEX **m_fftTarget256;
/*
	float **m_fftGloRef64PRe;		// save FFT of PCTCH_CNT patches on global reference frame
	float **m_fftTarget64PRe;		// save FFT of PCTCH_CNT patches on target frame
	float **m_fftLocRef64FRe;		// save FFT of 4 patches on local reference frame for saccade/blink detection
	float **m_fftTarget64FRe;		// save FFT of 4 patches on target frame for saccade/blink detection
	float **m_fftGloRef64PIm;		// save FFT of PCTCH_CNT patches on global reference frame
	float **m_fftTarget64PIm;		// save FFT of PCTCH_CNT patches on target frame
	float **m_fftLocRef64FIm;		// save FFT of 4 patches on local reference frame for saccade/blink detection
	float **m_fftTarget64FIm;		// save FFT of 4 patches on target frame for saccade/blink detection
	
	float *m_fftGloRef128Re;		
	float *m_fftGloRef256Re;
	float *m_fftTarget128Re;		
	float *m_fftTarget256Re;
	float *m_fftGloRef128Im;		
	float *m_fftGloRef256Im;
	float *m_fftTarget128Im;		
	float *m_fftTarget256Im;
*/
	BYTE  *m_imgRef;
	BYTE  *m_imgPre;
	short *m_imgIn;
	short *m_imgOut;

	int    m_imgWidth;
	int    m_imgHeight;
	float  m_coef_peak064;
	float  m_coef_peak128;
	float  m_coef_peak256;
	int    m_nCenX128;
	int    m_nCenY128;
	int    m_nCenX256;
	int    m_nCenY256;
	int    m_nCenX064;
	int    m_nCenY064;
	int    m_nIdxX128;
	int    m_nIdxY128;
	int    m_nIdxX256;
	int    m_nIdxY256;
	int    m_nIdxX064;
	int    m_nIdxY064;
	int    m_nPatchCnt;
	BOOL   m_bGloRefFFT;
	int   *m_ofsY;

	BOOL   m_bSaccadeBlink;
	float  m_CorrPeak256;
	float  m_CorrPeak128;
	float  m_CorrPeak064;

	int    m_dx_old;
	int    m_dy_old;
	int    m_refYsold;
	int    m_tarYsold;

	int  FFT(int dir, int m, float *x, float *y);
	int  FFT2D(COMPLEX **c, int nx, int ny, int m, int n, int dir);
//	int  FFT2D(float *imgRe, float *imgIm, int nx, int ny, int m, int n, int dir);
	int  CheckSizeFFT(int size, int *power2);

protected: // create from serialization only

public:
	void FastGauss(BYTE *imgDest, BYTE *imgSrc, int imgW, int imgH, int filterIdx);
	void FastGauss(BYTE *imgDest, BYTE *imgSrc, int imgW, int imgH, int filterIdx, int pid, int pcnt, int pheight);
	bool convolve2DSeparable(short* in, short* out, int dataSizeX, int dataSizeY, short* kernelX, int kSizeX, short* kernelY, int kSizeY);
	CStabFFT();
	~CStabFFT();

	int  StabInit(STAB_PARAMS stab_params);
	int  ImagePatchFFT(BYTE *image, int ofsX, int ofsY, BOOL bGlobalRef);
	int  GetPeakCoefs(float *coef_peak256, float *coef_peak128, float *coef_peak064);
	int  SaccaBlinkDetection(float coef_peak);
	int  GetCenterXY(float coef_peak1, float coef_peak2, BOOL jumpFlag, int ofsXOld, int ofsYOld, int *ofsX, int *ofsY);
	int  GetTargetXY(float coef_peak, int centerX, int centerY, int *deltaX, int *deltaY, int patchCnt);
//	int  StabInitL(int imgWidth, int imgHeight, int cenFFTx1, int cenFFTy1, int cenFFTx2, int cenFFTy2, int fineX, int fineY, int patchCnt, int *osY);
//	int  ImagePatchFFTL(BYTE *image, int ofsX, int ofsY, BOOL bGlobalRef, BOOL bJumpFlag);
//	int  GetPeakCoefsL(float *coef_peak256, float *coef_peak128, float *coef_peak064);
//	int  SaccaBlinkDetectionL(float coef_peak);
//	int  GetCenterXYL(float coef_peak1, float coef_peak2, BOOL jumpFlag, int ofsXOld, int ofsYOld, int *ofsX, int *ofsY);
//	int  GetTargetXYL(float coef_peak, int centerX, int centerY, int *deltaX, int *deltaY, int patchCnt);

	int  ChooseCUDADevice(int deviceID);
	int  GetCUDADeviceCounts(int *counts);
	int  GetCUDADeviceNames(int deviceID, char *name);
	int  CUDA_FFTinit(STAB_PARAMS stab_params);
	int  CUDA_FFTrelease();
	int  ImagePatchCUDA_K(int ofsX, int ofsY, BOOL bGlobalRef, int patchIdx);
	int  GetPeakCoefsCUDA(float *coef_peak256, float *coef_peak128, float *coef_peak064);
	int  SaccadeDetectionCUDA_K(float coef_peak, int patchIdx, int *sx, int *sy);
	int  GetCenterCUDA(float coef_peak1, float coef_peak2, BOOL jumpFlag, int ofsXOld, int ofsYOld, int *ofsX, int *ofsY);
	int  FastConvCUDA(unsigned char *imgO, unsigned char *imgI, int imgW, int imgH, int KernelID, BOOL ref, BOOL fil);
	int  FastConvCUDA(unsigned char *imgO, unsigned char *imgI, int imgW, int imgH, int KernelID, BOOL ref, BOOL eof, int pid, int pcnt, int pheight, BOOL fil);

	int  SaccadeDetectionK(int frameIndex, int patchIdx, int *sx, int *sy);
	BOOL CalcCentralMotion(int cxOld, int cyOld, BOOL wideOpen, int *cxNew, int *cyNew);
	int  GetPatchXY(int BlockID, int centerX, int centerY, int *deltaX, int *deltaY);
};