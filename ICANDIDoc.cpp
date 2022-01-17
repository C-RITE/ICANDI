// ICANDIDoc.cpp : implementation of the CICANDIDoc class
//

#include "stdafx.h"
#include "ICANDI.h"

#include "ViewRawVideo.h"
#include "ICANDIDoc.h"
#include "ICANDIParamsDlg.h"
#include "CalDesinu.h"
#include "mmsystem.h"
#include <fstream>
#include "matrix.h"
#include "utils/x64/slr.h"
#include <iostream>

#include "ICANDIParams.h"
#include "StabFFT.h"

#define	 TIMING_FLAG_ON  FALSE

const int  dewarp_x = 100;
const int  dewarp_y = 100;
const BYTE STIMULUS_SIZE_X = 16;
const BYTE STIMULUS_SIZE_Y = 16;
const int  LINES_PER_STIMULUS = 32;		// line span per stimulus in the case of stimuli projection

unsigned short  g_iStimulusSizeX_RD;
unsigned short  g_iStimulusSizeX_GR;
unsigned short  g_iStimulusSizeX_IR;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if _DEBUG
std::ofstream messageLogFile;
#endif
// define 4 views for displaying videos and other messages
CView        *g_viewMsgVideo;		// pointer to main control view
CView        *g_viewRawVideo;		// pointer to raw video view
CView        *g_viewDewVideo;		// pointer to dewarped video view
CView        *g_viewDltVideo;		// pointer to delta (stimulus position) view

BOOL		  g_bConstPwr;			// stimulus constant power or constant size?
BOOL          g_bZeroGainFlag;
BOOL          g_bHistgram;
BOOL          g_bStimulusOn;        // turn On/Off stimulus beam
BOOL		  g_bMarkStimulus;
BOOL          g_bFFTIsRunning;		// flag indicating FFT is running or not
BOOL		  g_bRecord;	//12/15/2011
UINT          g_frameIndex;			// counter indicating frame number processed by FFT
ICANDIParams  g_ICANDIParams;			// struct for storing application parameters
AOSLO_MOVIE   aoslo_movie;
PatchParams   g_PatchParamsA;		// parameters for handling matrox block-level sampling
PatchParams   g_PatchParamsB;		// parameters for handling matrox block-level sampling

POINT         g_StimulusPos0Gain;		// stimulus position at gain 0, temporary added by Qiang
POINT         g_StimulusPos;		// stimulus position on the stabilized video
POINT         g_StimulusPos0G;
POINT         g_StimulusPosBak;		// stimulus position on the stabilized video
POINT	      g_nStimPos;
POINT	      g_nStimPosBak_Matlab;
POINT	      g_nStimPosBak_Matlab_Retreive; //05/04/2012
BOOL		  g_bGain0Tracking;		//Track stimulus when gain is 0 (on/off) 10/21/2011
POINT		  g_nStimPosBak_Gain0Track;
int			  g_nBrightness;
int			  g_nContrast;

long          g_sampling_counter;		// counter indicating frame number sampled by Matrox
int           g_BlockCounts;
long          g_BlockIndex;
UINT          LINE_INTERVAL;
UINT          g_nViewID;

BOOL          g_bTimingTest;
FILE         *g_fpTimeLog;
LARGE_INTEGER g_ticksPerSecond;

CAnimateCtrl *g_AnimateCtrl;
BOOL		  g_bPlaybackUpdate;

unsigned short*			g_usRed_LUT;
unsigned short*			g_usGreen_LUT;
unsigned short*			g_usIR_LUT;
//Laser control variables
double      g_dRedMax;
double      g_dGreenMax;
double      g_dIRMax;
int         g_ncurRedPos;
int         g_ncurGreenPos;
int         g_ncurIRPos;
BOOL		g_bWritePsyStimulus;

HANDLE		g_eRunSLR;
BOOL		g_bRunSLR;
HANDLE		g_eSyncOCT;
BOOL		g_bSyncOCT;
BOOL		g_bSyncOCTReady;
BOOL		g_bValidSignal[33];

//matlab conrtol parameters
BOOL		g_bMatlabAVIsavevideo;
BOOL		g_bMatlab_Trigger; //flag for SOF handler to update the stimulus based on the sequence from Matlab
BOOL		g_bMatlabCtrl; //indicates the recording function that the request came from Matlab
BOOL		g_bMatlabVideo;
short		g_sMatlab_Update_Ind[3]; //new index of the stimulus to update, received from Matlab
BOOL		g_bMatlab_Update; //indicates the SOF handler to update stimulus based on Matlab requirements
BOOL		g_bMatlab_Loop; //flag for SOF handler to update the stimulus in continous loop based on the sequence from Matlab
unsigned short	g_nSequence_Matlab[3][1800]; //stimulus frame sequence to present
float		g_fGain_Matlab[1800]; //gain of stimulus tracking
short		g_nLocX[3][1800];
short		g_nLocY[3][1800];
BYTE		g_ubAngle_Matlab[1800]; //angle of stimulus rotation
BYTE		g_ubStimPresFlg_Matlab[1800];
unsigned short	g_nIRPower_Matlab[1800];
unsigned short	g_nRedPower_Matlab[1800];
unsigned short	g_nGreenPower_Matlab[1800];

short		g_nCurFlashCount;
short		g_nFlashCount;
CPoint		g_cpSubStimDim_Matlab;
CPoint		g_cpSubStimOffs_Matlab;
BOOL		g_bDynStimBkgnd_Matlab;
unsigned short	*g_pDynStim_Matlab;

// these global parameters handle Virtex-5 BMD
BOOL              g_bKernelPlugin;
CVirtex5BMD       g_objVirtex5BMD;
WDC_DEVICE_HANDLE g_hDevVirtex5;
BOOL              g_bFirstVsync;
BOOL              g_bEOF;
DWORD             g_dwTotalCount;
DIAG_DMA          g_dma;
VIDEO_INFO        g_VideoInfo;
FILE             *g_fp;

BOOL              g_bEOFHandler;
HANDLE            g_EventEOB;
HANDLE            g_EventEOF;
HANDLE            g_EventLoadStim;
HANDLE            g_EventLoadLUT;
HANDLE            g_EventReadStimVideo;
HANDLE            g_EventSendViewMsg;
HANDLE			  *g_EventNetCommMsgAO;
HANDLE			  *g_EventNetCommMsgAOM;
HANDLE			  g_EventStimPresentFlag;
int               g_BlockID;
float             g_qxCenter;
float             g_qyCenter;
BOOL              g_bPreCritical;
BOOL              g_bCritical;
BOOL              g_bMultiStimuli;
BOOL			  g_bStimProjSel;
BOOL              g_bStimProjection;
BOOL              g_bStimProjectionIR;
BOOL              g_bStimProjectionRD;
BOOL              g_bStimProjectionGR;
BOOL			  g_bIRstim;
BOOL			  g_bGRstim;
BOOL			  g_bRDstim;
BOOL			  g_bDrawCrossMarker;	// added for a better control of stimulus-cross
BOOL              g_bCalcStimPos;

// define the shift amount of stimulus location on channel 1. 
// This is the Red channel and the shift amount is limited to |x|<=32 and |y|<=32.
POINT             g_Channel1Shift;		// only for stimulus
// define the shift amount of stimulus location on channel 2. 
// This is the Green channel and the shift amount is limited to |x|<=32 and |y|<=32.
POINT             g_Channel2Shift;		// only for stimulus

POINT	g_RGBClkShiftsAuto[3];
POINT	g_RGBClkShiftsUser[3];

BOOL			  g_bAutoMeasureTCA;
BOOL			  g_bTCAOverride;

//Motion scaler variables
float			  g_Motion_ScalerX;	//volts per 6181 pixels in X direction
float			  g_Motion_ScalerY;	//volts per 6181 pixels in Y direction
int				  g_InvertMotionTraces;

STAB_PARAMS   g_StabParams;
CStabFFT      g_objStabFFT;			// object for using GPU-based FFT
BOOL          g_bSaccadeFlag;		// saccade flag for the current frame
BOOL          g_bSaccadeFlagPrev;	// saccade flag for the previous frame
BOOL          g_bNoGPUdevice;
//BOOL          g_bFirstPhysicalStripe;

FILE*		temptest;
// The imaging channel (IR) signal will be output to the blue channel of the on-board DAC,
// and the stimulus location will be a 8x8 square modulated on the rectangular wave

// StimDelivery: Linear Stretching global definition

int* linStretch;



void g_GetAppSystemTime(int *hours, int *minutes, int *seconds, double *milliseconds) {
	int           l_hours;
	int           l_minutes;
	int           l_seconds;
	double        l_milliseconds;
	LARGE_INTEGER l_time;
	LARGE_INTEGER l_tick;

	QueryPerformanceCounter(&l_tick);
	l_time.QuadPart = l_tick.QuadPart/g_ticksPerSecond.QuadPart;
	l_hours = (int)(l_time.QuadPart/3600);
	l_time.QuadPart = l_time.QuadPart - (l_hours * 3600);
	l_minutes = (int)(l_time.QuadPart/60);
	l_seconds = (int)(l_time.QuadPart - (l_minutes * 60));			
	l_milliseconds = (double) (1000.0 * (l_tick.QuadPart % g_ticksPerSecond.QuadPart) / g_ticksPerSecond.QuadPart);

	*hours        = l_hours;
	*minutes      = l_minutes;
	*seconds      = l_seconds;
	*milliseconds = l_milliseconds;
}

/*
    This routine interpolates linearly the movements (x and y) of each 
    patche on the grids of an image.

    parameters:
       1. in:      positions of patches, in the order of 
                   [X1, X2, ..., Xn, Y1, Y2, ...,Yn] 
       2. patches: number of patches                    
       3. out:     interpolated movement, in horizontal or vertical 
       4. len:     width of height of image
       5. offset:  for interpolating x- or y- movement. This parameter
                   doesn't have to be used, but "if... then..." is 
                   waived here
    
 */
int g_Interp1(float *in, int patches, float *out, int len, int offset) {
	int   i, j;
	int   os_top, os_bottom;

	float da, db;

	if (len < patches) return -1;

	os_top = 0;//g_ICANDIParams.L_YDIM[0]/2;

	os_bottom = len - os_top - 1;

	for (i = 0; i < os_top; i ++) out[i] = in[offset+1];
	for (i = os_bottom; i < len; i ++) out[i] = in[offset+patches];

	da = (float)((patches-1.0000001) / (os_bottom-os_top));
	for (i = os_top; i < os_bottom; i ++) {
		db = da * (i - os_top);
		j = (int)db;
		out[i] = in[j+offset+1]*(1-db+j) + in[j+offset+2]*(db-j);
	}

	return 0;
}

int g_Interp1D(int *in, int patches, float *out, int len) {
	int   i, j;
	float da, db;

	if (len < patches) return -1;

	da = (float)((patches-1.0000001) / (len-1));
	for (i = 0; i < len; i ++) {
		db = da * i;
		j = (int)db;
		out[i] = in[j]*(j+1-db) + in[j+1]*(db-j);
	}

	return 0;
}

/*
    This routine does image deparping

    parameters:
        1.  imgI:     original warped image
        2.  imgO:     dewrapped image
        3.  u:        motion in X direction 
        4.  v:        motion in Y direction 
		5.  fraID:    frame ID, only for testing purpose
 */
int g_Dewarp(BYTE *imgI, BYTE *imgO, float *u, float *v, int fraID) {
	int   i, j, k, Yold, fXj, fYj, idx1, idx2;
	int   nx, ny, nnx, nny, p, q, r;
	int   offsetX, offsetY;
	float Xj, Yj;

	offsetX = dewarp_x;
	offsetY = dewarp_y;

	nx = aoslo_movie.width;
	ny = aoslo_movie.height;
	nnx = nx + dewarp_x*2;
	nny = ny + dewarp_y*2; 

	Yold = (int)(floor(-v[0]) + offsetY);
	for (j = 0; j < ny; j ++) {
		Xj =   - u[j] + offsetX;
		Yj = j - v[j] + offsetY;

		fXj = (int)floor(Xj);
		fYj = (int)floor(Yj);

		if (fXj > nnx-2 || fXj < 1 || fYj > nny-2 || fYj < 1) {
			//AfxMessageBox("Dewarping fails.");
			return -1;
			//continue;
		}
		
		p = j*nx;
		q = fYj*nnx;

		if (abs(Yold-fYj) <= 1) {
			for (k = 0; k < nx; k ++) {
				idx1 = k + p;
				if (fXj+k>=0 && fXj+k<nnx) {
					idx2 = fXj + k + q;
					imgO[idx2] = imgI[idx1];
				}
			}
		} else {
			if (Yold > fYj) {
				for (i = fYj; i <= Yold-1; i ++) {
					r = i*nnx;
					for (k = 0; k < nx; k ++) {
						idx1 = k + p;
						if (fXj+k>=0 && fXj+k<nnx) {
							idx2 = fXj+k + r;
							imgO[idx2] = imgI[idx1];
						}
					}
				}
			} else {
				for (i = Yold+1; i <= fYj; i ++) {
					r = i*nnx;
					for (k = 0; k < nx; k ++) {
						idx1 = k + p;
						if (fXj+k>=0 && fXj+k<nnx) {
							idx2 = fXj + k + r;
							imgO[idx2] = imgI[idx1];
						}
					}
				}
			}
		}
		Yold = fYj;
	}

	return 0;
}


/*
   Read lookup table for desinusoiding raw image sampled directly from Matrox grabber

   Parameters:
       1. fWidth:      image width
	   2. fHeight:     image height
	   3. UnMatrixIdx: Column for indice
	   4. UnMatrixVal: Column for values

   Return:
		True:  succeed
		False: fail
 */
BOOL g_ReadLUT(int fWidth, int fHeight, int *UnMatrixIdx, float *UnMatrixVal, float *StretchFactor) {
	FILE    *fp;
	int     *LUTheader, ret;
	CString  msg;

	msg = g_ICANDIParams.fnameLUT;
	msg.TrimLeft(' ');
	msg.TrimRight(' ');
	if (msg.GetLength() == 0) {
		AfxMessageBox("Error: no lookup table file name is loaded. Please go to menu [Setup]->[FFT Parameters] or [Setup]->[Desinusoid] to load a lookup table file", MB_ICONEXCLAMATION);
		g_bFFTIsRunning = FALSE;
		return FALSE;
	}

	LUTheader = new int [2];
	fopen_s(&fp, g_ICANDIParams.fnameLUT, "rb");
	if (fp == NULL) {
		msg.Format("Error: can't find file <%s>", g_ICANDIParams.fnameLUT);
		AfxMessageBox(msg, MB_ICONEXCLAMATION);
//		fclose(fp);
		g_bFFTIsRunning = FALSE;
		return FALSE;
	}

	// read lookup table size
	ret = fread(LUTheader, sizeof(int), 2, fp);
	if (LUTheader[0] != fWidth || LUTheader[1] != fHeight) {
		msg.Format("Error: the look up table shows different video size [%,%d] to the loaded video file [%d,%d]", LUTheader[0], LUTheader[1], fWidth, fHeight);
		AfxMessageBox(msg, MB_ICONEXCLAMATION);
		fclose(fp);
		g_bFFTIsRunning = FALSE;
		return FALSE;
	}

	// read lookup table index
	ret = fread(UnMatrixIdx, sizeof(int), (fWidth+fHeight)*2, fp);
	if (ret != (fWidth+fHeight)*2) {
		AfxMessageBox("Error: data in this lookup table are not correct", MB_ICONEXCLAMATION);
		fclose(fp);
		g_bFFTIsRunning = FALSE;
		return FALSE;
	}

	// read lookup table values
	ret = fread(UnMatrixVal, sizeof(float), (fWidth+fHeight)*2, fp);
	if (ret != (fWidth+fHeight)*2) {
		AfxMessageBox("Error: data in this lookup table are not correct", MB_ICONEXCLAMATION);
		fclose(fp);
		g_bFFTIsRunning = FALSE;
		return FALSE;
	}

	// read stretch factor
	ret = fread(StretchFactor, sizeof(float), fWidth, fp);
	if (ret != fWidth) {
		AfxMessageBox("Error: data in this lookup table are not correct", MB_ICONEXCLAMATION);
		fclose(fp);
		g_bFFTIsRunning = FALSE;
		return FALSE;
	}

	fclose(fp);
	delete [] LUTheader;

	return TRUE;
}

// add a stimulus witness on the video
void g_MarkStimulusPos()
{
	if (g_ICANDIParams.WriteMarkFlags[8]) {
		int    x, y, xt, yt, dewarp_sx, dewarp_sy, i, idx, iSize;
		float  uj, vj;
		BOOL   bDrawnonzeroGain = FALSE;

		dewarp_sx = (aoslo_movie.dewarp_sx - aoslo_movie.width) / 2;
		dewarp_sy = (aoslo_movie.dewarp_sy - aoslo_movie.height) / 2;
		x = (g_StimulusPos.x > 0 ? g_StimulusPos.x: ((g_StimulusPos0G.x > 0 && !aoslo_movie.fStabGainStim) ? g_StimulusPos0G.x:0)) - dewarp_sx;		// position X on the reference frame
		y = (g_StimulusPos.y > 0 ? g_StimulusPos.y: ((g_StimulusPos0G.y > 0 && !aoslo_movie.fStabGainStim) ? g_StimulusPos0G.y:0)) - dewarp_sy;		// position Y on the reference frame
		iSize = aoslo_movie.width * aoslo_movie.height;

		if (x < 0 || y < 0) {
			//	x = dewarp_sx;
			//	y = dewarp_sy;
			return;
		}
		if (y < aoslo_movie.height) {
			if (aoslo_movie.fStabGainStim != 1.0) { // draw for not equal to 1.0 gain indicating IR
				uj = aoslo_movie.StimulusDeltaX*aoslo_movie.fStabGainStim;
				vj = aoslo_movie.StimulusDeltaY*aoslo_movie.fStabGainStim;
				//		uj = aoslo_movie.StimulusDeltaX;
				//		vj = aoslo_movie.StimulusDeltaY;			
				// Red
				if (g_ICANDIParams.WriteMarkFlags[2] && aoslo_movie.bRedAomOn && aoslo_movie.nLaserPowerRed) {// && (g_Channel1Shift.x != 0 || g_Channel1Shift.y != 0 || g_ICANDIParams.ApplyTCA)) {
					switch (g_ICANDIParams.FRAME_ROTATION) {
						case 0:
							xt = x + (int)(uj + g_ICANDIParams.RGBClkShifts[0].x + g_Channel1Shift.x);					// translation X of the target frame
							yt = y + (int)(vj - g_ICANDIParams.RGBClkShifts[0].y - g_Channel1Shift.y);					// translation Y of the target frame
							break;
						case 1:
							xt = x + (int)(uj - g_ICANDIParams.RGBClkShifts[0].y - g_Channel1Shift.y);					// translation X of the target frame
							yt = y + (int)(vj + g_ICANDIParams.RGBClkShifts[0].x + g_Channel1Shift.x);					// translation Y of the target frame
							break;
						case 2:
							xt = x + (int)(uj - g_ICANDIParams.RGBClkShifts[0].y - g_Channel1Shift.y);					// translation X of the target frame
							yt = y + (int)(vj + g_ICANDIParams.RGBClkShifts[0].x - g_Channel1Shift.x);					// translation Y of the target frame
							break;
						default:;
					}
					for (i = yt-5; i <= yt+5; i ++) {
						idx = i*aoslo_movie.height + xt;
						if (idx >= 0 && idx < iSize) {
							aoslo_movie.video_in[idx] = 253;
							aoslo_movie.video_ins[idx] = 253;
						}
					}
					for (i = xt-5; i <= xt+5; i ++) {
						idx = yt*aoslo_movie.height + i;
						if (idx >= 0 && idx < iSize) {
							aoslo_movie.video_in[idx] = 253;
							aoslo_movie.video_ins[idx] = 253;
						}
					}
				}

				// Green
			if (g_ICANDIParams.WriteMarkFlags[1] && aoslo_movie.bGrAomOn && aoslo_movie.nLaserPowerGreen) {// && (g_Channel2Shift.x != 0 || g_Channel2Shift.y != 0 || g_ICANDIParams.ApplyTCA)) {
				switch (g_ICANDIParams.FRAME_ROTATION) {
					case 0:
						xt = x + (int)(uj + g_ICANDIParams.RGBClkShifts[1].x + g_Channel2Shift.x);					// translation X of the target frame
						yt = y + (int)(vj - g_ICANDIParams.RGBClkShifts[1].y - g_Channel2Shift.y);					// translation Y of the target frame (maybe wrong? use line below?)
						//yt = y + (int)(vj + g_ICANDIParams.RGBClkShifts[1].y + g_Channel2Shift.y);					// translation Y of the target frame
						break;
					case 1:
						xt = x + (int)(uj - g_ICANDIParams.RGBClkShifts[1].y - g_Channel2Shift.y);					// translation X of the target frame
						yt = y + (int)(vj + g_ICANDIParams.RGBClkShifts[1].x + g_Channel2Shift.x);					// translation Y of the target frame
						break;
					case 2:
						xt = x + (int)(uj - g_ICANDIParams.RGBClkShifts[1].y - g_Channel2Shift.y);					// translation X of the target frame
						yt = y + (int)(vj + g_ICANDIParams.RGBClkShifts[1].x - g_Channel2Shift.x);					// translation Y of the target frame
						break;
					default:;
				}
					for (i = yt-5; i <= yt+5; i ++) {
						idx = i*aoslo_movie.height + xt;
						if (idx >= 0 && idx < iSize) {
							aoslo_movie.video_in[idx] = 251;
							aoslo_movie.video_ins[idx] = 251;
						}
					}
					for (i = xt-5; i <= xt+5; i ++) {
						idx = yt*aoslo_movie.height + i;
						if (idx >= 0 && idx < iSize) {
							aoslo_movie.video_in[idx] = 251;
							aoslo_movie.video_ins[idx] = 251;
						}
					}
				}

				// IR
				xt = x + (int)uj;					// translation X of the target frame
				yt = y + (int)vj;					// translation Y of the target frame
				if (g_ICANDIParams.WriteMarkFlags[3] && aoslo_movie.bIRAomOn) {
					for (i = yt-5; i <= yt+5; i ++) {
						idx = i*aoslo_movie.height + xt;
						if (idx >= 0 && idx < iSize) {
							aoslo_movie.video_in[idx] = 255;
							aoslo_movie.video_ins[idx] = 255;
						}
					}
					for (i = xt-5; i <= xt+5; i ++) {
						idx = yt*aoslo_movie.height + i;
						if (idx >= 0 && idx < iSize) {
							aoslo_movie.video_in[idx] = 255;
							aoslo_movie.video_ins[idx] = 255;
						}
					}
				}
				if (g_bGain0Tracking) {
					if (aoslo_movie.fStabGainStim != 0.) {
						g_nStimPosBak_Gain0Track.x = xt + dewarp_sx;
						g_nStimPosBak_Gain0Track.y = yt + dewarp_sy;
					}
				}
				bDrawnonzeroGain = TRUE;
			}

			//Draw for Gain = 1. IR stimulus
			uj = aoslo_movie.StimulusDeltaX;
			vj = aoslo_movie.StimulusDeltaY;		

			// Red
			if (g_ICANDIParams.WriteMarkFlags[6] && aoslo_movie.bRedAomOn && aoslo_movie.nLaserPowerRed) {// && (g_Channel1Shift.x != 0 || g_Channel1Shift.y != 0 || g_ICANDIParams.ApplyTCA)) {
				switch (g_ICANDIParams.FRAME_ROTATION) {
						case 0:
							xt = x + (int)(uj + g_ICANDIParams.RGBClkShifts[0].x + g_Channel1Shift.x);					// translation X of the target frame
							yt = y + (int)(vj - g_ICANDIParams.RGBClkShifts[0].y - g_Channel1Shift.y);					// translation Y of the target frame
							break;
						case 1:
							xt = x + (int)(uj - g_ICANDIParams.RGBClkShifts[0].y - g_Channel1Shift.y);					// translation X of the target frame
							yt = y + (int)(vj + g_ICANDIParams.RGBClkShifts[0].x + g_Channel1Shift.x);					// translation Y of the target frame
							break;
						case 2:
							xt = x + (int)(uj - g_ICANDIParams.RGBClkShifts[0].y - g_Channel1Shift.y);					// translation X of the target frame
							yt = y + (int)(vj + g_ICANDIParams.RGBClkShifts[0].x - g_Channel1Shift.x);					// translation Y of the target frame
							break;
						default:;
					}
				for (i = yt-5; i <= yt+5; i ++) {
					idx = i*aoslo_movie.height + xt;
					if (idx >= 0 && idx < iSize) {
						aoslo_movie.video_in[idx] = bDrawnonzeroGain?252:253;
						aoslo_movie.video_ins[idx] = bDrawnonzeroGain?252:253;
					}
				}
				for (i = xt-5; i <= xt+5; i ++) {
					idx = yt*aoslo_movie.height + i;
					if (idx >= 0 && idx < iSize) {
						aoslo_movie.video_in[idx] = bDrawnonzeroGain?252:253;
						aoslo_movie.video_ins[idx] = bDrawnonzeroGain?252:253;
					}
				}
			}
			// Green
			if (g_ICANDIParams.WriteMarkFlags[5] && aoslo_movie.bGrAomOn && aoslo_movie.nLaserPowerGreen) {// && (g_Channel2Shift.x != 0 || g_Channel2Shift.y != 0 || g_ICANDIParams.ApplyTCA)) {
				switch (g_ICANDIParams.FRAME_ROTATION) {
					case 0:
						xt = x + (int)(uj + g_ICANDIParams.RGBClkShifts[1].x + g_Channel2Shift.x);					// translation X of the target frame
						yt = y + (int)(vj + g_ICANDIParams.RGBClkShifts[1].y + g_Channel2Shift.y);					// translation Y of the target frame
						//yt = y + (int)(vj - g_ICANDIParams.RGBClkShifts[1].y - g_Channel2Shift.y);					// translation Y of the target frame: wrong!!
						break;
					case 1:
						xt = x + (int)(uj - g_ICANDIParams.RGBClkShifts[1].y - g_Channel2Shift.y);					// translation X of the target frame
						yt = y + (int)(vj + g_ICANDIParams.RGBClkShifts[1].x + g_Channel2Shift.x);					// translation Y of the target frame
						break;
					case 2:
						xt = x + (int)(uj - g_ICANDIParams.RGBClkShifts[1].y - g_Channel2Shift.y);					// translation X of the target frame
						yt = y + (int)(vj + g_ICANDIParams.RGBClkShifts[1].x - g_Channel2Shift.x);					// translation Y of the target frame
						break;
					default:;
				}
				for (i = yt-5; i <= yt+5; i ++) {
					idx = i*aoslo_movie.height + xt;
					if (idx >= 0 && idx < iSize) {
						aoslo_movie.video_in[idx] = bDrawnonzeroGain?250:251;
						aoslo_movie.video_ins[idx] = bDrawnonzeroGain?250:251;
					}
				}
				for (i = xt-5; i <= xt+5; i ++) {
					idx = yt*aoslo_movie.height + i;
					if (idx >= 0 && idx < iSize) {
						aoslo_movie.video_in[idx] = bDrawnonzeroGain?250:251;
						aoslo_movie.video_ins[idx] = bDrawnonzeroGain?250:251;
					}
				}
			}

			// IR
			xt = x + (int)uj;					// translation X of the target frame
			yt = y + (int)vj;					// translation Y of the target frame
			if (g_ICANDIParams.WriteMarkFlags[7] && aoslo_movie.bIRAomOn) {
				for (i = yt-5; i <= yt+5; i ++) {
					idx = i*aoslo_movie.height + xt;
					if (idx >= 0 && idx < iSize) {
						aoslo_movie.video_in[idx] = bDrawnonzeroGain?254:255;
						aoslo_movie.video_ins[idx] = bDrawnonzeroGain?254:255;
					}
				}
				for (i = xt-5; i <= xt+5; i ++) {
					idx = yt*aoslo_movie.height + i;
					if (idx >= 0 && idx < iSize) {
						aoslo_movie.video_in[idx] = bDrawnonzeroGain?254:255;
						aoslo_movie.video_ins[idx] = bDrawnonzeroGain?254:255;
					}
				}
			}
			if (g_bGain0Tracking && aoslo_movie.fStabGainStim != 0 && !bDrawnonzeroGain) {
				g_nStimPosBak_Gain0Track.x = xt + dewarp_sx;
				g_nStimPosBak_Gain0Track.y = yt + dewarp_sy;
			}
			aoslo_movie.WriteMarkFlag = FALSE;
		}
	}
	g_bDrawCrossMarker = FALSE;
}

void g_WritePsyStimulus()
{
	int    x, y, i, j, idx, iSize;
	
	iSize = aoslo_movie.width * aoslo_movie.height;	
	x = 255;	
	y = 500;
	for (i = y-5; i<= y+5; i++)
		for (j = x-5; j<= x+5; j++)
		{
			idx = i*aoslo_movie.width + j;
			if (idx >= 0 && idx < iSize) {
				aoslo_movie.video_in[idx] = 0;
				aoslo_movie.video_ins[idx] = 0;
			}
		}
}
// end of frame
void g_DiagDmaIntHandler(WDC_DEVICE_HANDLE hDev, VIRTEX5_INT_RESULT *pIntResult)
{
//	g_bEOF = TRUE;
//	g_sampling_counter ++;
//	g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, g_sampling_counter);
}


void g_MemCpyDev2Host(PVOID mem_i, unsigned char *mem_o, int len, int LineOffset) {
	int        i, j, k1, k2, ofx, ofy;
	BOOL       qw_shift;

	if (LineOffset >= 0 && LineOffset <= g_VideoInfo.img_height - g_VideoInfo.line_spacing) {
//		for (i = 0; i < g_VideoInfo.img_width*g_VideoInfo.line_spacing; i ++) {
//			*(mem_o+i+LineOffset*g_VideoInfo.img_width) = ((unsigned char *)(mem_i))[i];
//		}

		qw_shift = FALSE;
		for (j = 4; j < g_VideoInfo.line_spacing-4; j ++) {
			k2 = j * g_VideoInfo.img_width;
			for (i = g_VideoInfo.img_width-4; i < g_VideoInfo.img_width-2; i ++) {
				if (((unsigned char *)(mem_i))[k2+i] < 200) {
					qw_shift = TRUE;
					break;
				}
			}
		}

//		if (qw_shift == TRUE) fprintf(g_fp, "Quad-Word shift. frame ID = %d\n", g_sampling_counter);

		//ofx = (g_VideoInfo.img_width - aoslo_movie.width) / 2;
		ofx = qw_shift == TRUE ? 12 : 4;
		ofy = (g_VideoInfo.img_height - aoslo_movie.height) / 2;
		for (j = 0; j < g_VideoInfo.line_spacing; j ++) {
			k1 = j * aoslo_movie.width;
			k2 = (j+ofy) * g_VideoInfo.img_width;
			for (i = 0; i < aoslo_movie.width; i ++) {
			//	g_PatchParamsA.img_sliceR[k1+i] = (g_BlockID != 1)?((unsigned char *)(mem_i))[k2+i+ofx]:0; //zero top stripe
				g_PatchParamsA.img_sliceR[k1+i] = ((unsigned char *)(mem_i))[k2+i+ofx];
			}
		}
	}
}

void g_UpdateMotions(BOOL bDrawDewarp, int cx, int cy, int *MarkPatch, int *deltaX, int *deltaY) 
{
	int         i, j;
	int         hours;
	int         minutes;
	int         seconds;
	double      milliseconds;

	// when the last stripe is received, motions from all patches are reorganized
	// and stabilized image is drawn on GUI
	///////////////////
	// reorganize deltas
	////////////////////
	for (i = 0; i < g_BlockCounts; i ++) {
		deltaX[i] = -deltaX[i];
		deltaY[i] = -deltaY[i];
		//deltaX[i] = -cx;
		//deltaY[i] = -cy;
	}

	// update central patch if its motion is not reliable
	i = g_BlockCounts/2;
	if (abs(deltaX[i]+cx)>THRESHOLD_SX ||
		abs(deltaY[i]+cy)>THRESHOLD_SY ||
		MarkPatch[i] < 0) {
		deltaX[i] = -cx;
		deltaY[i] = -cy;
	}
	// update up half of patch motions
	for (j = i+1; j < g_BlockCounts; j ++) {
		if (abs(deltaX[j]-deltaX[j-1])>THRESHOLD_SX ||
			abs(deltaY[j]-deltaY[j-1])>THRESHOLD_SY ||
			MarkPatch[j] < 0) {
			deltaX[j] = deltaX[j-1];
			deltaY[j] = deltaY[j-1];
		}
	}
	// update low half of patch motions
	for (j = i-1; j >= 0; j --) {
		if (abs(deltaX[j]-deltaX[j+1])>THRESHOLD_SX ||
			abs(deltaY[j]-deltaY[j+1])>THRESHOLD_SY ||
			MarkPatch[j] < 0) {
			deltaX[j] = deltaX[j+1];
			deltaY[j] = deltaY[j+1];
		}
	}
	// shift deltax and deltay on elements since there is no data for
	// calculating motion of the first patches
	deltaX[g_BlockCounts]   = deltaX[g_BlockCounts-1];
	deltaY[g_BlockCounts]   = deltaY[g_BlockCounts-1];

	// copy motion data for stimulus delivery
	g_ICANDIParams.PATCH_CNT = g_BlockCounts+1;
	g_PatchParamsB.shift_xy[0] = (float)g_ICANDIParams.PATCH_CNT;
	for (j = 0; j < g_ICANDIParams.PATCH_CNT; j ++) {
		g_PatchParamsB.shift_xy[j+1] = (float)deltaX[j];
		g_PatchParamsB.shift_xy[j+g_ICANDIParams.PATCH_CNT+1] = (float)deltaY[j];
	}

	// Here we do dewarping 
	if (g_Interp1(g_PatchParamsB.shift_xy, (int)g_PatchParamsB.shift_xy[0], g_PatchParamsB.shift_xi, aoslo_movie.height, 0) == 0 &&
		g_Interp1(g_PatchParamsB.shift_xy, (int)g_PatchParamsB.shift_xy[0], g_PatchParamsB.shift_yi, aoslo_movie.height, (int)g_PatchParamsB.shift_xy[0]) == 0) 
	{
		// erase background
		ZeroMemory(aoslo_movie.video_out, aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy);
			
		// run dewarping
		if (g_Dewarp(aoslo_movie.video_in, aoslo_movie.video_out, g_PatchParamsB.shift_xi, g_PatchParamsB.shift_yi, g_frameIndex) == -1) {
//		if (g_Dewarp(aoslo_movie.video_ins, aoslo_movie.video_out, g_PatchParamsB.shift_xi, g_PatchParamsB.shift_yi, g_frameIndex) == -1) {
			aoslo_movie.no_dewarp_video = TRUE;
		} else {
			aoslo_movie.no_dewarp_video = FALSE;
		}

/*		int Xj, Yj, idx;
		for (j = 0; j < aoslo_movie.height; j ++) {
			for (i = 0; i < aoslo_movie.width; i ++) {
				Xj = -cx + dewarp_x + i;
				Yj = -cy + dewarp_x + j;
				idx = Yj * aoslo_movie.dewarp_sx + Xj;
				aoslo_movie.video_out[idx] = aoslo_movie.video_ins[j*aoslo_movie.width+i];
			}
		}*/
	}

	if (g_bFFTIsRunning == TRUE && g_bTimingTest) {

		g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
		fprintf(g_fpTimeLog, "f_cnt:%d, registration done,      %d:%d:%d:%5.3f\n", 
			g_frameIndex, hours, minutes, seconds, milliseconds);
/*
		for (j = 0; j < g_ICANDIParams.PATCH_CNT; j ++)
			fprintf(g_fpTimeLog, "(%d,%d) ", deltaX[j], deltaY[j]);
		fprintf(g_fpTimeLog, "\n");
		*/
	}
}


void g_SOFHandler()
{
	int         hours;
	int         minutes;
	int         seconds;
	double      milliseconds;
	int         i, j, k, seed, frameID, size, w_red, w_ir, w_gr, h_red, h_ir, h_gr, ind_red, ind_ir, ind_gr;
	BOOL		load; //01/11/2012

	aoslo_movie.nStimHeight = 0;
	g_bPreCritical  = FALSE;
	g_bCritical     = FALSE;
	g_bZeroGainFlag = FALSE;
	aoslo_movie.FlashOnFlag   = FALSE;
	aoslo_movie.WriteMarkFlag = FALSE;			
//	g_objVirtex5BMD.AppClearCriticalInfo(g_hDevVirtex5);
	
	if (g_bMatlab_Update == TRUE) {
		if (!g_bMatlabVideo) {
			if (g_bMatlab_Loop == TRUE || g_bMatlab_Trigger == TRUE) {//update according to sequence and stop at the end or update according to sequence continuously
				g_sMatlab_Update_Ind[0] = g_sMatlab_Update_Ind[1] = g_sMatlab_Update_Ind[2] = g_nCurFlashCount;
				aoslo_movie.bRedAomOn && (g_nRedPower_Matlab[g_nCurFlashCount] != g_nRedPower_Matlab[g_nCurFlashCount-1])?g_objVirtex5BMD.AppUpdate14BITsLaserRed(g_hDevVirtex5, g_usRed_LUT[g_nRedPower_Matlab[g_nCurFlashCount]]):0;
				aoslo_movie.bIRAomOn && (g_nIRPower_Matlab[g_nCurFlashCount] != g_nIRPower_Matlab[g_nCurFlashCount-1])?g_objVirtex5BMD.AppUpdateIRLaser(g_hDevVirtex5, g_usIR_LUT[g_nIRPower_Matlab[g_nCurFlashCount]]):0;
				aoslo_movie.bGrAomOn && (g_nGreenPower_Matlab[g_nCurFlashCount] != g_nGreenPower_Matlab[g_nCurFlashCount-1])?g_objVirtex5BMD.AppUpdate14BITsLaserGR(g_hDevVirtex5, g_usGreen_LUT[g_nGreenPower_Matlab[g_nCurFlashCount]]):0;
				g_StimulusPos0G.x = g_StimulusPos.x = g_nStimPosBak_Matlab.x + g_nLocX[0][g_nCurFlashCount];
				g_StimulusPos0G.y = g_StimulusPos.y = g_nStimPosBak_Matlab.y + g_nLocY[0][g_nCurFlashCount];			
/*				g_StimulusPos0G.x = g_StimulusPos.x = ((!aoslo_movie.fStabGainStim && g_bGain0Tracking)?
					g_nStimPosBak_Gain0Track.x:g_nStimPosBak_Matlab.x) + g_nLocX[0][g_nCurFlashCount];
				g_StimulusPos0G.y = g_StimulusPos.y = ((!aoslo_movie.fStabGainStim && g_bGain0Tracking)?
					g_nStimPosBak_Gain0Track.y:g_nStimPosBak_Matlab.y) + g_nLocY[0][g_nCurFlashCount];*/
				g_Channel1Shift.x = (abs(g_nLocX[1][g_nCurFlashCount])<=32)?g_nLocX[1][g_nCurFlashCount]:0;
				g_Channel1Shift.y = (abs(g_nLocY[1][g_nCurFlashCount])<=32)?g_nLocY[1][g_nCurFlashCount]:0;
				g_Channel2Shift.x = (abs(g_nLocX[2][g_nCurFlashCount])<=32)?g_nLocX[2][g_nCurFlashCount]:0;
				g_Channel2Shift.y = (abs(g_nLocY[2][g_nCurFlashCount])<=32)?g_nLocY[2][g_nCurFlashCount]:0;
				aoslo_movie.fStabGainStim = g_fGain_Matlab[g_nCurFlashCount];
				aoslo_movie.nRotateLocation = (int)g_ubAngle_Matlab[g_nCurFlashCount];
				aoslo_movie.stimulus_audio_flag = g_ubStimPresFlg_Matlab[g_nCurFlashCount];
				//if (aoslo_movie.stimulus_audio_flag && g_ubStimPresFlg_Matlab[g_nCurFlashCount]) {		// testing an idea from Pavan
				// 	SetEvent(g_EventStimPresentFlag);														// testing an idea from Pavan
				//}

				if (++g_nCurFlashCount >= g_nFlashCount) {
					g_nCurFlashCount = 0;
					(g_bMatlab_Trigger == TRUE && g_bMatlab_Loop != TRUE) ?g_bMatlab_Update = FALSE, g_bMatlab_Trigger = FALSE:0;
				}
			}
			else {//just update once with given index				
				g_bMatlab_Update = FALSE;
			}
			
			ind_ir = (g_nSequence_Matlab[0][g_sMatlab_Update_Ind[0]]>=0 && g_nSequence_Matlab[0][g_sMatlab_Update_Ind[0]]<aoslo_movie.stimuli_num)?g_nSequence_Matlab[0][g_sMatlab_Update_Ind[0]]:1;
			ind_red = (g_nSequence_Matlab[1][g_sMatlab_Update_Ind[1]]>=0 && g_nSequence_Matlab[1][g_sMatlab_Update_Ind[1]]<aoslo_movie.stimuli_num)?g_nSequence_Matlab[1][g_sMatlab_Update_Ind[1]]:0;
			ind_gr = (g_nSequence_Matlab[2][g_sMatlab_Update_Ind[2]]>=0 && g_nSequence_Matlab[2][g_sMatlab_Update_Ind[2]]<aoslo_movie.stimuli_num)?g_nSequence_Matlab[2][g_sMatlab_Update_Ind[2]]:0;

			if ((ind_ir==1 || !ind_ir) && ((ind_red==1 || !ind_red) && (ind_gr==1 || !ind_gr))) {//just turn off the stimulus				
				w_ir = h_ir = w_red = h_red = w_gr = h_gr = 16;
				!g_ubStimPresFlg_Matlab[g_nCurFlashCount-1]?aoslo_movie.stimulus_audio_flag = FALSE:0;
				aoslo_movie.WriteMarkFlag = FALSE;
				g_bStimulusOn = FALSE;
			}
			else {
				if (ind_ir > 1) {
					w_ir = aoslo_movie.stimuli_sx[ind_ir];
					h_ir = aoslo_movie.stimuli_sy[ind_ir];
				}
				if (ind_red > 1) {
					w_red = aoslo_movie.stimuli_sx[ind_red];
					h_red = aoslo_movie.stimuli_sy[ind_red];
				}
				if (ind_gr > 1) {
					w_gr = aoslo_movie.stimuli_sx[ind_gr];
					h_gr = aoslo_movie.stimuli_sy[ind_gr];
				}

				if (!ind_ir || ind_ir == 1) { //choose the large w and h from red and green stimuli
					if (ind_red > 1 && ind_gr > 1) {
						w_ir = (w_red>=w_gr? w_red:w_gr);
						h_ir = (h_red>=h_gr? h_red:h_gr);
					}
					else if (ind_red <= 1 && ind_gr > 1) {
						w_ir = w_gr;
						h_ir = h_gr;
					}
					else {
						w_ir = w_red;
						h_ir = h_red;
					}
				}
				if (!ind_red || ind_red == 1) { //choose the large w and h from ir and green stimuli
					if (ind_ir > 1 && ind_gr > 1) {
						w_red = (w_ir>=w_gr? w_ir:w_gr);
						h_red = (h_ir>=h_gr? h_ir:h_gr);
					}
					else if (ind_ir <= 1 && ind_gr > 1) {
						w_red = w_gr;
						h_red = h_gr;
					}
					else {
						w_red = w_ir;
						h_red = h_ir;
					}
				}
				if (!ind_gr || ind_gr == 1) { //choose the large w and h from ir and red stimuli
					if (ind_ir > 1 && ind_red > 1) {
						w_gr = (w_ir>=w_red? w_ir:w_red);
						h_gr = (h_ir>=h_red? h_ir:h_red);
					}
					else if (ind_ir <= 1 && ind_red > 1) {
						w_gr = w_red;
						h_gr = h_red;
					}
					else {
						w_gr = w_ir;
						h_gr = h_ir;
					}
				}

				aoslo_movie.FlashOnFlag   = TRUE;
				aoslo_movie.WriteMarkFlag = TRUE;	
				g_bStimulusOn = TRUE;
			}

			// update ir stimulus pattern
			memcpy(aoslo_movie.stim_ir_buffer, aoslo_movie.stimuli_buf[ind_ir], w_ir*h_ir*sizeof(unsigned short));
			aoslo_movie.stim_ir_nx = w_ir;
			aoslo_movie.stim_ir_ny = h_ir;

			// update red stimulus pattern
			memcpy(aoslo_movie.stim_rd_buffer, aoslo_movie.stimuli_buf[ind_red], w_red*h_red*sizeof(unsigned short));
			aoslo_movie.stim_rd_nx = w_red;
			aoslo_movie.stim_rd_ny = h_red;

			// update green stimulus pattern
			memcpy(aoslo_movie.stim_gr_buffer, aoslo_movie.stimuli_buf[ind_gr], w_gr*h_gr*sizeof(unsigned short));
			aoslo_movie.stim_gr_nx = w_gr;
			aoslo_movie.stim_gr_ny = h_gr;				

			SetEvent(g_EventLoadStim);			
		}
		else if (g_bMatlabVideo) { //play the stimulus video loaded from matlab g_nFlashCount g_nCurFlashCount aoslo_movie.nStimFrameIdx >= aoslo_movie.nStimFrameNum
			if (g_nFlashCount != -1 && ++g_nCurFlashCount == 1) { //12/15/2011
				aoslo_movie.nStimFrameIdx = -1;
			}
			//01/11/2012
			aoslo_movie.nStimFrameIdx ++;
			load = TRUE;
			if (aoslo_movie.nStimFrameIdx >= aoslo_movie.nStimFrameNum && g_bMatlab_Update == TRUE) { //12/12/2011
				if (g_bMatlab_Loop && (g_nFlashCount == -1 || g_nCurFlashCount < g_nFlashCount)) {//infinite loop
					aoslo_movie.nStimFrameIdx = 0;
				}
				else {
					aoslo_movie.nStimFrameIdx = -1;
					load = FALSE;
					g_bMatlab_Update = FALSE;
					//	g_bStimulusOn = FALSE;	
					memcpy(aoslo_movie.stim_ir_buffer, aoslo_movie.stimuli_buf[1], STIMULUS_SIZE_X*STIMULUS_SIZE_Y*sizeof(unsigned short));
					aoslo_movie.stim_ir_nx = STIMULUS_SIZE_X;
					aoslo_movie.stim_ir_ny = STIMULUS_SIZE_Y;

					// update red stimulus pattern
					memcpy(aoslo_movie.stim_rd_buffer, aoslo_movie.stimuli_buf[0], STIMULUS_SIZE_X*STIMULUS_SIZE_Y*sizeof(unsigned short));
					aoslo_movie.stim_rd_nx = STIMULUS_SIZE_X;
					aoslo_movie.stim_rd_ny = STIMULUS_SIZE_Y;

					// update green stimulus pattern
					memcpy(aoslo_movie.stim_gr_buffer, aoslo_movie.stimuli_buf[0], STIMULUS_SIZE_X*STIMULUS_SIZE_Y*sizeof(unsigned short));
					aoslo_movie.stim_gr_nx = STIMULUS_SIZE_X;
					aoslo_movie.stim_gr_ny = STIMULUS_SIZE_Y;

					SetEvent(g_EventLoadStim);
				}
			}
			if (load == TRUE) {	//12/15/2011		
				memcpy(aoslo_movie.stim_rd_buffer, aoslo_movie.stim_video[aoslo_movie.nStimVideoIdx]
				+(aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*(aoslo_movie.nStimFrameIdx*aoslo_movie.nStimVideoPlanes[aoslo_movie.nStimVideoIdx])) 
					, aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*sizeof(unsigned short));
				memcpy(aoslo_movie.stim_gr_buffer, aoslo_movie.stim_video[aoslo_movie.nStimVideoIdx]
				+(aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*(aoslo_movie.nStimFrameIdx*aoslo_movie.nStimVideoPlanes[aoslo_movie.nStimVideoIdx]+1)) 
					, aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*sizeof(unsigned short));
				memcpy(aoslo_movie.stim_ir_buffer, aoslo_movie.stim_video[aoslo_movie.nStimVideoIdx]
				+(aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*(aoslo_movie.nStimFrameIdx*aoslo_movie.nStimVideoPlanes[aoslo_movie.nStimVideoIdx]+2)) 
					, aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*sizeof(unsigned short));
				aoslo_movie.stim_rd_nx = aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx];
				aoslo_movie.stim_rd_ny = aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx];
				aoslo_movie.stim_gr_nx = aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx];
				aoslo_movie.stim_gr_ny = aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx];
				aoslo_movie.stim_ir_nx = aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx];
				aoslo_movie.stim_ir_ny = aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx];	
				SetEvent(g_EventLoadStim);	
			//	fprintf(g_fp, "SOF, fraID:%d, %d, %d\n", g_frameIndex, aoslo_movie.nStimVideoIdx, aoslo_movie.nStimFrameIdx);
				aoslo_movie.FlashOnFlag   = TRUE;
				aoslo_movie.WriteMarkFlag = TRUE;
				g_bStimulusOn = TRUE;
			}
		}
	}
	else
		// turn on/off stimulus based on flash frequency and flash duty cycle as we do normally
		for (i = 0; i <= aoslo_movie.FlashDutyCycle; i ++) {
			if (g_sampling_counter%aoslo_movie.FlashDist == i) {
				aoslo_movie.FlashOnFlag   = TRUE;
				aoslo_movie.WriteMarkFlag = TRUE;			
			}
		}	

	// deliver stimulus to multiple cones 
	if (aoslo_movie.DeliveryMode > 0 && aoslo_movie.StimulusNum >= 1 && aoslo_movie.RandDelivering) {
		// generage a random path
		if (g_sampling_counter%(aoslo_movie.StimulusNum*aoslo_movie.FlashDist) == 0) {
			seed = g_sampling_counter/(aoslo_movie.StimulusNum*aoslo_movie.FlashDist) - aoslo_movie.FrameCountOffset;
			if (aoslo_movie.RandPathIndice != NULL) {
				for (i = 0; i < aoslo_movie.StimulusNum; i ++) {
					aoslo_movie.RandPathIndex[i] = aoslo_movie.RandPathIndice[seed*aoslo_movie.StimulusNum+i];
				}
			}
		}

		// random path has been updated
		frameID = g_sampling_counter % (aoslo_movie.StimulusNum*aoslo_movie.FlashDist); // remaining
		for (i = 0; i < aoslo_movie.StimulusNum; i ++) {
			if (frameID%(aoslo_movie.FlashDist*aoslo_movie.StimulusNum) == i*aoslo_movie.FlashDist) {
				j = aoslo_movie.RandPathIndex[i];
				g_StimulusPos.x = aoslo_movie.StimulusPos[j].x;
				g_StimulusPos.y = aoslo_movie.StimulusPos[j].y;
				break;
			}
		}
	}
	//
	// added on 6-15-2009
	//
	// testing multiple stimuli delivery. Additional codes are needed for specified features
	//
	if (g_bMultiStimuli == TRUE) {
		k = g_sampling_counter / GRABBER_FRAME_RATE;//30;
		for (i = 0; i < aoslo_movie.stimuli_num; i ++) {
			// load static images to FPGA as stimulus
			if (k%aoslo_movie.stimuli_num == i) {

				// update red stimulus pattern
				j = (k+1)%aoslo_movie.stimuli_num;
				size = aoslo_movie.stimuli_sx[j] * aoslo_movie.stimuli_sy[j] * sizeof(unsigned short);
				memcpy(aoslo_movie.stim_rd_buffer, aoslo_movie.stimuli_buf[j], size);
				aoslo_movie.stim_rd_nx = aoslo_movie.stimuli_sx[j];
				aoslo_movie.stim_rd_ny = aoslo_movie.stimuli_sy[j];

				// update green stimulus pattern
				size = aoslo_movie.stimuli_sx[i] * aoslo_movie.stimuli_sy[i] * sizeof(unsigned short);
				memcpy(aoslo_movie.stim_gr_buffer, aoslo_movie.stimuli_buf[i], size);	// this could be a different pattern
				aoslo_movie.stim_gr_nx = aoslo_movie.stimuli_sx[i];
				aoslo_movie.stim_gr_ny = aoslo_movie.stimuli_sy[i];

				// update IR stimulus pattern
				j = (k+2)%aoslo_movie.stimuli_num;
				size = aoslo_movie.stimuli_sx[j] * aoslo_movie.stimuli_sy[j] * sizeof(unsigned short);
				memcpy(aoslo_movie.stim_ir_buffer, aoslo_movie.stimuli_buf[j], size);
				aoslo_movie.stim_ir_nx = aoslo_movie.stimuli_sx[j];
				aoslo_movie.stim_ir_ny = aoslo_movie.stimuli_sy[j];

				SetEvent(g_EventLoadStim);
/*				// load IR stimulus pattern to FPGA
				g_objVirtex5BMD.AppLoadStimulus(g_hDevVirtex5, aoslo_movie.stim_ir_buffer, aoslo_movie.stim_ir_nx, aoslo_movie.stim_ir_ny, 3);
				// load green stimulus pattern to FPGA
				g_objVirtex5BMD.AppLoadStimulus(g_hDevVirtex5, aoslo_movie.stim_gr_buffer, aoslo_movie.stim_gr_nx, aoslo_movie.stim_gr_ny, 2);
				// load Red stimulus pattern to FPGA
				g_objVirtex5BMD.AppLoadStimulus(g_hDevVirtex5, aoslo_movie.stim_rd_buffer, aoslo_movie.stim_rd_nx, aoslo_movie.stim_rd_ny, 1);
*/
				break;
			}
		}
	} else {
		// load real-time video to FPGA as stimulus
		if (aoslo_movie.bWithStimVideo == TRUE && g_bFFTIsRunning == TRUE && g_frameIndex > 0) {
		// 01/11/2012
			aoslo_movie.nStimFrameIdx ++;
			load = FALSE;	

			//fprintf(g_fp, "%d, %d\n", aoslo_movie.nStimVideoIdx, aoslo_movie.nStimFrameIdx);

			if (aoslo_movie.bStimRewind == TRUE) {
				if (aoslo_movie.nStimFrameIdx >= aoslo_movie.nStimFrameNum) 
					aoslo_movie.nStimFrameIdx = 0;
			//	SetEvent(g_EventLoadStim);
				load = TRUE;
			} else {
				if (aoslo_movie.nStimFrameIdx >= aoslo_movie.nStimFrameNum) {
				//	aoslo_movie.nStimFrameIdx = aoslo_movie.nStimFrameNum - 1;
					aoslo_movie.nStimFrameIdx = -1;
				} else {
				//	SetEvent(g_EventLoadStim);
					load = TRUE;
				}
			}
			if (load == TRUE) {
				memcpy(aoslo_movie.stim_rd_buffer, aoslo_movie.stim_video[aoslo_movie.nStimVideoIdx]
				+(aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*(aoslo_movie.nStimFrameIdx*aoslo_movie.nStimVideoPlanes[aoslo_movie.nStimVideoIdx])) 
					, aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*sizeof(unsigned short));
				memcpy(aoslo_movie.stim_gr_buffer, aoslo_movie.stim_video[aoslo_movie.nStimVideoIdx]
				+(aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*(aoslo_movie.nStimFrameIdx*aoslo_movie.nStimVideoPlanes[aoslo_movie.nStimVideoIdx]+1)) 
					, aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*sizeof(unsigned short));
				memcpy(aoslo_movie.stim_ir_buffer, aoslo_movie.stim_video[aoslo_movie.nStimVideoIdx]
				+(aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*(aoslo_movie.nStimFrameIdx*aoslo_movie.nStimVideoPlanes[aoslo_movie.nStimVideoIdx]+2)) 
					, aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx]*aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx]*sizeof(unsigned short));
				aoslo_movie.stim_rd_nx = aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx];
				aoslo_movie.stim_rd_ny = aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx];
				aoslo_movie.stim_gr_nx = aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx];
				aoslo_movie.stim_gr_ny = aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx];
				aoslo_movie.stim_ir_nx = aoslo_movie.nStimVideoNX[aoslo_movie.nStimVideoIdx];
				aoslo_movie.stim_ir_ny = aoslo_movie.nStimVideoNY[aoslo_movie.nStimVideoIdx];
				SetEvent(g_EventLoadStim);
			}
		}
	}

	if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
		g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
		fprintf(g_fpTimeLog, "\nf_cnt:%d, SOF,      %d:%d:%d:%5.3f\n", 
			g_frameIndex, hours, minutes, seconds, milliseconds);
	}
	
	if ((aoslo_movie.stim_rd_ny >= 64 || aoslo_movie.stim_ir_ny >= 64 || aoslo_movie.stim_gr_ny >= 64) && g_bStimProjSel == TRUE) {
		g_bStimProjection = TRUE;
	} else {
		g_bStimProjection = FALSE;
	}

	// This three parameters are for GPU-based stimulus delivery
	if (aoslo_movie.stim_ir_ny > 64) {
		aoslo_movie.stim_num_yir = aoslo_movie.stim_ir_ny/LINE_INTERVAL+1;
		for (i = 0; i < aoslo_movie.stim_num_yir; i ++) 
			aoslo_movie.stim_loc_yir[aoslo_movie.stim_num_yir-1-i] = (g_StimulusPos.y-dewarp_y-aoslo_movie.stim_ir_ny/2)+ i*LINE_INTERVAL;
		g_bStimProjectionIR = TRUE;
	} else {
		g_bStimProjectionIR = FALSE;
	}
	if (aoslo_movie.stim_rd_ny > 64) {
		g_bStimProjectionRD = TRUE;
		aoslo_movie.stim_num_yrd = aoslo_movie.stim_rd_ny/LINE_INTERVAL+1;
		for (i = 0; i < aoslo_movie.stim_num_yrd; i ++) 
			aoslo_movie.stim_loc_yrd[aoslo_movie.stim_num_yrd-1-i] = (g_StimulusPos.y-dewarp_y-aoslo_movie.stim_rd_ny/2)+ i*LINE_INTERVAL;
	} else {
		g_bStimProjectionRD = FALSE;
	}
	if (aoslo_movie.stim_gr_ny > 64) {
		g_bStimProjectionGR = TRUE;
		aoslo_movie.stim_num_ygr = aoslo_movie.stim_gr_ny/LINE_INTERVAL+1;
		for (i = 0; i < aoslo_movie.stim_num_ygr; i ++) 
			aoslo_movie.stim_loc_ygr[aoslo_movie.stim_num_ygr-1-i] = (g_StimulusPos.y-dewarp_y-aoslo_movie.stim_gr_ny/2)+ i*LINE_INTERVAL;
	} else {
		g_bStimProjectionGR = FALSE;
	}

	//
	// added on 7-8-2009
	//
	// run multiple stimuli on the same frame
	//
	if (g_bStimProjection == TRUE && g_StimulusPos.y>dewarp_y) {
		if (!g_bCalcStimPos) {
			aoslo_movie.stim_num_y = aoslo_movie.stim_rd_ny/LINES_PER_STIMULUS+1;
			for (i = 0; i < aoslo_movie.stim_num_y; i ++) 
				aoslo_movie.stim_loc_y[aoslo_movie.stim_num_y-1-i] = (g_StimulusPos.y-aoslo_movie.stim_rd_ny/2)+ i*LINES_PER_STIMULUS;
		}
	} else {
		aoslo_movie.stim_num_y = 0;
	}
	aoslo_movie.stim_num_y = 0;

	//
	// added on 10-15-2009
	//
	// apply random mask on the imaging pupil
	//
	// attention please: this is for testing purpose only
	if (aoslo_movie.bPupilMask == TRUE) {
		int sx1, nx1;
		
		int  sx, sy;
		int  nx, ny;
		double      xprime, yprime;
		
		xprime = aoslo_movie.StimulusDeltaX*cos(aoslo_movie.nRotateLocation*3.1415926/180.0) + 
			aoslo_movie.StimulusDeltaY*sin(aoslo_movie.nRotateLocation*3.1415926/180.0);
		yprime =-aoslo_movie.StimulusDeltaX*sin(aoslo_movie.nRotateLocation*3.1415926/180.0) + 
			aoslo_movie.StimulusDeltaY*cos(aoslo_movie.nRotateLocation*3.1415926/180.0);
		sx = (int)(xprime*aoslo_movie.fStabGainMask) + 70;
		sy = (int)(yprime*aoslo_movie.fStabGainMask) + 86;
		
		if (sx <= 0 || sy < 0  || sx >= 165 || sy >= 172) {
			sx = sy = 0;
			nx = 511;
			ny = 511;
		} else {
			nx = 340;
			ny = 340;
		}
		if (g_ICANDIParams.Desinusoid == 1) {
			sx1 = sx;
			for (i = 0; i < aoslo_movie.width; i ++) {
				if (sx1 < g_PatchParamsA.StretchFactor[i]) break;
			}
			sx1 = i;
			
			nx1 = sx+nx;
			for (i = 0; i < aoslo_movie.width; i ++) {
				if (nx1 < g_PatchParamsA.StretchFactor[i]) break;
			}
			nx1 = i - sx1;
			
			sx = sx1;
			nx = nx1;
		}
		
		g_objVirtex5BMD.WritePupilMask(g_hDevVirtex5, sx, sy, nx, ny);
	}
}

/* -----------------------------------------------
    EOB handler
   ----------------------------------------------- */
void DLLCALLCONV VIRTEX5_IntHandler(PVOID pData)
{
	UINT32  intID, lineID, interrupt_flag;
	static UINT32  lineIDold;
    long    y_offset;
	int     BlockID;
	int     i, j, m, p, idx1, idx2;
	float   val1, val2, val;

	int     hours;
	int     minutes;
	int     seconds;
	double  milliseconds;

	// return immediately if users click "disconnect camera"
	if (((CICANDIApp*)AfxGetApp())->m_isGrabStarted == FALSE) return;
	// return immediately if the first v-sync has not come
	if (g_bFirstVsync == FALSE) return;

    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PVIRTEX5_DEV_CTX pDevCtx = (PVIRTEX5_DEV_CTX)WDC_GetDevContext(pDev);

	// get interrupt line index
	WDC_ReadAddr32((WDC_DEVICE_HANDLE)pDev, VIRTEX5_SPACE, VIRTEX5_LINECTRL_OFFSET, &intID);
	interrupt_flag = intID >> 28;

	lineID = intID << 4;
	lineID = lineID >> 20;

	// when line index is greater than image height, wait for the next interrupt
	if (lineID > g_VideoInfo.img_height) return;

	// calculate block index
	BlockID = lineID/LINE_INTERVAL;
	if (g_bEOF == TRUE) {
		if (lineID > LINE_INTERVAL) {
			BlockID = 1;
		//	return;
		}
		g_bEOF = FALSE;
	}
	g_BlockID = BlockID;

	// skip repeated interrupt
	if (interrupt_flag == 0 || interrupt_flag == 2) {
		if (lineID == lineIDold) return;
		//if (lineIDold != lineID-16 && lineID != 16) fprintf(g_fp, "%d  !!!  Line: %d lost\n", g_sampling_counter, lineID-16);
		lineIDold = lineID;
	} else {
		return;
	}

	// if there is no Kernel Plugin, high-level calling shall be applied
	if (g_bKernelPlugin == FALSE) 
		g_objVirtex5BMD.AppSamplePatch(g_hDevVirtex5, g_VideoInfo, lineID);


	if (g_bStimProjection == TRUE && g_StimulusPos.y>dewarp_y) {
		if (!g_bCalcStimPos) {
			aoslo_movie.stim_num_y = aoslo_movie.stim_rd_ny/LINES_PER_STIMULUS+1;
			for (i = 0; i < aoslo_movie.stim_num_y; i ++) 
				aoslo_movie.stim_loc_y[aoslo_movie.stim_num_y-1-i] = (g_StimulusPos.y-aoslo_movie.stim_rd_ny/2)+ i*LINES_PER_STIMULUS;
		}
	} else {
		aoslo_movie.stim_num_y = 0;
	}
	
	if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
		g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
		fprintf(g_fpTimeLog, "f_cnt:%d, BID = %d,      %d:%d:%d:%5.3f\n", 
			g_frameIndex, BlockID, hours, minutes, seconds, milliseconds);
	}

/*
	int rem = g_sampling_counter%GRABBER_FRAME_RATE;//30;
	if (rem==21||rem==22||rem==23||rem==24||rem==25) {
		ZeroMemory(g_PatchParamsA.img_sliceR, aoslo_movie.width*aoslo_movie.height);
	} else {*/
		// Copy data from device buffer to PC buffer
//	if (g_BlockID > 1)
		g_MemCpyDev2Host(pDevCtx->pBuf, g_VideoInfo.video_in, g_dwTotalCount*sizeof(UINT32), lineID-g_VideoInfo.line_spacing);
//	else
//		ZeroMemory(aoslo_movie.video_out, aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy);
//	}


	y_offset = lineID - LINE_INTERVAL;
	if (BlockID == 1) aoslo_movie.stimulus_flag = FALSE;

	// do desinusoiding when it is necessary
	if (g_ICANDIParams.Desinusoid == 1) {

		if (g_PatchParamsA.UnMatrixIdx == NULL && g_PatchParamsA.UnMatrixVal == NULL && g_PatchParamsA.StretchFactor == NULL) { 
			g_PatchParamsA.UnMatrixIdx = new int   [(aoslo_movie.width+aoslo_movie.height)*2];
			g_PatchParamsA.UnMatrixVal = new float [(aoslo_movie.width+aoslo_movie.height)*2];
			g_PatchParamsA.StretchFactor = new float [aoslo_movie.width];
			if (g_ReadLUT(aoslo_movie.width, aoslo_movie.height, g_PatchParamsA.UnMatrixIdx, g_PatchParamsA.UnMatrixVal, g_PatchParamsA.StretchFactor) == FALSE) {
				delete [] g_PatchParamsA.UnMatrixIdx;
				delete [] g_PatchParamsA.UnMatrixVal;
				delete [] g_PatchParamsA.StretchFactor;
				g_PatchParamsA.UnMatrixIdx = NULL;
				g_PatchParamsA.UnMatrixVal = NULL;
				g_PatchParamsA.StretchFactor = NULL;
				g_ICANDIParams.Desinusoid = 0;
			}
		}

		// we need to do desinusoiding here
		for (j = 0; j < LINE_INTERVAL; j ++) {
			m = (j+y_offset) * aoslo_movie.width;
			p = j * aoslo_movie.width;
			for (i = 0; i < aoslo_movie.width; i ++) {
				idx1 = g_PatchParamsA.UnMatrixIdx[aoslo_movie.width-1-i];
				idx2 = g_PatchParamsA.UnMatrixIdx[2*aoslo_movie.width-1-i];
				val1 = g_PatchParamsA.UnMatrixVal[aoslo_movie.width-1-i];
				val2 = g_PatchParamsA.UnMatrixVal[2*aoslo_movie.width-1-i]; 
				val = (*(g_PatchParamsA.img_sliceR+p+idx1))*val1 + (*(g_PatchParamsA.img_sliceR+p+idx2))*val2;
				*(aoslo_movie.video_in+m+aoslo_movie.width-1-i) = (BYTE)val;

				// this image slice will be sent to MSC server
				*(g_PatchParamsA.img_slice+p+aoslo_movie.width-1-i) = (BYTE)val;
			}
		}
		
	} else {
		for (j = 0; j < LINE_INTERVAL; j ++) {
			m = (j+y_offset) * aoslo_movie.width;
			p = j * aoslo_movie.width;
			for (i = 0; i < aoslo_movie.width; i ++) {		
				*(aoslo_movie.video_in+m+i) = *(g_PatchParamsA.img_sliceR+p+i);

				// this image slice will be sent to MSC server
				*(g_PatchParamsA.img_slice+p+i) = *(g_PatchParamsA.img_sliceR+p+i);
			}
		}
	}

	SetEvent(g_EventEOB);

    // Execute the diagnostics application's interrupt handler routine 
	if (lineID == g_VideoInfo.img_height) {
		g_bEOF = TRUE;
//		fprintf(g_fp, "    A: %d\n", g_sampling_counter+1);

		if (g_bStimulusOn) {
			if (g_bFFTIsRunning == TRUE && fabs(aoslo_movie.fStabGainStim) > 0.001) {
				if (g_bStimProjection) {
					// clear stimulus location
					//g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, 0, aoslo_movie.width, 0, 0, 0, -1, -100);
					//g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, false, 0, 0, 0, 0, 0, aoslo_movie.width, 0, 0, 0, 0, 0, 0, 0);
				}				
				// clear stimulus location
				g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, 0, aoslo_movie.width, 0, 0, 0, -1, -100);
				g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, false, 0, 0, 0, 0, 0, 0, 0, aoslo_movie.width, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
			} else {
				// clear stimulus location
				g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, 0, aoslo_movie.width, 0, 0, 0, -1, -100);
				g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, false, 0, 0, 0, 0, 0, 0, 0, aoslo_movie.width, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
			}
		}

		memcpy(aoslo_movie.video_ins, aoslo_movie.video_in, aoslo_movie.width*aoslo_movie.height);
//		ZeroMemory(aoslo_movie.video_in, aoslo_movie.width*aoslo_movie.height);

		if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
			g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
			fprintf(g_fpTimeLog, "f_cnt:%d, EOF,           %d:%d:%d:%5.3f\n", g_frameIndex, hours, minutes, seconds, milliseconds);
		}
		if (g_bFFTIsRunning == TRUE) {
			if (g_frameIndex == 1) {
				switch (g_ICANDIParams.FRAME_ROTATION)	{
					case 0: //no rotation or flip
						memcpy(aoslo_movie.video_ref_bk, aoslo_movie.video_ins, aoslo_movie.height*aoslo_movie.width);
						break;
					case 1: //rotate 90 deg
						MUB_rotate90(&aoslo_movie.video_ref_bk, &aoslo_movie.video_ins, aoslo_movie.height, aoslo_movie.width);
						break;
					case 2: //flip along Y axis
						memcpy(aoslo_movie.video_ref_bk, aoslo_movie.video_ins, aoslo_movie.height*aoslo_movie.width);
						MUB_Rows_rev(&aoslo_movie.video_ref_bk, aoslo_movie.height, aoslo_movie.width);
						break;

						//  ***************************************   WE WERE HERE   *************************************
					case 3: //rotate 270 and flip along Y axis        
						memcpy(aoslo_movie.video_ref_bk, aoslo_movie.video_ins, aoslo_movie.height*aoslo_movie.width);
						MUB_rotate270(&aoslo_movie.video_ref_bk, &aoslo_movie.video_ins, aoslo_movie.height, aoslo_movie.width);
						MUB_Rows_rev(&aoslo_movie.video_ref_bk, aoslo_movie.height, aoslo_movie.width);
						break;


					default:
						;
				}
				g_qxCenter = 0.0f;
				g_qyCenter = 0.0f;
			}
		}

		g_sampling_counter ++;
		g_bEOFHandler = TRUE;

		// update raw view and message view
		g_nViewID = 1;
		SetEvent(g_EventSendViewMsg);
//		fprintf(g_fp, "    B: %d\n", g_sampling_counter);
		
		// tried to prevent location marking in fail-trials with additional "&& g_bDrawCrossMarker == TRUE"
		if (((g_StimulusPos0G.x > 0 && g_StimulusPos0G.y > 0) || (g_StimulusPos.x > 0 && g_StimulusPos.y > 0)) 
			&& aoslo_movie.stimulus_flag == TRUE && g_bStimulusOn) {// && g_bDrawCrossMarker == TRUE) {
				aoslo_movie.WriteMarkFlag?g_MarkStimulusPos():0;
		}
		if (g_bWritePsyStimulus == TRUE)
		{
			g_WritePsyStimulus();
			g_bWritePsyStimulus = FALSE;
		}
//		aoslo_movie.FlashOnFlag = FALSE;

		// at the end of each frame, and MSC running flag is turned on
		// we need to do dewarping, and display stabilized video, draw 
		// stimulus point on raw video
		if (g_bFFTIsRunning == TRUE && BlockID == g_BlockCounts) {
			g_frameIndex ++;
		}

		g_SOFHandler();
	}
}


PAVISTREAM g_GetAviStream(CString aviFileName, int *Frames, int *Width, int *Height, int *FirstFrame, int *Planes, long *BufSize)
{
    int      index=0;
	CString  msg;

	// now we initialize the .avi file and read it into the buffer
    AVIFileInit();

    PAVIFILE avi;
    int res = AVIFileOpen(&avi, aviFileName, OF_READ, NULL);

    if (res != AVIERR_OK) {
        //an error occures
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		msg = "Error: can't read file <" + aviFileName;
		AfxMessageBox(msg, MB_ICONWARNING);
        if (avi != NULL) AVIFileRelease(avi);
        
        return NULL;
    }

    AVIFILEINFO avi_info;
    AVIFileInfo(avi, &avi_info, sizeof(AVIFILEINFO));

    PAVISTREAM  pStream;
    res = AVIFileGetStream(avi, &pStream, streamtypeVIDEO /*video stream*/, 
                                               0 /*first stream*/);

	*Frames = avi_info.dwLength;
	if (avi_info.dwLength == -1)
	{
		cv::VideoCapture* capture = new cv::VideoCapture(cv::String(aviFileName), 0);
		if (capture->isOpened())
		{
			*Frames = (int)capture->get(cv::CAP_PROP_FRAME_COUNT);
			capture->release();
			capture = NULL;
		}
		//CvCapture* capture = cvCreateFileCapture(aviFileName);
		//cvQueryFrame( capture );
		//*Frames = (int) cvGetCaptureProperty( capture , CV_CAP_PROP_FRAME_COUNT );
		//cvReleaseCapture( &capture );
	}
	*Width  = avi_info.dwWidth;
	//*Width  = (avi_info.dwWidth % 4 == 0) ? avi_info.dwWidth : (avi_info.dwWidth/4)*4+4;
	//*Width  = (avi_info.dwWidth % 4 == 0) ? avi_info.dwWidth : avi_info.dwWidth+(avi_info.dwWidth%4);
	*Height = avi_info.dwHeight;
	if (Planes!=NULL) 
		*Planes = (avi_info.dwSuggestedBufferSize >= avi_info.dwWidth*avi_info.dwHeight*3)?3:1;
	if (BufSize!=NULL) 
		*BufSize = avi_info.dwSuggestedBufferSize;

    if (res != AVIERR_OK) {
		SetCursor(LoadCursor(NULL, IDC_ARROW));
        if (pStream != NULL) AVIStreamRelease(pStream);
		AfxMessageBox("Error: can't get avi file stream.", MB_ICONWARNING);

        AVIFileExit();
		return NULL;
	}

	//do some task with the stream
	if(FirstFrame!=NULL) 
	{
		*FirstFrame = AVIStreamStart(pStream);
		if (*FirstFrame == -1) {
			//Error getteing the frame inside the stream
			if (pStream != NULL) AVIStreamRelease(pStream);
			AfxMessageBox("Error: can't get frame index from the avi stream.", MB_ICONWARNING);

			AVIFileExit();
			return NULL;
		}
	}

	return pStream;
}

#if defined(_WIN64)
void CALLBACK g_TimeoutTimerFunc(UINT wTimerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR pParam, DWORD_PTR dw2)
#else
void CALLBACK g_TimeoutTimerFunc(UINT wTimerID, UINT msg, DWORD dwUser, DWORD pParam, DWORD dw2)
#endif // 
//void CALLBACK g_TimeoutTimerFunc(UINT wTimerID, UINT msg, DWORD dwUser, DWORD pParam, DWORD dw2)
{
	static  UINT32 timeout_cnt;

	if (((CICANDIApp*)AfxGetApp())->m_isGrabStarted) {
		if (g_objVirtex5BMD.DetectSyncSignals(g_hDevVirtex5) == TRUE) 
			timeout_cnt = 0;
		else 
			timeout_cnt ++;

		if (timeout_cnt >= 3) {
			g_objVirtex5BMD.AppStopADC(g_hDevVirtex5);
			//g_objVirtex5BMD.DIAG_DMAClose(g_hDevVirtex5, &g_dma);

			((CICANDIApp*)AfxGetApp())->m_isGrabStarted = FALSE;
			g_bFFTIsRunning = FALSE;		// FFT is turned off
			g_sampling_counter = 0;

			g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, ID_GRAB_STOP);
			g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, ID_GRAB_STOP);

			AfxMessageBox("Sync signals are lost. Video sampling is stopped.");
		}
	}
} 


// xc        : stimulus center location
// xshift    : additional stimulus shift in x direction
// stimWidth : stimulus width
// y1        : stimulus start location in y direction
// y2        : stimulus end location in y direction
// y3        : stimulus extended location in y direction (beyond y2)
// channelID : 1-IR, 2-Green, 3-Red
void g_UpdateStimOnFPGA(int xc0, int xch, int stimWidth, int iy1, int iy2, int iy3, int channelID)
{
	int hours, minutes, seconds;
	double milliseconds;

	int i, x0, xh, laser_power, nw1, nw2;
	int x0p, xhp, xLp, xRp, deltaxp, x1, x2;
	unsigned short *lut_loc_buf1, *lut_loc_buf2;
	float cent, w1, w2;
	int ny1, ny2, ny3;
/*
	if (g_ICANDIParams.FRAME_ROTATION) {
		rdShiftY = -g_ICANDIParams.RGBClkShifts[0].x;
		grShiftY = -g_ICANDIParams.RGBClkShifts[1].x;
	} else {
		rdShiftY = -g_ICANDIParams.RGBClkShifts[0].y;
		grShiftY = -g_ICANDIParams.RGBClkShifts[1].y;
	}
*/
	if (channelID == STIM_CHANNEL_GR) {
		ny1 = iy1;// + g_Channel2Shift.y + grShiftY;
		ny2 = iy2;// + g_Channel2Shift.y + grShiftY;
		//ny3 = (iy3 == 0) ? iy3 : iy3 + g_Channel2Shift.y;
	} else if (channelID == STIM_CHANNEL_RD) {
		ny1 = iy1;// + g_Channel1Shift.y + rdShiftY;
		ny2 = iy2;// + g_Channel1Shift.y + rdShiftY;
		//ny3 = (iy3 == 0) ? iy3 : iy3 + g_Channel1Shift.y;
	} else {
		ny1 = iy1;
		ny2 = iy2;
		ny3 = iy3;
	}
	ny3 = iy3;
	

	x0 = xc0;
	xh = xch;
	if (g_ICANDIParams.Desinusoid == 1) {
		// presinusoid stimulus start location in x direction
		for (i = 0; i < aoslo_movie.width; i ++) {
			if (xh < g_PatchParamsA.StretchFactor[i]) break;
		}
		xhp = i;		// stimulus center location in sinusoidal space

		// presinusoid stimulus end location in x direction
		for (i = 0; i < aoslo_movie.width; i ++) {
			if (x0 < g_PatchParamsA.StretchFactor[i]) break;
		}
		x0p = i;		// stimulus center location in sinusoidal space

		if (!g_bConstPwr || channelID == STIM_CHANNEL_IR) {			// constant size (N.D.20191010)	(always const. size for IR, ND 20191119)
			float HalfStim = stimWidth/2.f;
			
			// presinusoid stimulus left boundary
			xLp = x0-floor(HalfStim);
			if (xLp >= 0) {
				for (i = 0; i < aoslo_movie.width; i ++) {
					if (xLp < g_PatchParamsA.StretchFactor[i]) break;
				}
				xLp = x0p - i;
			} else {
				xLp = x0p;
			}

			// presinusoid stimulus right boundary
			xRp = x0+ceil(HalfStim);
			if (xRp < aoslo_movie.width) {
				for (i = 0; i < aoslo_movie.width; i ++) {
					if (xRp < g_PatchParamsA.StretchFactor[i]) break;
				}
				xRp = i - x0p - 1;
			} else {
				xRp = aoslo_movie.width-1-x0p;
			}
		}

		else{						// constant power (N.D.20191010)
			float HalfStimA = stimWidth/2.f;
			float HalfStimB = stimWidth/2.f-1;
			xLp = floor(HalfStimA);
			xRp = ceil(HalfStimB);
		}

	} else {
		x0p = x0;
		xhp = xh;
		xLp = stimWidth/2;
		xRp = stimWidth/2-1;
	}
	
	deltaxp = xLp + xRp + 1;		// stimulus width in sinusoidal space

	if (channelID == STIM_CHANNEL_IR) {
		// for whatever reason, stimulus width in y direction needs one additional null operation to clear
		// junk data in the FPGA block memory. This is still a mystery!!!!!!!!!!!!!!!
		g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, ny1, ny2, ny3, stimWidth, LINE_INTERVAL, STIM_CHANNEL_NU);
	}
	// write stimulus vertical location 
	g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, ny1, ny2, ny3, stimWidth, LINE_INTERVAL, channelID);

	// write pre-sinusoidal lookup table
	if (channelID == STIM_CHANNEL_IR) {
		// write stimulus horizontal location
		g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, true, x0p, xhp, xLp, xRp, ny1, ny2, ny3, g_ICANDIParams.Desinusoid, channelID);

		// load IR lookup table for warping stimulus pattern
		lut_loc_buf1 = new unsigned short [deltaxp+2];

		// with desinusoiding
		if (g_ICANDIParams.Desinusoid == STIM_CHANNEL_IR) {
			if (x0 > 0 && x0p > 0) {
				x1 = x0p - xLp;
				x2 = x0p + xRp;
				
				/*if (g_bConstPwr)
					x0 = linStretch[x1];								// constant power (N.D.20191010)	 (always const. size for IR, ND 20191119)
				else*/
					x0 = (int)g_PatchParamsA.StretchFactor[x1];				// constant size(N.D.20191010)

				for (i = x1; i <= x2; i ++) {
					/*if (g_bConstPwr)
						cent = linStretch[i]-x0;							// constant power (N.D.20191010)	 (always const. size for IR, ND 20191119)
					else*/
						cent = g_PatchParamsA.StretchFactor[i]-x0;			// constant size(N.D.20191010)
					
					lut_loc_buf1[i-x1] = (unsigned short)floor(cent);
				}

			} else {
				for (i = 0; i < deltaxp; i ++) {
					lut_loc_buf1[i] = i;
				}
			}

		} else {
			for (i = 0; i < deltaxp; i ++) {
				lut_loc_buf1[i] = i;
			}
		}

//		fprintf(g_fp, "ir: %d/%d, (%d,%d) (%d,%d,%d)\n", g_frameIndex, g_BlockID, xc0, xch, y1, y2, y3);

		// upload presinusoidal LUT
		g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, stimWidth, deltaxp, lut_loc_buf1, 7);
		g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);

		delete [] lut_loc_buf1;

		return;

	} else if (channelID == STIM_CHANNEL_GR) {
		laser_power = 4*aoslo_movie.nLaserPowerGreen;
	} else if (channelID == STIM_CHANNEL_RD) {
		laser_power = 4*aoslo_movie.nLaserPowerRed;
	}

	// write stimulus horizontal location
	g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, true, x0p, xhp, xLp, xRp, ny1, ny2, ny3, g_ICANDIParams.Desinusoid, channelID);

	lut_loc_buf1 = new unsigned short [deltaxp+2];
	lut_loc_buf2 = new unsigned short [deltaxp+2];

	if (g_ICANDIParams.Desinusoid == 1) {
		// calculate multiplication of pixel weights and laser power
		if (!g_bConstPwr){				// constant size (N.D.20191010)
			for (i = x0p-xLp; i <= x0p+xRp; i ++) {
				cent = g_PatchParamsA.StretchFactor[i];
				w1 = ceil(cent)-cent;
				w2 = cent-floor(cent);
				nw1 = (int)(w1*laser_power);
				nw2 = (int)(w2*laser_power);

				if (channelID == STIM_CHANNEL_GR)			// Bug Fix, N.D.20191010 (Calculation was not written to aoslo_movie.weightsRed)
					aoslo_movie.weightsGreen[i-x0p+xLp] = (nw1 << 16) | nw2;

				else if (channelID == STIM_CHANNEL_RD)
					aoslo_movie.weightsRed[i-x0p+xLp] = (nw1 << 16) | nw2;
			}
		}

		else {							// constant power (N.D.20191010)
			nw1 = laser_power >> 1;					
			nw2 = nw1 << 16;
			if (channelID == STIM_CHANNEL_GR){
				for (i = 0; i < stimWidth; i ++)
					aoslo_movie.weightsGreen[i] = nw1 | nw2;
					}

			else if (channelID == STIM_CHANNEL_RD){
				for (i = 0; i < stimWidth; i ++)
					aoslo_movie.weightsRed[i] = nw1 | nw2;
			}
		}

		if (x0 > 0 && x0p > 0) {
			x1 = x0p - xLp;
			x2 = x0p + xRp;

			if (g_bConstPwr)
				x0 = linStretch[x1];									// constant power (N.D.20191010)
			else
				x0 = (int)g_PatchParamsA.StretchFactor[x1];				// constant size (N.D.20191010)
			
			
			for (i = x1; i <= x2; i ++) {
				
				if (!g_bConstPwr){				// constant size (N.D.20191010)
					cent = g_PatchParamsA.StretchFactor[i]-x0;
					lut_loc_buf2[i-x1] = (unsigned short)ceil(cent);
				}

				else{							// constant power (N.D.20191010)
					cent = linStretch[i]-x0;			
					lut_loc_buf2[i-x1] = (unsigned short)floor((cent));
				}

				lut_loc_buf1[i-x1] = (unsigned short)floor(cent);
			}

		} else {
			for (i = 0; i < deltaxp; i ++) {
				lut_loc_buf2[i] = lut_loc_buf1[i] = i;
			}
		}

	} else {
		for (i = 0; i < stimWidth; i ++) {
			nw1 = laser_power >> 1;
			nw2 = nw1 << 16;
			aoslo_movie.weightsGreen[i] = nw1 | nw2;
		}

		for (i = 0; i < deltaxp; i ++) {
			lut_loc_buf2[i] = lut_loc_buf1[i] = i;
		}
	}

	if (channelID == STIM_CHANNEL_GR) {

		// upload warp LUT for green stimulus pattern
		g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, stimWidth, deltaxp, lut_loc_buf1, 3);
		g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, stimWidth, deltaxp, lut_loc_buf2, 6);

		// write green pixel weights 
		g_objVirtex5BMD.AppWriteWarpWeights(g_hDevVirtex5, deltaxp, aoslo_movie.weightsGreen, 1);
		g_bGRstim = TRUE;

	} else if (channelID == STIM_CHANNEL_RD) {

		// upload warp LUT for green stimulus pattern
		g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, stimWidth, deltaxp, lut_loc_buf1, 2);
		g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, stimWidth, deltaxp, lut_loc_buf2, 5);

		// write red pixel weights 
		g_objVirtex5BMD.AppWriteWarpWeights(g_hDevVirtex5, deltaxp, aoslo_movie.weightsRed, 0);
		g_bRDstim = TRUE;
	}

	delete [] lut_loc_buf1;
	delete [] lut_loc_buf2;
}


// This function is for delivering stimulus with GPU-based FFT algorithm.
// There is no need to have a separate thread for stimulus delivery with this solution
// because stabilization has been synchronized data stripes from the FPGA board.
// This is the fastest situation we can obtain, where stimulus locations are
// retrieved directly from stripe locations without additional computation.
void g_StimulusDeliveryFFT(int sx, int sy, BOOL bStimulus, int blockID)
{
	double      xprime, yprime;
	static double xpLockIR, ypLockIR, xpLockGR, ypLockGR, xpLockRD, ypLockRD;
	int         i, x, y, x_ir, x_gr, x_rd, x0, x0ir, x0gr, x0rd, y1, y2, y3;
	int         x1ir, x2ir, x1rd, x2rd, x1gr, x2gr, power_rd, power_gr, nw1, nw2;
	int         slice_height_a, slice_height_b;
	int         rdShiftY, grShiftY;
	float       cent, w1, w2;
	static int  y1ir, y2ir, y3ir, x_ubir, x_dbir;
	static int  y1gr, y2gr, y3gr, x_ubgr, x_dbgr;
	static int  y1rd, y2rd, y3rd, x_ubrd, x_dbrd;
	static int  slice_start_old_ir, slice_start_old_gr, slice_start_old_rd;

	//g_bIRstim = g_bRDstim = g_bGRstim = FALSE;

	// stabilization on with non-zero gain
	if (g_bFFTIsRunning == TRUE && fabs(aoslo_movie.fStabGainStim) > 0.001) {
		xprime = sx*cos(aoslo_movie.nRotateLocation*3.1415926/180.0) + 
				 sy*sin(aoslo_movie.nRotateLocation*3.1415926/180.0);
		yprime =-sx*sin(aoslo_movie.nRotateLocation*3.1415926/180.0) + 
				 sy*cos(aoslo_movie.nRotateLocation*3.1415926/180.0);

		// ---------------------------------
		// IR stimulus
		// ---------------------------------
		x = g_StimulusPos.x - dewarp_x;		// position X on the reference frame
		y = g_StimulusPos.y - dewarp_y;		// position Y on the reference frame
		// valid stimulus location
		if (x > 0 && x < aoslo_movie.width && y > 0 && y < aoslo_movie.height) {
			
			// -------------------------------
			// process IR stimulus
			// -------------------------------
			if (g_bStimProjectionIR) {
				slice_height_a  = (blockID+2) * LINE_INTERVAL;

				if (aoslo_movie.stim_num_yir > 0) {
					if (slice_height_a-slice_start_old_ir>LINE_INTERVAL &&
						aoslo_movie.stim_num_yir != aoslo_movie.stim_ir_ny/LINE_INTERVAL+1) {
						slice_height_a = slice_start_old_ir + LINE_INTERVAL;
					}
					slice_start_old_ir = slice_height_a;
				}

				if (aoslo_movie.stim_num_yir>0) {
					y1 = aoslo_movie.stim_loc_yir[aoslo_movie.stim_num_yir-1];
					if (aoslo_movie.stim_num_yir > aoslo_movie.stim_ir_ny/LINE_INTERVAL) {
						xpLockIR = xprime;	// lock (x',y') as the center of the stimulus center
						ypLockIR = yprime;
					}
				}
			} else {
				slice_height_a  = (blockID+2) * LINE_INTERVAL;

				y1 = y - floor(aoslo_movie.stim_ir_ny/2.f);      // start location of ir stimulus
				y2 = y + ceil(aoslo_movie.stim_ir_ny/2.f);		 // end location of ir stimulus
				y3 = -1;
				xpLockIR = xprime;
				ypLockIR = yprime;
			}
			slice_height_b  = slice_height_a + LINE_INTERVAL;

			y1 += (int)(aoslo_movie.fStabGainStim*ypLockIR);
/*
			if (g_bTimingTest) {
				g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
				fprintf(g_fpTimeLog, "boundary-O   :%d, BID-%d, %d:%d:%d:%5.3f,    (%d,%d) (%d,%d) (%d,%d)\n", 
					g_frameIndex, blockID, hours, minutes, seconds, milliseconds, 
					y, y1, slice_height_a, slice_height_b, sx, sy);
			}
*/

			if (y1 >= slice_height_a && y1 <= slice_height_b) {

				y1  -= (int)(aoslo_movie.fStabGainStim*ypLockIR);
				y1  += (int)(aoslo_movie.fStabGainStim*yprime);
				x0ir = x + (int)(aoslo_movie.fStabGainStim*xprime);
				y2ir = y1;
				x_ubir = x0ir;
			/*
				if (g_bTimingTest) {
					fprintf(g_fpTimeLog, "  boundary-i: (%d,%d,%d), (%f:%f), %d\n", 
						y1, slice_height_a, slice_height_b, ypLockIR, yprime, aoslo_movie.stim_num_yir);
				}
*/
				// now update stimulus information on FPGA
				if (g_bStimProjectionIR) {
					if (aoslo_movie.stim_num_yir>0) {
						aoslo_movie.stim_num_yir --;
					}
					if (aoslo_movie.stim_num_yir==0) {
						y3ir = y1 + aoslo_movie.stim_ir_ny%LINE_INTERVAL - 2;
					} else if (aoslo_movie.stim_num_yir==aoslo_movie.stim_ir_ny/LINE_INTERVAL) {
						y3ir = 0;
						y1ir = y2ir;
					} else {
						y3ir = 0;
					}
				} else {
					y2 += (int)(aoslo_movie.fStabGainStim*yprime);
				}

				if (bStimulus && g_bStimulusOn && aoslo_movie.FlashOnFlag) {
					if (g_bStimProjectionIR) {
						if (aoslo_movie.stim_num_yir<aoslo_movie.stim_ir_ny/LINE_INTERVAL) {
							g_UpdateStimOnFPGA(x_dbir, x_ubir, aoslo_movie.stim_ir_nx, y1ir, y2ir, y3ir, STIM_CHANNEL_IR);
/*
							if (g_bTimingTest) {
								g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
								fprintf(g_fpTimeLog, "STIMULUS IR   :%d, BID-%d, %d:%d:%d:%5.3f,    (%d,%d,%d) (%d,%d), ID:%d\n", 
									g_frameIndex, blockID, hours, minutes, seconds, milliseconds, 
									y1ir, y2ir, y3ir, x_dbir, x_ubir, aoslo_movie.stim_num_yir);
							}
*/
						} 

						y1ir   = y2ir;
						x_dbir = x_ubir;
					} else {
						g_UpdateStimOnFPGA(x0ir, x0ir, aoslo_movie.stim_ir_nx, y1, y2, y3, STIM_CHANNEL_IR);
/*
						if (g_bTimingTest) {
							g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
							fprintf(g_fpTimeLog, "stimulus ir   :%d, BID-%d, %d:%d:%d:%5.3f,    (%d,%d,%d) (%d,%d)\n", 
								g_frameIndex, blockID, hours, minutes, seconds, milliseconds, 
								y1, y2, y3, x0ir, x0ir);
						}
						*/
					}
				}

				//save the new location for presenting stimulus at gain 0
				g_StimulusPos0G.x = sx + g_StimulusPos.x;
				g_StimulusPos0G.y = sy + g_StimulusPos.y;	
				
				// calculate stimulus locations
				if (aoslo_movie.bOneFrameDelay) {
					aoslo_movie.StimulusDeltaX = aoslo_movie.StimulusDeltaXOld;
					aoslo_movie.StimulusDeltaY = aoslo_movie.StimulusDeltaYOld;
				} else {
					aoslo_movie.StimulusDeltaX = (float)sx;
					aoslo_movie.StimulusDeltaY = (float)sy;
				}

			}


		}

		// ---------------------------------
		// green stimulus (Bonn: Frame_Rotation = 0, Berkeley = 1)
		// ---------------------------------
		if (g_ICANDIParams.FRAME_ROTATION) {
			// position X on the reference frame
			x = g_StimulusPos.x - dewarp_x - g_ICANDIParams.RGBClkShifts[1].y - g_Channel2Shift.y;
			// position Y on the reference frame
			y = g_StimulusPos.y - dewarp_y + g_ICANDIParams.RGBClkShifts[1].x + g_Channel2Shift.x;
		}
		else {
			// position X on the reference frame
			x = g_StimulusPos.x - dewarp_x - g_ICANDIParams.RGBClkShifts[1].x + g_Channel2Shift.x;
			// position Y on the reference frame
			y = g_StimulusPos.y - dewarp_y - g_ICANDIParams.RGBClkShifts[1].y + g_Channel2Shift.y;
		}
//		x = g_StimulusPos.x - dewarp_x;		// position X on the reference frame
//		y = g_StimulusPos.y - dewarp_y;		// position Y on the reference frame

		if (x > 0 && x < aoslo_movie.width && y > 0 && y < aoslo_movie.height) {
			// -------------------------------
			// process Green stimulus
			// -------------------------------
			if (g_bStimProjectionGR) {
				slice_height_a  = (blockID+2) * LINE_INTERVAL;
				
				if (aoslo_movie.stim_num_ygr > 0) {
					if (slice_height_a-slice_start_old_gr>LINE_INTERVAL &&
						aoslo_movie.stim_num_ygr != aoslo_movie.stim_gr_ny/LINE_INTERVAL+1) {
						slice_height_a = slice_start_old_gr + LINE_INTERVAL;
					}
					slice_start_old_gr = slice_height_a;
				}

				if (aoslo_movie.stim_num_ygr>0) {
					y1 = aoslo_movie.stim_loc_ygr[aoslo_movie.stim_num_ygr-1];
					if (aoslo_movie.stim_num_ygr > aoslo_movie.stim_gr_ny/LINE_INTERVAL) {
						xpLockGR = xprime;	// lock (x',y') as the center of the stimulus center
						ypLockGR = yprime;
					}
				}
			} else {
				slice_height_a  = (blockID+1) * LINE_INTERVAL;

				y1 = y - floor(aoslo_movie.stim_gr_ny/2.f);		// start location of green stimulus
				y2 = y + ceil(aoslo_movie.stim_gr_ny/2.f);		// end location of green stimulus
				y3 = -1;
				xpLockGR = xprime;
				ypLockGR = yprime;
			}
			slice_height_b  = slice_height_a + LINE_INTERVAL;

			y1 += (int)(aoslo_movie.fStabGainStim*ypLockGR);
			if (y1 >= slice_height_a && y1 <= slice_height_b) {

				y1  -= (int)(aoslo_movie.fStabGainStim*ypLockGR);
				y1  += (int)(aoslo_movie.fStabGainStim*yprime);
				x0gr = x + (int)(aoslo_movie.fStabGainStim*xprime);
				y2gr = y1;
				x_ubgr = x0gr;
			
				// now update stimulus information on FPGA
				if (g_bStimProjectionGR) {
					if (aoslo_movie.stim_num_ygr>0) {
						aoslo_movie.stim_num_ygr --;
					}
					if (aoslo_movie.stim_num_ygr==0) {
						y3gr = y1 + aoslo_movie.stim_gr_ny%LINE_INTERVAL - 2;
					} else if (aoslo_movie.stim_num_ygr==aoslo_movie.stim_gr_ny/LINE_INTERVAL) {
						y3gr = 0;
						y1gr = y2gr;
					} else {
						y3gr = 0;
					}
				} else {
					y2 += (int)(aoslo_movie.fStabGainStim*yprime);
				}

				//x0gr = x + g_Channel2Shift.x + (int)(aoslo_movie.fStabGainStim*xprime); // bug: another redundant g_Channel2Shift!
				x0gr = x + (int)(aoslo_movie.fStabGainStim*xprime);

				if (bStimulus && g_bStimulusOn && aoslo_movie.FlashOnFlag) {
					if (g_bStimProjectionGR) {
						if (aoslo_movie.stim_num_ygr<aoslo_movie.stim_gr_ny/LINE_INTERVAL) {
							g_UpdateStimOnFPGA(x_dbgr, x_ubgr, aoslo_movie.stim_gr_nx, y1gr, y2gr, y3gr, STIM_CHANNEL_GR);
						}
						y1gr   = y2gr;
						x_dbgr = x_ubgr;

					} else {
						g_UpdateStimOnFPGA(x0gr, x0gr, aoslo_movie.stim_gr_nx, y1, y2, y3, STIM_CHANNEL_GR);
					}
				}
			}
		}


		// ---------------------------------
		// red stimulus
		// ---------------------------------
		if (g_ICANDIParams.FRAME_ROTATION) {
			// position X on the reference frame
			x = g_StimulusPos.x - dewarp_x - g_ICANDIParams.RGBClkShifts[0].y - g_Channel1Shift.y;
			// position Y on the reference frame
			y = g_StimulusPos.y - dewarp_y + g_ICANDIParams.RGBClkShifts[0].x + g_Channel1Shift.x;
		}
		else {
			// position X on the reference frame
			x = g_StimulusPos.x - dewarp_x - g_ICANDIParams.RGBClkShifts[0].x + g_Channel1Shift.x;
			// position Y on the reference frame
			y = g_StimulusPos.y - dewarp_y - g_ICANDIParams.RGBClkShifts[0].y + g_Channel1Shift.y;
		}
//		x = g_StimulusPos.x - dewarp_x;		// position X on the reference frame
//		y = g_StimulusPos.y - dewarp_y;		// position Y on the reference frame

		if (x > 0 && x < aoslo_movie.width && y > 0 && y < aoslo_movie.height) {
			// -------------------------------
			// process red stimulus
			// -------------------------------
			if (g_bStimProjectionRD) {
				slice_height_a  = (blockID+2) * LINE_INTERVAL;
				
				if (aoslo_movie.stim_num_yrd > 0) {
					if (slice_height_a-slice_start_old_rd>LINE_INTERVAL &&
						aoslo_movie.stim_num_yrd != aoslo_movie.stim_rd_ny/LINE_INTERVAL+1) {
						slice_height_a = slice_start_old_rd + LINE_INTERVAL;
					}
					slice_start_old_rd = slice_height_a;
				}

				if (aoslo_movie.stim_num_yrd>0) {
					y1 = aoslo_movie.stim_loc_yrd[aoslo_movie.stim_num_yrd-1];
					if (aoslo_movie.stim_num_yrd > aoslo_movie.stim_rd_ny/LINE_INTERVAL) {
						xpLockRD = xprime;	// lock (x',y') as the center of the stimulus center
						ypLockRD = yprime;
					}
				}
			} else {
				slice_height_a  = (blockID+1) * LINE_INTERVAL;

				y1 = y - floor(aoslo_movie.stim_rd_ny/2.f);				     // start location of red stimulus
				y2 = y + ceil(aoslo_movie.stim_rd_ny/2.f);				     // end location of red stimulus
				y3 = -1;
				xpLockRD = xprime;
				ypLockRD = yprime;
			}
			slice_height_b  = slice_height_a + LINE_INTERVAL;

			y1 += (int)(aoslo_movie.fStabGainStim*ypLockRD);
			if (y1 >= slice_height_a && y1 <= slice_height_b) {

				y1  -= (int)(aoslo_movie.fStabGainStim*ypLockRD);
				y1  += (int)(aoslo_movie.fStabGainStim*yprime);
				x0rd = x + (int)(aoslo_movie.fStabGainStim*xprime);
				y2rd = y1;
				x_ubrd = x0rd;
			
				// now update stimulus information on FPGA
				if (g_bStimProjectionRD) {
					if (aoslo_movie.stim_num_yrd>0) {
						aoslo_movie.stim_num_yrd --;
					}
					if (aoslo_movie.stim_num_yrd==0) {
						y3rd = y1 + aoslo_movie.stim_rd_ny%LINE_INTERVAL - 2;
					} else if (aoslo_movie.stim_num_yrd==aoslo_movie.stim_rd_ny/LINE_INTERVAL) {
						y3rd = 0;
						y1rd = y2rd;
					} else {
						y3rd = 0;
					}
				} else {
					y2 += (int)(aoslo_movie.fStabGainStim*yprime);
				}

				//x0rd = x + g_Channel1Shift.x + (int)(aoslo_movie.fStabGainStim*xprime); // bug: another redundant g_Channel1Shift!
				x0rd = x + (int)(aoslo_movie.fStabGainStim*xprime);

				if (bStimulus && g_bStimulusOn && aoslo_movie.FlashOnFlag && g_bGRstim) {
					if (g_bStimProjectionRD) {
						if (aoslo_movie.stim_num_yrd<aoslo_movie.stim_rd_ny/LINE_INTERVAL) {
							g_UpdateStimOnFPGA(x_dbrd, x_ubrd, aoslo_movie.stim_rd_nx, y1rd, y2rd, y3rd, STIM_CHANNEL_RD);
						}

						y1rd   = y2rd;
						x_dbrd = x_ubrd;
					} else {
						g_UpdateStimOnFPGA(x0rd, x0rd, aoslo_movie.stim_rd_nx, y1, y2, y3, STIM_CHANNEL_RD);
					}
				}
			}
			
			// check if matlab experiment is running (set conditions more restrict)
			if (g_bMatlabCtrl) {
				// check if stimulus output channel is green/red or green cascade
				if (g_nLocX[1][1] !=0  || g_nLocX[2][1] !=0) {
					// check if update was successfull
					if(g_bRDstim && g_bGRstim) {
						SetEvent(g_EventStimPresentFlag);
						g_bDrawCrossMarker = TRUE;
						//aoslo_movie.WriteMarkFlag = TRUE;
					}
				} else {
					// experiment is only IR coded, or correct delivery in Green/Red channel is not important
					SetEvent(g_EventStimPresentFlag);
					g_bDrawCrossMarker = TRUE;
				}
			} else {
				SetEvent(g_EventStimPresentFlag);
				g_bDrawCrossMarker = TRUE;
					//aoslo_movie.WriteMarkFlag = TRUE;
			}

		aoslo_movie.stimulus_flag = TRUE;
		//	g_bIRstim = g_bRDstim = g_bGRstim = FALSE;

			//aoslo_movie.stimulus_audio_flag = aoslo_movie.stimulus_flag = TRUE;		// testing an idea from Pavan

			if (g_bRecord);
			//	Out32(57424,3);
		} // valid stimulus location

		// the feature of one-frame delay is applied to simple stimulus only
		// extended stimulus will be distorted obnormally if one-frame delay is used
		if (aoslo_movie.bOneFrameDelay) {
			aoslo_movie.StimulusDeltaXOld = (float)sx;
			aoslo_movie.StimulusDeltaYOld = (float)sy;
		}
	
	} else {

		// ----------------------------
		// zero-gain stimulus delivery
		// ----------------------------

		if (g_StimulusPos0G.x > 0 && g_StimulusPos0G.y > 0) {

			aoslo_movie.StimulusDeltaX = 0;
			aoslo_movie.StimulusDeltaY = 0;
			g_bCritical = TRUE;

			// -------------------------------------------------
			// we now add stimulus delivery here
			// -------------------------------------------------

			x = g_StimulusPos0G.x - dewarp_x;		// position X on the reference frame
			y = g_StimulusPos0G.y - dewarp_y;		// position Y on the reference frame

			if (x > 0 && x < aoslo_movie.width && y > 0 && y < aoslo_movie.height && 
				g_bStimulusOn == TRUE && aoslo_movie.FlashOnFlag == TRUE) {
				power_rd = 4*aoslo_movie.nLaserPowerRed;
				power_gr = 4*aoslo_movie.nLaserPowerGreen;
				x1rd = x2rd = aoslo_movie.stim_rd_nx/2;
				x1gr = x2gr = aoslo_movie.stim_gr_nx/2;
				x1ir = x2ir = aoslo_movie.stim_ir_nx/2;
				if (g_ICANDIParams.Desinusoid == 1) {
					x0   = x;
//					x0gr = x+g_Channel2Shift.x;
//					x0rd = x+g_Channel1Shift.x;
					if (g_ICANDIParams.FRAME_ROTATION) {
						x0gr = x-g_Channel2Shift.y-g_ICANDIParams.RGBClkShifts[1].y;
						x0rd = x-g_Channel1Shift.y-g_ICANDIParams.RGBClkShifts[0].y;
					} else {
						x0gr = x+g_Channel2Shift.x+g_ICANDIParams.RGBClkShifts[1].x;
						x0rd = x+g_Channel1Shift.x+g_ICANDIParams.RGBClkShifts[0].x;
					}
					x0ir = x;

					for (i = 0; i < aoslo_movie.width; i ++) {
						if (x0 < g_PatchParamsA.StretchFactor[i]) break;
					}
					x = i;

					// presinusoid red channel stimulus
					aoslo_movie.x_cc_rd = x0rd;
					for (i = 0; i < aoslo_movie.width; i ++) {
						if (x0rd < g_PatchParamsA.StretchFactor[i]) break;
					}
					x_rd = i;

					if (!g_bConstPwr){		// constant size (N.D.20191010)
						float HalfStim = aoslo_movie.stim_rd_nx/2.f;
						
						x1rd = x0rd-floor(HalfStim);
						if (x1rd >= 0) {
							for (i = 0; i < aoslo_movie.width; i ++) {
								if (x1rd < g_PatchParamsA.StretchFactor[i]) break;
							}
							x1rd = x_rd - i;
						} else {
							x1rd = x_rd;
						}
						
						x2rd = x0rd+ceil(HalfStim);
						if (x2rd < aoslo_movie.width) {
							for (i = 0; i < aoslo_movie.width; i ++) {
								if (x2rd < g_PatchParamsA.StretchFactor[i]) break;
							}
							x2rd = i - x_rd - 1;
						} else {
							x2rd = aoslo_movie.width-1-x_rd;
						}

						// calculate multiplication of pixel weights and laser power
						for (i = x_rd-x1rd; i <= x_rd+x2rd; i ++) {							// constant size (N.D.20191010)
							cent = g_PatchParamsA.StretchFactor[i];
							w1 = ceil(cent)-cent;											
							w2 = cent-floor(cent);										
							nw1 = (int)(w1*power_rd);
							nw2 = (int)(w2*power_rd);
							aoslo_movie.weightsRed[i-x_rd+x1rd] = (nw1 << 16) | nw2;
						}

					}

					else {			// constant power (N.D.20191010)
						float HalfStimA = aoslo_movie.stim_rd_nx/2.f;
						float HalfStimB = aoslo_movie.stim_rd_nx/2.f-1;
						x1rd = floor(HalfStimA);
						x2rd = ceil(HalfStimB);										
						nw1 = power_rd >> 1;
						nw2 = nw1 << 16;
						for (i = 0; i < aoslo_movie.stim_rd_nx; i ++)
							aoslo_movie.weightsRed[i] = nw1 | nw2;
					}
				
					// presinusoid green channel stimulus
					aoslo_movie.x_cc_gr = x0gr;
					for (i = 0; i < aoslo_movie.width; i ++) {
						if (x0gr < g_PatchParamsA.StretchFactor[i]) break;
					}
					x_gr = i;
					
					if (!g_bConstPwr){	// constant size (N.D.20191010)
						float HalfStim = aoslo_movie.stim_gr_nx/2.f;
						
						x1gr = x0gr-floor(HalfStim);
						if (x1gr >= 0) {
							for (i = 0; i < aoslo_movie.width; i ++) {
								if (x1gr < g_PatchParamsA.StretchFactor[i]) break;
							}
							x1gr = x_gr - i;
						} else {
							x1gr = x_gr;
						}
						
						x2gr = x0gr+ceil(HalfStim);
						if (x2gr < aoslo_movie.width) {
							for (i = 0; i < aoslo_movie.width; i ++) {
								if (x2gr < g_PatchParamsA.StretchFactor[i]) break;
							}
							x2gr = i - x_gr - 1;
						} else {
							x2gr = aoslo_movie.width-1-x_gr;
						}

						// calculate multiplication of pixel weights and laser power
						for (i = x_gr-x1gr; i <= x_gr+x2gr; i ++) {							// constant size (N.D.20191010)
							cent = g_PatchParamsA.StretchFactor[i];
							w1 = ceil(cent)-cent;
							w2 = cent-floor(cent);
							nw1 = (int)(w1*power_gr);
							nw2 = (int)(w2*power_gr);
							aoslo_movie.weightsGreen[i-x_gr+x1gr]  = (nw1 << 16) | nw2;
						}
					}

					else {				// constant power (N.D.20191010)
						float HalfStimA = aoslo_movie.stim_gr_nx/2.f;
						float HalfStimB = aoslo_movie.stim_gr_nx/2.f-1;
						x1gr = floor(HalfStimA);
						x2gr = ceil(HalfStimB);	
						nw1 = power_gr >> 1;
						nw2 = nw1 << 16;
						for (i = 0; i < aoslo_movie.stim_gr_nx; i ++)
							aoslo_movie.weightsGreen[i]  = nw1 | nw2;
					}

					// presinusoid IR channel stimulus
					aoslo_movie.x_cc_ir = x0ir;
					for (i = 0; i < aoslo_movie.width; i ++) {
						if (x0ir < g_PatchParamsA.StretchFactor[i]) break;
					}
					x_ir = i;
					
					//if (!g_bConstPwr){			// constant size (N.D.20191010)	 (always const. size for IR, ND 20191119)
						float HalfStim = aoslo_movie.stim_ir_nx/2.f;
						
						x1ir = x0ir-floor(HalfStim);
						if (x1ir >= 0) {
							for (i = 0; i < aoslo_movie.width; i ++) {
								if (x1ir < g_PatchParamsA.StretchFactor[i]) break;
							}
							x1ir = x_ir - i;
						} else {
							x1ir = x_ir;
						}
						
						x2ir = x0ir+ceil(HalfStim);
						if (x2ir < aoslo_movie.width) {
							for (i = 0; i < aoslo_movie.width; i ++) {
								if (x2ir < g_PatchParamsA.StretchFactor[i]) break;
							}
							x2ir = i - x_ir - 1;
						} else {
							x2ir = aoslo_movie.width-1-x_ir;
						}
					/*}

					else {						// constant power (N.D.20191018)	 (always const. size for IR, ND 20191119)
						float HalfStimA = aoslo_movie.stim_ir_nx/2.f;
						float HalfStimB = aoslo_movie.stim_ir_nx/2.f-1;
						x1ir = floor(HalfStimA);
						x2ir = ceil(HalfStimB);
					}*/

				} else {
					x_ir = x;
//					x0gr = x+g_Channel2Shift.x;
//					x0rd = x+g_Channel1Shift.x;
					if (g_ICANDIParams.FRAME_ROTATION) {
						x0gr = x-g_Channel2Shift.y-g_ICANDIParams.RGBClkShifts[1].y;
						x0rd = x-g_Channel1Shift.y-g_ICANDIParams.RGBClkShifts[0].y;
					} else {
						x0gr = x+g_Channel2Shift.x+g_ICANDIParams.RGBClkShifts[1].x;
						x0rd = x+g_Channel1Shift.x+g_ICANDIParams.RGBClkShifts[0].x;
					}

					aoslo_movie.x_cc_gr = x0gr;
					x_gr = x0gr;
					x1gr = aoslo_movie.stim_gr_nx/2;
					x2gr = aoslo_movie.stim_gr_nx/2-1;

					aoslo_movie.x_cc_rd = x0rd;
					x_rd = x0rd;
					x1rd = aoslo_movie.stim_rd_nx/2;
					x2rd = aoslo_movie.stim_rd_nx/2-1;

					x0ir = x;
					aoslo_movie.x_cc_ir = x0ir;
					x_ir = x0ir;
					x1ir = aoslo_movie.stim_ir_nx/2;
					x2ir = aoslo_movie.stim_ir_nx/2-1;

					for (i = 0; i < g_iStimulusSizeX_RD; i ++) {
						nw1 = power_rd >> 1;
						nw2 = nw1 << 16;
						aoslo_movie.weightsRed[i] = nw1 | nw2;
					}
					for (i = 0; i < g_iStimulusSizeX_GR; i ++) {
						nw1 = power_gr >> 1;
						nw2 = nw1 << 16;
						aoslo_movie.weightsGreen[i]  = nw1 | nw2;
					}
				}

				aoslo_movie.x_center_gr= x_gr;
				aoslo_movie.x_left_gr  = x1gr;	
				aoslo_movie.x_right_gr = x2gr;
				g_iStimulusSizeX_GR    = x2gr+x1gr+1;

				aoslo_movie.x_center_rd= x_rd;
				aoslo_movie.x_left_rd  = x1rd;
				aoslo_movie.x_right_rd = x2rd;
				g_iStimulusSizeX_RD    = x2rd+x1rd+1;

				aoslo_movie.x_center_ir= x_ir;
				aoslo_movie.x_left_ir  = x1ir;
				aoslo_movie.x_right_ir = x2ir;
				g_iStimulusSizeX_IR    = x2ir+x1ir+1;

				if (g_ICANDIParams.FRAME_ROTATION) {
					rdShiftY = -g_ICANDIParams.RGBClkShifts[0].x - g_Channel1Shift.x;
					grShiftY = -g_ICANDIParams.RGBClkShifts[1].x - g_Channel2Shift.x;
				} else {
					rdShiftY = -g_ICANDIParams.RGBClkShifts[0].y + g_Channel1Shift.y;
					grShiftY = -g_ICANDIParams.RGBClkShifts[1].y + g_Channel2Shift.y;
				}
				if (aoslo_movie.stim_gr_nx==aoslo_movie.stim_rd_nx && aoslo_movie.stim_gr_ny==aoslo_movie.stim_rd_ny &&
					aoslo_movie.stim_ir_nx==aoslo_movie.stim_rd_nx && aoslo_movie.stim_ir_ny==aoslo_movie.stim_rd_ny) {
						int y1stim, y2stim;
						y1stim = y-floor(aoslo_movie.stim_rd_ny/2.f);		// conserve stimulus heigth (ND, 20191017)
						y2stim = y+ceil(aoslo_movie.stim_rd_ny/2.f);		// conserve stimulus heigth (ND, 20191017)
						g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, y1stim, y2stim, rdShiftY, grShiftY, aoslo_movie.stim_rd_nx, -1, -100);
						g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, true, x_ir, x_ir, x_gr, x_gr, x_rd, x_rd, y1stim, y2stim, 0, rdShiftY, grShiftY, g_ICANDIParams.Desinusoid, x1ir, x2ir, x1gr, x2gr, x1rd, x2rd);

				} else {
					g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, y, rdShiftY, grShiftY, aoslo_movie.stim_ir_nx, aoslo_movie.stim_ir_ny, aoslo_movie.stim_gr_nx, aoslo_movie.stim_gr_ny, aoslo_movie.stim_rd_nx, aoslo_movie.stim_rd_ny);
					g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, true, x_ir, y, x_gr, y+grShiftY, x_rd, y+rdShiftY, aoslo_movie.stim_ir_ny, aoslo_movie.stim_rd_ny, aoslo_movie.stim_gr_ny, g_ICANDIParams.Desinusoid, x1ir, x2ir, x1rd, x2rd, x1gr, x2gr);
				}

				SetEvent(g_EventLoadLUT);

//				g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
//				fprintf(g_fp, "f_cnt:%d, zero gain stimulus,      %d:%d:%d:%5.3f\n", g_sampling_counter, hours, minutes, seconds, milliseconds);

				aoslo_movie.stimulus_flag = TRUE;
				SetEvent(g_EventStimPresentFlag);
				g_bDrawCrossMarker = TRUE;
				
			} // valid stimulus location

		}

	}

//	delete [] delta_x;
//	delete [] delta_y;
}

void CICANDIDoc::GetSysTime(CString &buf){

time_t rawtime;
struct tm * timeinfo;
char buffer [80];
time (&rawtime);
timeinfo = localtime (&rawtime);
strftime (buffer, 80, "%Y_%m_%d_%H_%M_%S_", timeinfo);
buf = buffer;

}

/*
	Video grabbing thread. It includes the following features

	1. Grab video via a AD9980 on the FPGA board. 
	2. Pass image from on-board buffer to PC buffer stripe by stripe
	3. Do desinusoiding on PC (GUI side) line by line
	4. Send image stripe by stripe (16 lines in each stripe) to MSC Server
	5. Read stimulus position back from Server (in real time)
	6. Read deltas of patches back to GUI
	7. Do dewarping on the whole frame for stablization after each V-Sync occurs
	8. Notify four panels for drawing images and figures
 */
DWORD WINAPI CICANDIDoc::ThreadVideoSampling(LPVOID pParam)
{
	int         dewarp_sx, dewarp_sy;
	int         SLICE_HEIGHT;
	int         i;

	CEvent      WaitEvents; // creates time delay events

    DWORD       dwStatus, dwOptions;
    UINT32      vsync_bit, ddmacr, u32Pattern = 0xfeedbeef;//, regRData, regWData, ctrlBits;
    WORD        wSize, wCount;
    BOOL        fIsRead;
    BOOL        fEnable64bit;
    BYTE        bTrafficClass;
	CString     msg;

	// define width and height of the dewarped image
	aoslo_movie.dewarp_sx = aoslo_movie.width  + dewarp_x*2;
	aoslo_movie.dewarp_sy = aoslo_movie.height + dewarp_y*2;
	SLICE_HEIGHT = LINE_INTERVAL;//g_BlockHeight;

	// send video size to views for memory allocations
	g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, MOVIE_HEADER_ID);
	g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, MOVIE_HEADER_ID);
	g_viewDewVideo->SendMessage(WM_MOVIE_SEND, 0, MOVIE_HEADER_ID);
	g_viewDltVideo->SendMessage(WM_MOVIE_SEND, 0, MOVIE_HEADER_ID);

	// if desinusoiding is needed, read in lookup table
	if (g_ICANDIParams.Desinusoid == 1) {
		g_PatchParamsA.UnMatrixIdx = new int   [(aoslo_movie.width+aoslo_movie.height)*2];
		g_PatchParamsA.UnMatrixVal = new float [(aoslo_movie.width+aoslo_movie.height)*2];
		g_PatchParamsA.StretchFactor = new float [aoslo_movie.width];
		if (g_ReadLUT(aoslo_movie.width, aoslo_movie.height, g_PatchParamsA.UnMatrixIdx, g_PatchParamsA.UnMatrixVal, g_PatchParamsA.StretchFactor) == FALSE) {
			delete [] g_PatchParamsA.UnMatrixIdx;
			delete [] g_PatchParamsA.UnMatrixVal;
			delete [] g_PatchParamsA.StretchFactor;
			g_PatchParamsA.UnMatrixIdx = NULL;
			g_PatchParamsA.UnMatrixVal = NULL;
			g_PatchParamsA.StretchFactor = NULL;
			return -1;
		}
	}

	// allocate memory for dewarpped movies
	g_PatchParamsA.img_slice = new BYTE [aoslo_movie.width*aoslo_movie.height];
	//g_PatchParamsB.img_slice = new BYTE [aoslo_movie.width*aoslo_movie.height];
	g_PatchParamsA.img_sliceR= new BYTE [aoslo_movie.width*aoslo_movie.height];
	g_PatchParamsB.img_sliceR= new BYTE [aoslo_movie.width*aoslo_movie.height];
	aoslo_movie.video_in      = new BYTE [aoslo_movie.width*aoslo_movie.height];
	aoslo_movie.video_ins     = new BYTE [aoslo_movie.width*aoslo_movie.height];
	aoslo_movie.video_sparse  = new BYTE [aoslo_movie.width*aoslo_movie.height];
	aoslo_movie.video_ref_g   = new BYTE [aoslo_movie.width*aoslo_movie.height];
	aoslo_movie.video_ref_l   = new BYTE [aoslo_movie.width*aoslo_movie.height];
	aoslo_movie.video_out     = new BYTE [aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy];
	aoslo_movie.video_ref_bk  = new BYTE [aoslo_movie.width*aoslo_movie.height];
	if (g_PatchParamsA.img_slice == NULL || 
		//g_PatchParamsB.img_slice == NULL || 
		g_PatchParamsA.img_sliceR== NULL || 
		g_PatchParamsB.img_sliceR== NULL || 
		aoslo_movie.video_in      == NULL || 
		aoslo_movie.video_ins     == NULL || 
		aoslo_movie.video_sparse  == NULL || 
		aoslo_movie.video_ref_g   == NULL || 
		aoslo_movie.video_ref_l   == NULL || 
		aoslo_movie.video_out     == NULL ||
		aoslo_movie.video_ref_bk  == NULL) {
		AfxMessageBox("Out of memory.", MB_ICONWARNING);
		return -1;
	}
	for (i = 0; i < aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy; i++) aoslo_movie.video_out[i] = 191;
	for (i = 0; i < aoslo_movie.width*aoslo_movie.height; i++) aoslo_movie.video_in[i] = 191;

	dewarp_sx = aoslo_movie.dewarp_sx;
	dewarp_sy = aoslo_movie.dewarp_sy;
	
	g_BlockCounts = aoslo_movie.height/SLICE_HEIGHT;

	// allocate memory for delta-X's and delta-Y's
	g_PatchParamsB.shift_xi   = new float[aoslo_movie.height];
	g_PatchParamsB.shift_yi   = new float[aoslo_movie.height];
	g_PatchParamsB.shift_xy   = new float[aoslo_movie.height];

	if (aoslo_movie.video_saveA1 != NULL) delete [] aoslo_movie.video_saveA1;
	if (aoslo_movie.video_saveA2 != NULL) delete [] aoslo_movie.video_saveA2;
	if (aoslo_movie.video_saveA3 != NULL) delete [] aoslo_movie.video_saveA3;
	aoslo_movie.video_saveA1 = new BYTE [MEMORY_POOL_LENGTH*aoslo_movie.width*aoslo_movie.height];
	aoslo_movie.video_saveA2 = new BYTE [MEMORY_POOL_LENGTH*aoslo_movie.width*aoslo_movie.height];
	aoslo_movie.video_saveA3 = new BYTE [MEMORY_POOL_LENGTH*aoslo_movie.width*aoslo_movie.height];
	if (aoslo_movie.video_saveA1 == NULL || 
		aoslo_movie.video_saveA2 == NULL ||
		aoslo_movie.video_saveA3 == NULL) {
		AfxMessageBox("No enough memory space for saving raw video.", MB_ICONWARNING);
		return -1;
	}

	// allocate memory for saving stabilized video
	if (aoslo_movie.video_saveB1 != NULL) delete [] aoslo_movie.video_saveB1;
	if (aoslo_movie.video_saveB2 != NULL) delete [] aoslo_movie.video_saveB2;
	if (aoslo_movie.video_saveB3 != NULL) delete [] aoslo_movie.video_saveB3;
	aoslo_movie.video_saveB1 = new BYTE [MEMORY_POOL_LENGTH*aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy];
	aoslo_movie.video_saveB2 = new BYTE [MEMORY_POOL_LENGTH*aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy];
	aoslo_movie.video_saveB3 = new BYTE [MEMORY_POOL_LENGTH*aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy];
	if (aoslo_movie.video_saveB1 == NULL || 
		aoslo_movie.video_saveB2 == NULL ||
		aoslo_movie.video_saveB3 == NULL) {
		AfxMessageBox("No enough memory space for saving stabilized video.", MB_ICONWARNING);
		return -1;
	}


	// initialize stimulus position
	aoslo_movie.StimulusDeltaX = 0;
	aoslo_movie.StimulusDeltaY = 0;
	aoslo_movie.StimulusResX   = 0;
	aoslo_movie.StimulusResY   = 0;
	aoslo_movie.CenterX        = 0;
	aoslo_movie.CenterY        = 0;

	g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, SENDING_MOVIE);
	g_viewDewVideo->SendMessage(WM_MOVIE_SEND, 0, SENDING_MOVIE);
	g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, SENDING_MOVIE);
	g_viewDltVideo->SendMessage(WM_MOVIE_SEND, 0, SENDING_MOVIE);

	g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, 0, aoslo_movie.width, 0, 0, 0, -1, -100);
	g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, false, 0, 0, 0, 0, 0, 0, 0, aoslo_movie.width, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    // Get input for user 
    wCount    = g_VideoInfo.tlp_counts;
    fIsRead   = FALSE;
	dwOptions = DMA_FROM_DEVICE;
    
	// Get the max payload size from the device 
    wSize     = g_objVirtex5BMD.DMAGetMaxPacketSize(g_hDevVirtex5, fIsRead) / sizeof(UINT32);
    g_dwTotalCount = (DWORD)wCount * (DWORD)wSize;

    // Open DMA handle 
	BZERO(g_dma);
    dwStatus = g_objVirtex5BMD.VIRTEX5_DMAOpen(g_hDevVirtex5, &g_dma.pBuf, dwOptions, g_dwTotalCount * sizeof(UINT32), &g_dma.hDma);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
		msg.Format("Failed to open DMA handle. Error 0x%lx", dwStatus);
        AfxMessageBox(msg);
        return -1;
    }

    fEnable64bit = FALSE;
    bTrafficClass = 0;
    g_objVirtex5BMD.VIRTEX5_DMADevicePrepare(g_dma.hDma, fIsRead, wSize, wCount, u32Pattern, fEnable64bit, bTrafficClass);

    // Enable DMA interrupts (if not polling) 
    g_objVirtex5BMD.VIRTEX5_DmaIntEnable(g_hDevVirtex5, fIsRead);

    if (!g_objVirtex5BMD.VIRTEX5_IntIsEnabled(g_hDevVirtex5))
    {
        dwStatus = g_objVirtex5BMD.VIRTEX5_IntEnable(g_hDevVirtex5, g_DiagDmaIntHandler);

        if (WD_STATUS_SUCCESS != dwStatus)
        {
            msg.Format("Failed enabling DMA interrupts. Error 0x%lx", dwStatus);
			AfxMessageBox(msg);
            return -1;
        }
    }

	dwStatus = WDC_DMASyncCpu(g_dma.hDma->pDma);

    // Start sampling
	g_objVirtex5BMD.AppStartADC(g_hDevVirtex5);

//	fopen_s(&g_fp, "data.txt", "w");

	g_bFirstVsync = FALSE;
	do {
		ddmacr = g_objVirtex5BMD.VIRTEX5_ReadReg32(g_dma.hDma->hDev, VIRTEX5_LINECTRL_OFFSET);
		vsync_bit = ddmacr & BIT4;
		if (vsync_bit > 0) 
			g_bFirstVsync = TRUE;
	} while (g_bFirstVsync == FALSE);

	do {
		// wait for toolbar/menu item "disconnected" to be clicked
		::WaitForSingleObject(WaitEvents, 10);

	} while (((CICANDIApp*)AfxGetApp())->m_isGrabStarted);

	// clear stimulus location
	g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, 0, aoslo_movie.width, 0, 0, 0, -1, -100);
	g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, false, 0, 0, 0, 0, 0, 0, 0, aoslo_movie.width, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	// stop sampling
	g_objVirtex5BMD.AppStopADC(g_hDevVirtex5);

	g_objVirtex5BMD.DIAG_DMAClose(g_hDevVirtex5, &g_dma);


	g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, SEND_MOVIE_DONE);
	g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, SEND_MOVIE_DONE);
	g_viewDewVideo->SendMessage(WM_MOVIE_SEND, 0, SEND_MOVIE_DONE);
	g_viewDltVideo->SendMessage(WM_MOVIE_SEND, 0, SEND_MOVIE_DONE);

	Sleep(100);

	if (aoslo_movie.video_saveA1 != NULL) {
		delete [] aoslo_movie.video_saveA1;
		aoslo_movie.video_saveA1 = NULL;
	}
	if (aoslo_movie.video_saveA2 != NULL) {
		delete [] aoslo_movie.video_saveA2;
		aoslo_movie.video_saveA2 = NULL;
	}
	if (aoslo_movie.video_saveA3 != NULL) {
		delete [] aoslo_movie.video_saveA3;
		aoslo_movie.video_saveA3 = NULL;
	}
	if (aoslo_movie.video_saveB1 != NULL) {
		delete [] aoslo_movie.video_saveB1;
		aoslo_movie.video_saveB1 = NULL;
	}
	if (aoslo_movie.video_saveB2 != NULL) {
		delete [] aoslo_movie.video_saveB2;
		aoslo_movie.video_saveB2 = NULL;
	}
	if (aoslo_movie.video_saveB3 != NULL) {
		delete [] aoslo_movie.video_saveB3;
		aoslo_movie.video_saveB3 = NULL;
	}

//	AfxMessageBox("Stop2");
	Sleep(100);

	// free all memory
	delete [] g_PatchParamsB.shift_xi;
	delete [] g_PatchParamsB.shift_yi;
	delete [] g_PatchParamsB.shift_xy;

	delete [] aoslo_movie.video_out;
	delete [] aoslo_movie.video_in;
	delete [] aoslo_movie.video_ins;
	delete [] aoslo_movie.video_ref_bk;
	delete [] aoslo_movie.video_sparse;
	delete [] aoslo_movie.video_ref_g;
	delete [] aoslo_movie.video_ref_l;

	delete [] g_PatchParamsA.img_slice;
	delete [] g_PatchParamsA.img_sliceR;
	delete [] g_PatchParamsB.img_sliceR;


//	AfxMessageBox("Stop3");
	Sleep(100);

	if (g_ICANDIParams.Desinusoid == 1) {
		delete [] g_PatchParamsA.UnMatrixIdx;
		delete [] g_PatchParamsA.UnMatrixVal;
		delete [] g_PatchParamsA.StretchFactor;
		g_PatchParamsA.UnMatrixIdx = NULL;
		g_PatchParamsA.UnMatrixVal = NULL;
		g_PatchParamsA.StretchFactor = NULL;
	}

	return 0;
}

DWORD WINAPI CICANDIDoc::ThreadStablizationFFT(LPVOID pParam)
{
	int      i, retCode, patchIdx, sliceHeight, sx, sy;
	int     *deltaX, *deltaY, dltX, dltY, mirrorX, mirrorY, *MarkPatch;
	BOOL     bDrawDewarp;	// this flag marks all target patches in the mapping range of the target frames.
	int      cxOld, cyOld, cx, cy, blockID;
	BOOL     wideOpen, bRetCode, bStimulus;
	static int mirrorXold, mirrorYold, mirrorXoldsent, mirrorYoldsent, mirrorXcur, mirrorYcur;
	float	deltaXprime, deltaYprime;

//	shift_xi  = new float[2048];
//	shift_yi  = new float[2048];
//	shift_xy  = new float[255];
	deltaX    = new int [255];
	deltaY    = new int [255];
	MarkPatch = new int [255];

	cxOld = cyOld = 0;
	mirrorX = mirrorY = 0;
	mirrorXold = mirrorYold = mirrorYoldsent = mirrorXoldsent = mirrorXcur = mirrorYcur = 0;
	wideOpen = FALSE;

	FILE *fp;
	fp = fopen("block_motion.txt","w+");
	int cons_Yzeros_cnt, cons_Xzeros_cnt, mirrorX_skip_cnt, mirrorY_skip_cnt;
	cons_Yzeros_cnt = cons_Xzeros_cnt = mirrorX_skip_cnt = mirrorY_skip_cnt = 0;

	do {
		// sample a full frame of image to Matrox buffer which is mapped to a PCI buffer
		::WaitForSingleObject(g_EventEOB, INFINITE);

		BOOL bGlobalRef, bEOF, bRetImg = FALSE;

		blockID = g_BlockID;

		// with FFT stabilization, special fitering is needed
		if (g_frameIndex <= 1) bGlobalRef = TRUE;
		else bGlobalRef = FALSE;
		if (blockID == g_BlockCounts) bEOF = TRUE;
		else bEOF = FALSE;

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//             aoslo_movie.video_in should be truncated to 512x512 for FFT
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		if (g_bFFTIsRunning)
			g_objStabFFT.FastConvCUDA(aoslo_movie.video_sparse, aoslo_movie.video_in, aoslo_movie.width, 
				aoslo_movie.height, g_ICANDIParams.Filter, bGlobalRef, bEOF, blockID-1, BLOCK_COUNT, LINE_INTERVAL, bRetImg);

			/* ===============================================
			The codes below detect saccade/blink.
		    Here we do saccade/blink first, since there is 
		    no point to do stabilization if the current frame 
			is a saccade/blink frame
		   =============================================== */
		sliceHeight = blockID*LINE_INTERVAL;
		if (g_frameIndex < 1) {
		} else if (g_frameIndex == 1) {
			if (sliceHeight == g_ICANDIParams.F_YDIM*3/4) {
/*
				if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
					g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
					fprintf(g_fpTimeLog, "f_cnt:%d, stabilization initialization,      %d:%d:%d:%5.3f\n", 
						g_frameIndex, hours, minutes, seconds, milliseconds);
				}
*/
				retCode = g_objStabFFT.SaccadeDetectionK(g_frameIndex, blockID, &sx, &sy);
/*
				if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
					g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
					fprintf(g_fpTimeLog, "f_cnt:%d, stabilization initialization ends,      %d:%d:%d:%5.3f\n", 
						g_frameIndex, hours, minutes, seconds, milliseconds);
				}
*/
				g_bSaccadeFlag = FALSE;
			}
		} else {
			if (sliceHeight%FFT_HEIGHT064 == 0 && 
				sliceHeight/FFT_HEIGHT064 <= 4 && sliceHeight/FFT_HEIGHT064 >= 1) {
				patchIdx = sliceHeight/FFT_HEIGHT064 - 1;
/*
				if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
					g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
					fprintf(g_fpTimeLog, "f_cnt:%d, Saccade detection start,      %d:%d:%d:%5.3f\n", 
						g_frameIndex, hours, minutes, seconds, milliseconds);
				}
*/
				retCode = g_objStabFFT.SaccadeDetectionK(g_frameIndex, patchIdx, &sx, &sy);

				if (retCode != STAB_SUCCESS) 
					g_bSaccadeFlag = TRUE;
				else
					g_bSaccadeFlag = FALSE;
/*
				if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
					g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
					fprintf(g_fpTimeLog, "f_cnt:%d, Saccade detection end,      %d:%d:%d:%5.3f, %s (%d,%d)\n", 
						g_frameIndex, hours, minutes, seconds, milliseconds, 
						g_bSaccadeFlag?_T("--saccade--"):_T("OK"), sx, sy);
				}
*/
			}
		}
			

		/* ===============================================
			The codes below calculate motion of frame center
			This part is calculated at the end of each frame
		   =============================================== */
		if (blockID == g_BlockCounts && g_frameIndex > 1) {
/*
			if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
				g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
				fprintf(g_fpTimeLog, "f_cnt:%d, Central motion start,      %d:%d:%d:%5.3f\n", 
					g_frameIndex, hours, minutes, seconds, milliseconds);
			}
*/
			bDrawDewarp = FALSE;

			// calculate central motion only when the target frame is not a saccade/blink frame
			if (!g_bSaccadeFlag) {
				// previous frame is a saccade/blink frame, wide open the patch size
				if (g_bSaccadeFlagPrev) {
					cxOld = 0;
					cyOld = 0;
					wideOpen = TRUE;
				// previous frame is a no saccade/blink frame, use half patch size
				} else {
					// use cx_old, cy_old as the shift, and calculate motion of frame center
					wideOpen = FALSE;
				}
				bRetCode = g_objStabFFT.CalcCentralMotion(cxOld, cyOld, wideOpen, &cx, &cy);
				if (bRetCode) {
					/*
					if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
						g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
						fprintf(g_fpTimeLog, "f_cnt:%d, Central motion end,      %d:%d:%d:%5.3f, (%d,%d)-(%d,%d), %s\n", 
							g_frameIndex, hours, minutes, seconds, milliseconds, 
							cxOld, cyOld, cx, cy, wideOpen?_T("relock"):_T("drift"));
					}
*/
					// save new data for stabilization
					cxOld = cx;
					cyOld = cy;
					g_bSaccadeFlagPrev = g_bSaccadeFlag;

					bDrawDewarp = TRUE;
				} else {
//					if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
//						fprintf(g_fpTimeLog, "f_cnt:%d, Central motion calculated incorrectly?????????????\n", g_frameIndex);
//					}

					cxOld = 0;
					cyOld = 0;
					g_bSaccadeFlagPrev = TRUE;
				}
			} else {
				cxOld = 0;
				cyOld = 0;
//				if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
//					fprintf(g_fpTimeLog, "f_cnt:%d, Central motion skipped---------------\n", g_frameIndex);
//				}
				g_bSaccadeFlagPrev = g_bSaccadeFlag;
			}

			MarkPatch[3] = -1;
			g_UpdateMotions(bDrawDewarp, cx, cy, MarkPatch, deltaX, deltaY);

			// update stabilized image
//			g_nViewID = 2;
//			SetEvent(g_EventSendViewMsg);
			g_viewDewVideo->SendMessage(WM_MOVIE_SEND, 0, g_frameIndex);

			// initialize patch motion at the beginning of each frame
			for (i = 0; i < 255; i ++) MarkPatch[i] = -1;
			if (g_bSaccadeFlagPrev || g_bSaccadeFlag) {
				for (i = 0; i < 255; i ++) deltaX[i] = deltaY[i] =0;
			}
		}


		/* ===============================================
			The codes below calculate patch motions.
			Number of patches is synchronized by the interrupt
			counts (per frame) from the hardware. 
		   =============================================== */
		Out32(0x378, 0);
		if (g_frameIndex > 1 && blockID < g_BlockCounts) {
			// patch stabilization is run only when
			// both current frame and previous frame are not saccade frames.
			if (!g_bSaccadeFlagPrev && !g_bSaccadeFlag) {
				// start calculating patch motion
				retCode = g_objStabFFT.GetPatchXY(blockID, cx, cy, &dltX, &dltY);

				// the approach here will synchronize the steering mirror at the 
				// frequency of interrupt generator from the image grabber.
				// The total latency on the steeting mirror will be
				//		sampling latency (T1)                   : ~1.0 msec
				//		GPU computational latency (T2)          : ~0.2 msec
				//		PCIe buffering, ADC/DAC conversion (T3) : 0.1~0.2 msec
				// so the total latency shall be about 1.3-1.4 msec.
				// the steering mirror can be driven at 700Hz~800Hz dependent on GPU power
				if (retCode == STAB_SUCCESS) {
					mirrorX = deltaX[blockID-1] = dltX;
					mirrorY = deltaY[blockID-1] = dltY;
					MarkPatch[blockID-1] = retCode;
					bStimulus = TRUE;
					Out32(0x378, 0);
					g_bValidSignal[blockID-1] = TRUE;
				// target patch is out the top range of reference frame
				} else if (retCode == STAB_OUT_OF_TOP) {
					// get patch motion from the last patch of previous frame
					mirrorX = deltaX[blockID-1] = -deltaX[g_BlockCounts-2];
					mirrorY = deltaY[blockID-1] = -deltaY[g_BlockCounts-2];
					bStimulus = TRUE;
					Out32(0x378, 2);
					g_bValidSignal[blockID-1] = FALSE;
				// target patch is out the bottom range of reference frame
				} else if (retCode == STAB_OUT_OF_BOTTOM) {
					// get patch motion from the previous patch on the same frame
					//added on 06042015
					if (blockID>1) {
					mirrorX = deltaX[blockID-1] = deltaX[blockID-2];//mirrorXold;
					mirrorY = deltaY[blockID-1] = deltaY[blockID-2];//mirrorYold;
					}
					else {
						mirrorX = deltaX[blockID-1] = mirrorXoldsent;
						mirrorY = deltaY[blockID-1] = mirrorYoldsent;
					}
				//	mirrorX = deltaX[blockID-1] = deltaX[blockID-2];
				//	mirrorY = deltaY[blockID-1] = deltaY[blockID-2];
					bStimulus = TRUE;
					Out32(0x378, 2);
					g_bValidSignal[blockID-1] = FALSE;
				// low correlation coefficient factor between reference patch and target patch
				} else if (retCode == STAB_LOW_COEFF) {
					if (blockID>1) {
					mirrorX = deltaX[blockID-1] = deltaX[blockID-2];//mirrorXold;
					mirrorY = deltaY[blockID-1] = deltaY[blockID-2];//mirrorYold;
					}
					else {
						mirrorX = deltaX[blockID-1] = mirrorXoldsent;
						mirrorY = deltaY[blockID-1] = mirrorYoldsent;
					}
					bStimulus = FALSE;
					Out32(0x378, 2);
					g_bValidSignal[blockID-1] = FALSE;
				} else {
					// with suspicious patch motion, the steering mirror is reset to 0
//					deltaX[blockID-1] = 0;
//					deltaY[blockID-1] = 0;
				//	mirrorX = deltaX[blockID-1] = mirrorXold;
				//	mirrorY = deltaY[blockID-1] = mirrorYold;
					//added 06042015
					if (blockID>1) {
					mirrorX = deltaX[blockID-1] = deltaX[blockID-2];//mirrorXold;
					mirrorY = deltaY[blockID-1] = deltaY[blockID-2];//mirrorYold;
					}
					else {
						mirrorX = deltaX[blockID-1] = mirrorXoldsent;
						mirrorY = deltaY[blockID-1] = mirrorYoldsent;
					}
					bStimulus = FALSE;
					Out32(0x378, 0);
					g_bValidSignal[blockID-1] = TRUE;
				}

//				mirrorX = deltaX[blockID-1];
//				mirrorY = deltaY[blockID-1];

			} else {
				deltaX[blockID-1] = 0;
				deltaY[blockID-1] = 0;
				// with saccade/blink flag on, the steering mirror is reset to 0
			//commented on 06042015
			//	mirrorX = 0; 
			//	mirrorY = 0;
			//added on 06042015
				mirrorX = mirrorXoldsent;
				mirrorY = mirrorYoldsent;
				retCode = -99;
				bStimulus = FALSE;
				Out32(0x378, 2);
				g_bValidSignal[blockID-1] = FALSE;
			}
/*
			if (g_bFFTIsRunning == TRUE && g_bTimingTest) {
				g_GetAppSystemTime(&hours, &minutes, &seconds, &milliseconds);
				fprintf(g_fpTimeLog, "         --------%d,      %d:%d:%d:%5.3f  retCode(%d) motion:(%d,%d), mirror:(%d,%d)\n", 
					blockID, hours, minutes, seconds, milliseconds, retCode,
					deltaX[blockID-1], deltaY[blockID-1], mirrorX, mirrorY);
			}
*/		
		} else {
			bStimulus = FALSE;
			mirrorX = mirrorXoldsent;
			mirrorY = mirrorYoldsent;
			Out32(0x378, 2);
			g_bValidSignal[blockID-1] = FALSE;
		}

/*		if (fp!= NULL)
			fprintf(fp, "%d\t%d\t%d\t%d\t%d\t%d\t%d\n",(int)retCode,dltX, deltaX[blockID-1], mirrorX, mirrorXcur, dltY, deltaY[blockID-1], mirrorY, mirrorYcur);
*/
		
		// update mirror motion by programming the DAC on FPGA board
		// 
		// ---------------    add a low pass filter to remove jitter on the steering mirror
		//
		//consecutive zeros case
/*		if (mirrorY == mirrorYold && !mirrorYold && cons_Yzeros_cnt<4) {
			cons_Yzeros_cnt++;
		}
		if (!mirrorYold && abs(mirrorY) && cons_Yzeros_cnt)
		{
			if (abs(mirrorYoldsent - mirrorY) < MOTION_TS_SY)
				mirrorYold = mirrorY;
			cons_Yzeros_cnt = 0;
		}
		if (mirrorX == mirrorXold && !mirrorXold && cons_Xzeros_cnt<4) {
			cons_Xzeros_cnt++;
		}
		if (!mirrorXold && abs(mirrorX) && cons_Xzeros_cnt)
		{
			if (abs(mirrorXoldsent - mirrorX) < MOTION_TS_SX)
				mirrorXold = mirrorX;
			cons_Xzeros_cnt = 0;
		}		
		if (abs(mirrorXold-mirrorX) > MOTION_TS_SX || abs(mirrorYold-mirrorY) > MOTION_TS_SY) {// || abs(mirrorX) <= 1 || abs(mirrorY) <=1) {
		} else {
			if (g_ICANDIParams.Desinusoid && retCode == STAB_SUCCESS && (!cons_Yzeros_cnt || !cons_Xzeros_cnt)) {
							
				if (!cons_Xzeros_cnt && !cons_Yzeros_cnt) {
					if (mirrorXoldsent && (abs(mirrorXoldsent - mirrorX) > MOTION_TS_SX+4) && mirrorX_skip_cnt < 3) {
						mirrorXcur = mirrorXoldsent;
						++mirrorX_skip_cnt;			
					} else {
						mirrorXcur = mirrorX; 
						mirrorX_skip_cnt=0;
					}
					if (mirrorYoldsent && (abs(mirrorYoldsent - mirrorY) > MOTION_TS_SY+4) && mirrorY_skip_cnt < 3) {
						mirrorYcur = mirrorYoldsent;
						++mirrorY_skip_cnt;
					} else {
						mirrorYcur = mirrorY; 
						mirrorY_skip_cnt=0;
					}									
				} else if (!cons_Xzeros_cnt && cons_Yzeros_cnt) {
					if (mirrorXoldsent && (abs(mirrorXoldsent - mirrorX) > MOTION_TS_SX+4) && mirrorX_skip_cnt < 3) {
						mirrorXcur = mirrorXoldsent;
						++mirrorX_skip_cnt;
					} else {
						mirrorXcur = mirrorX;
						mirrorX_skip_cnt = 0;
					}
					mirrorYcur = mirrorYoldsent;					
				} else if (cons_Xzeros_cnt && !cons_Yzeros_cnt) {					
					mirrorXcur = mirrorXoldsent;
					if (mirrorYoldsent && (abs(mirrorYoldsent - mirrorY) > MOTION_TS_SY+4) && mirrorY_skip_cnt < 3) {
						mirrorYcur = mirrorYoldsent;
						++mirrorY_skip_cnt;
					} else {
						mirrorYcur = mirrorY;
						mirrorY_skip_cnt = 0;
					}
				} else{					
					mirrorXcur = mirrorXoldsent;
					mirrorYcur = mirrorYoldsent;
				}

				mirrorYoldsent = mirrorYcur;
				mirrorXoldsent = mirrorXcur;							
				
				deltaXprime = ((float)mirrorXcur)*cos(3.414*g_ICANDIParams.MotionAngleX/180.)+ ((float)mirrorYcur)*sin(3.414*g_ICANDIParams.MotionAngleY/180.);
				deltaYprime = -((float)mirrorXcur)*sin(3.414*g_ICANDIParams.MotionAngleX/180.)+ ((float)mirrorYcur)*cos(3.414*g_ICANDIParams.MotionAngleY/180.);

//				deltaXprime = ((float)mirrorX)*cos(3.414*g_ICANDIParams.MotionAngleX/180.)+ ((float)mirrorY)*sin(3.414*g_ICANDIParams.MotionAngleY/180.);
//				deltaYprime = -((float)mirrorX)*sin(3.414*g_ICANDIParams.MotionAngleX/180.)+ ((float)mirrorY)*cos(3.414*g_ICANDIParams.MotionAngleY/180.);

				if (g_InvertMotionTraces) {
					g_objVirtex5BMD.AppMotionTraces(g_hDevVirtex5, -deltaXprime, -deltaYprime, g_Motion_ScalerX, g_Motion_ScalerY, fp);
				} else {
					g_objVirtex5BMD.AppMotionTraces(g_hDevVirtex5, deltaXprime, deltaYprime, g_Motion_ScalerX, g_Motion_ScalerY, fp); 				
				}
			}
		}*/

//		g_objVirtex5BMD.AppMotionTraces(g_hDevVirtex5, -mirrorX, -mirrorY, motion_scaler); 

		// stimulus is delivered here
		g_StimulusDeliveryFFT(-mirrorX, -mirrorY, bStimulus, blockID);
		mirrorXold = mirrorX;
		mirrorYold = mirrorY;
		
	} while (((CICANDIApp*)AfxGetApp())->m_isGrabStarted);

	fclose(fp);

	//delete  shift_xi;
	//delete  shift_yi;
	//delete  shift_xy;
	delete [] deltaX;
	delete [] deltaY;
	delete [] MarkPatch;

	return 0;
}



DWORD WINAPI CICANDIDoc::ThreadSaveVideoHDD(LPVOID pParam)
{
	CByteArray  m_array;
	CEvent      WaitEvents; // creates time delay events
	CICANDIDoc *pDoc = (CICANDIDoc *)pParam;
	int         i, j, k, idx1, idx2, offset, movie_len;
	BYTE       *movieA, *movieB;
	BOOL        bStabilizedOn;	
	Mat			frameA;
	Mat			frameB;
	movieA = new BYTE[aoslo_movie.height * aoslo_movie.width];
	movieB = new BYTE[aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy];
	frameA = Mat(aoslo_movie.height, aoslo_movie.width, CV_8UC1, movieA, 0);
	frameB = Mat(aoslo_movie.dewarp_sy, aoslo_movie.dewarp_sx, CV_8UC1, movieB, 0);

	do {
		::WaitForSingleObject(WaitEvents, 1);

		// process a full frame of image at the end of a V-sync
		if (pDoc->m_bDumpingVideo == TRUE) {			
			pDoc->m_bDumpingVideo = FALSE;

			bStabilizedOn = FALSE;

			// dump raw video to local harddrive
			if (pDoc->m_bValidAviHandleA == TRUE && 
				aoslo_movie.video_saveA1 != NULL &&
				aoslo_movie.video_saveA2 != NULL &&
				aoslo_movie.video_saveA3 != NULL) {

				if (pDoc->m_bValidAviHandleB == TRUE) bStabilizedOn = TRUE;

				movie_len = MEMORY_POOL_LENGTH;
				if (aoslo_movie.memory_pool_ID == 0) {
					for (k = 0; k < movie_len; k ++) {
						offset = k * aoslo_movie.height * aoslo_movie.width;
						memcpy(movieA, &aoslo_movie.video_saveA1[offset], aoslo_movie.height * aoslo_movie.width);					
						//pDoc->m_aviFileA.write(frameA);
						// image is always flipped around x-axis in windows
						pDoc->m_aviFileA.writeFrame(frameA, true);
					}
				} else if (aoslo_movie.memory_pool_ID == 1) {
					for (k = 0; k < movie_len; k ++) {
						offset = k * aoslo_movie.height * aoslo_movie.width;
						memcpy(movieA, &aoslo_movie.video_saveA2[offset], aoslo_movie.height * aoslo_movie.width);
						//pDoc->m_aviFileA.write(frameA);
						// image is always flipped around x-axis in windows
						pDoc->m_aviFileA.writeFrame(frameA, true);
					}
				} else if (aoslo_movie.memory_pool_ID == 2) {
					for (k = 0; k < movie_len; k ++) {
						offset = k * aoslo_movie.height * aoslo_movie.width;
						memcpy(movieA, &aoslo_movie.video_saveA3[offset], aoslo_movie.height * aoslo_movie.width);
						//pDoc->m_aviFileA.write(frameA);
						// image is always flipped around x-axis in windows
						pDoc->m_aviFileA.writeFrame(frameA, true);
					}
				}				
			}

			if (pDoc->m_bValidAviHandleA == FALSE) {
				if (aoslo_movie.avi_handle_on_A == TRUE) {
					//pDoc->m_aviFileA.release();
					pDoc->m_aviFileA.close();
					aoslo_movie.avi_handle_on_A = FALSE;
				}
			}

			// dump stabilized video to local harddrive
			if ((pDoc->m_bValidAviHandleB == TRUE || bStabilizedOn == TRUE) && 
				aoslo_movie.video_saveB1 != NULL &&
				aoslo_movie.video_saveB2 != NULL &&
				aoslo_movie.video_saveB3 != NULL) {				
				movie_len = MEMORY_POOL_LENGTH;
				if (aoslo_movie.memory_pool_ID == 0) {
					for (k = 0; k < movie_len; k ++) {
						offset = k * aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy;
						memcpy(movieB, &aoslo_movie.video_saveB1[offset], aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy);						
						//pDoc->m_aviFileB.write(frameB);
						// image is always flipped around x-axis in windows
						pDoc->m_aviFileB.writeFrame(frameB, true);
					}
				} else if (aoslo_movie.memory_pool_ID == 1) {
					for (k = 0; k < movie_len; k ++) {
						offset = k * aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy;
						memcpy(movieB, &aoslo_movie.video_saveB2[offset], aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy);						
						//pDoc->m_aviFileB.write(frameB);
						// image is always flipped around x-axis in windows
						pDoc->m_aviFileB.writeFrame(frameB, true);
					}
				} else if (aoslo_movie.memory_pool_ID == 2) {
					for (k = 0; k < movie_len; k ++) {
						offset = k * aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy;
						memcpy(movieB, &aoslo_movie.video_saveB3[offset], aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy);						
						//pDoc->m_aviFileB.write(frameB);
						// image is always flipped around x-axis in windows
						pDoc->m_aviFileB.writeFrame(frameB, true);
					}
				}
			}

			if (pDoc->m_bValidAviHandleB == FALSE) {
				if (aoslo_movie.avi_handle_on_B == TRUE) {
					//pDoc->m_aviFileB.release();
					pDoc->m_aviFileB.close();
					aoslo_movie.avi_handle_on_B = FALSE;
				}
			}
			if (pDoc->m_iSavedFramesA >= (pDoc->m_nMovieLength<<1)) {				
				pDoc->m_iSavedFramesA    = 0;
				g_bRecord = FALSE;
				if (g_bMatlabCtrl && g_bMatlabVideo && g_bMatlab_Update && g_bMatlabAVIsavevideo) g_bMatlabAVIsavevideo = FALSE, g_bMatlab_Update=FALSE,g_bStimulusOn = FALSE;
				g_viewMsgVideo->PostMessage(WM_MOVIE_SEND, 0, SAVE_VIDEO_FLAG);
				::WaitForSingleObject(WaitEvents, 350);
				PlaySound(MAKEINTRESOURCE(IDW_SOUND),AfxGetInstanceHandle(),SND_RESOURCE | SND_ASYNC);

				if (g_bSyncOCTReady) g_bSyncOCTReady = FALSE;
			}
		}
	} while (((CICANDIApp*)AfxGetApp())->m_isGrabStarted);	
	delete [] movieA;		
	delete [] movieB;
	
	return 0;
}


DWORD WINAPI CICANDIDoc::ThreadLoadData2FPGA(LPVOID pParam)
{
	HANDLE fpgaHandles[2];
	unsigned short  *lut_loc_buf1, *lut_loc_buf2, x0;
	int    i, x1, x2, deltax;
	float  cent;
	
	fpgaHandles[0] = g_EventLoadStim;
	fpgaHandles[1] = g_EventLoadLUT;

	do {
		// waiting for event 
		switch (::MsgWaitForMultipleObjects(2, fpgaHandles, FALSE, INFINITE, QS_ALLEVENTS)) {
		case WAIT_OBJECT_0:
			// load IR stimulus pattern to FPGA
			g_objVirtex5BMD.AppLoadStimulus8bits(g_hDevVirtex5, aoslo_movie.stim_ir_buffer, aoslo_movie.stim_ir_nx, aoslo_movie.stim_ir_ny);
			// load green stimulus pattern to FPGA
			g_objVirtex5BMD.AppLoadStimulus14bitsGreen(g_hDevVirtex5, aoslo_movie.stim_gr_buffer, aoslo_movie.stim_gr_nx, aoslo_movie.stim_gr_ny);
			// load Red stimulus pattern to FPGA
			g_objVirtex5BMD.AppLoadStimulus14bitsRed(g_hDevVirtex5, aoslo_movie.stim_rd_buffer, aoslo_movie.stim_rd_nx, aoslo_movie.stim_rd_ny);
			break;
		case WAIT_OBJECT_0+1:
			lut_loc_buf1 = new unsigned short [g_iStimulusSizeX_GR+2];
			lut_loc_buf2 = new unsigned short [g_iStimulusSizeX_GR+2];
			deltax = g_iStimulusSizeX_GR;

			// load green lookup table for presinusoiding stimulus pattern in x direction
			// with desinusoiding
			if (g_ICANDIParams.Desinusoid == 1) {
				if (aoslo_movie.x_center_gr > 0 && aoslo_movie.x_cc_gr > 0) {

					x1 = aoslo_movie.x_center_gr - aoslo_movie.x_left_gr;
					x2 = aoslo_movie.x_center_gr + aoslo_movie.x_right_gr;
					deltax = x2 - x1 + 1;

					if (g_bConstPwr)
						x0 = linStretch[x1];													// constant power (N.D.20191010)
					else
						x0 = (int)g_PatchParamsA.StretchFactor[x1];								// constant size (N.D.20191010)

					for (i = x1; i <= x2; i ++) {

						if (g_bConstPwr){				// constant power (N.D.20191010)
							cent = linStretch[i]-x0;
							lut_loc_buf2[i-x1] = (unsigned short)floor((cent));
						}
						else{							// constant size (N.D.20191010)
							cent = g_PatchParamsA.StretchFactor[i]-x0;
							lut_loc_buf2[i-x1] = (unsigned short)ceil(cent);
						}

						lut_loc_buf1[i-x1] = (unsigned short)floor(cent);
					}

				} else {
					for (i = 0; i < g_iStimulusSizeX_GR; i ++) {
						lut_loc_buf2[i] = lut_loc_buf1[i] = i;
					}
				}

			} else {
				for (i = 0; i < g_iStimulusSizeX_GR; i ++) {
					lut_loc_buf2[i] = lut_loc_buf1[i] = i;
				}
			}

			// upload warp LUT for Red stimulus pattern
			g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, aoslo_movie.stim_gr_nx, deltax, lut_loc_buf1, 3);
			g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, aoslo_movie.stim_gr_nx, deltax, lut_loc_buf2, 6);

			// write ir pixel weights 
			g_objVirtex5BMD.AppWriteWarpWeights(g_hDevVirtex5, deltax, aoslo_movie.weightsGreen, 1);

			delete [] lut_loc_buf1;
			delete [] lut_loc_buf2;

			// load red lookup table for warping stimulus pattern
			lut_loc_buf1 = new unsigned short [g_iStimulusSizeX_RD+2];
			lut_loc_buf2 = new unsigned short [g_iStimulusSizeX_RD+2];
			deltax = g_iStimulusSizeX_RD;

			// with desinusoiding
			if (g_ICANDIParams.Desinusoid == 1) {
				if (aoslo_movie.x_center_rd > 0 && aoslo_movie.x_cc_rd > 0) {

					x1 = aoslo_movie.x_center_rd - aoslo_movie.x_left_rd;
					x2 = aoslo_movie.x_center_rd + aoslo_movie.x_right_rd;
					deltax = x2 - x1 + 1;
					
					if (g_bConstPwr)
						x0 = linStretch[x1];													// constant power (N.D.20191010)
					else
						x0 = (int)g_PatchParamsA.StretchFactor[x1];								// constant size (N.D.20191010)
					
					for (i = x1; i <= x2; i ++) {
						
						if (g_bConstPwr){				// constant power (N.D.20191010)
							cent = linStretch[i]-x0;
							lut_loc_buf2[i-x1] = (unsigned short)floor((cent));
						}

						else{							// constant size (N.D.20191010)
							cent = g_PatchParamsA.StretchFactor[i]-x0;
							lut_loc_buf2[i-x1] = (unsigned short)ceil(cent);
						}

						lut_loc_buf1[i-x1] = (unsigned short)floor(cent);
					}

				} else {
					for (i = 0; i < g_iStimulusSizeX_RD; i ++) {
						lut_loc_buf2[i] = lut_loc_buf1[i] = i;
					}
				}

			} else {
				for (i = 0; i < g_iStimulusSizeX_RD; i ++) {
					lut_loc_buf2[i] = lut_loc_buf1[i] = i;
				}
			}

			// upload warp LUT for red stimulus pattern
			g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, aoslo_movie.stim_rd_nx, deltax, lut_loc_buf1, 2);
			g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, aoslo_movie.stim_rd_nx, deltax, lut_loc_buf2, 5);

			// write red pixel weights 
			g_objVirtex5BMD.AppWriteWarpWeights(g_hDevVirtex5, deltax, aoslo_movie.weightsRed, 0);

			delete [] lut_loc_buf1;
			delete [] lut_loc_buf2;


			// load IR lookup table for warping stimulus pattern
			lut_loc_buf1 = new unsigned short [g_iStimulusSizeX_IR+2];
			deltax = g_iStimulusSizeX_IR;

			// with desinusoiding
			if (g_ICANDIParams.Desinusoid == 1) {
				if (aoslo_movie.x_center_ir > 0 && aoslo_movie.x_cc_ir > 0) {

					x1 = aoslo_movie.x_center_ir - aoslo_movie.x_left_ir;
					x2 = aoslo_movie.x_center_ir + aoslo_movie.x_right_ir;
					deltax = x2 - x1 + 1;

					/*if (g_bConstPwr)	
						x0 = linStretch[x1];									// constant power (N.D.20191010) (always const. size for IR, ND 20191119)
					else*/
						x0 = (int)g_PatchParamsA.StretchFactor[x1];
					
					for (i = x1; i <= x2; i ++) {
						/*if (g_bConstPwr)
							cent = linStretch[i]-x0;							// constant power (N.D.20191010) (always const. size for IR, ND 20191119)
						else*/
							cent = g_PatchParamsA.StretchFactor[i]-x0;			// constant size(N.D.20191010)

						lut_loc_buf1[i-x1] = (unsigned short)floor(cent);
					}
				} else {
					for (i = 0; i < g_iStimulusSizeX_IR; i ++) {
						lut_loc_buf1[i] = i;
					}
				}

			} else {
				for (i = 0; i < g_iStimulusSizeX_IR; i ++) {
					lut_loc_buf1[i] = i;
				}
			}

			// upload warp LUT for IR stimulus pattern
			g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, aoslo_movie.stim_ir_nx, deltax, lut_loc_buf1, 7);

			delete [] lut_loc_buf1;

		/*	if (aoslo_movie.stimulus_audio_flag)
			{
			//	PlaySound(MAKEINTRESOURCE(IDW_DING),AfxGetInstanceHandle(),SND_RESOURCE | SND_ASYNC);
			//	aoslo_movie.stimulus_audio_flag = FALSE;
				SetEvent(g_EventStimPresentFlag);
			}*/
			if (g_bRecord);
			//	Out32(57424,3);
  /*
			for (i = 0; i < g_iStimulusSizeX_RD; i ++) {
				fprintf(g_fp, "(%d,%d)", lut_loc_buf1[i], lut_loc_buf2[i]);
			}
			fprintf(g_fp, " %d-%d-%d\n", x0, x1, x2);
*/

//			fprintf(g_fp, "%d: rdnx-%d, irnx-%d\n", g_frameIndex, g_iStimulusSizeX_RD, g_iStimulusSizeX_GR);

			break;

		default:
			break;
		}
	} while (true);

	CloseHandle(g_EventLoadStim);
	CloseHandle(g_EventLoadLUT);

	return 0;
}

DWORD WINAPI CICANDIDoc::ThreadPlayStimPresentFlag(LPVOID pParam)
{
	do {
		// sample a full frame of image to Matrox buffer which is mapped to a PCI buffer
		::WaitForSingleObject(g_EventStimPresentFlag, INFINITE);
		if (aoslo_movie.stimulus_audio_flag && g_bFFTIsRunning)
		{
			Sleep(100);			
			PlaySound(MAKEINTRESOURCE(IDW_DING),AfxGetInstanceHandle(),SND_RESOURCE | SND_ASYNC);
			aoslo_movie.stimulus_audio_flag = FALSE;
			g_bIRstim = g_bRDstim = g_bGRstim = FALSE;
			//if (g_bRecord);
			//	Out32(57424,3);
		}
	}while(TRUE);

	CloseHandle(g_EventStimPresentFlag);
}

DWORD WINAPI CICANDIDoc::ThreadSendViewMsg(LPVOID pParam)
{
	CICANDIDoc *pDoc = (CICANDIDoc *)pParam;

	do {
		// sample a full frame of image to Matrox buffer which is mapped to a PCI buffer
		::WaitForSingleObject(g_EventSendViewMsg, INFINITE);
		if (g_nViewID == 1) {
//			fprintf(g_fp, "view:%d, %d/%d/%d\n", g_nViewID, aoslo_movie.memory_pool_ID, g_sampling_counter, pDoc->m_iSavedFramesA);

			g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, g_sampling_counter);
			g_bEOFHandler = FALSE;

			if (g_bHistgram == TRUE) 
				g_viewDltVideo->SendMessage(WM_MOVIE_SEND, 0, g_sampling_counter);
		} else if (g_nViewID == 2) {
//			fprintf(g_fp, "view:%d, %d/%d/%d\n", g_nViewID, aoslo_movie.memory_pool_ID, g_sampling_counter, pDoc->m_iSavedFramesA);

			//g_viewDewVideo->SendMessage(WM_MOVIE_SEND, 0, g_frameIndex);
		}
	} while (TRUE);
	
	CloseHandle(g_EventSendViewMsg);
}

DWORD WINAPI CICANDIDoc::ThreadReadStimVideo(LPVOID pParam)
{
	CICANDIDoc *parent = (CICANDIDoc *)pParam;
	int         i, j, k, idxs, ind, idxd1, idxd2, idxd3, frames, fWidth, fHeight, iFirstFrame, nPlanes, ret, fWidthOffs;
	BYTE       *imgTemp, *bytBuff;
	long		fBufSize;
	unsigned short datain;
	unsigned short *tempBuff; 
	CString     msg;
	PAVISTREAM  pStream;
	PGETFRAME   pFrame;
	BITMAPINFOHEADER bih;

	do {
		// sample a full frame of image to Matrox buffer which is mapped to a PCI buffer
		::WaitForSingleObject(g_EventReadStimVideo, INFINITE);
		//AfxMessageBox("Read stimulus video file <" + parent->m_strStimVideoName + ">");
		
		aoslo_movie.bWithStimVideo = FALSE;		// flag with stimulus video
		aoslo_movie.nStimFrameIdx = -1;
		
		for (int vid = 0; vid < aoslo_movie.nStimVideoNum; vid ++) {

			pStream = g_GetAviStream(parent->m_strStimVideoName[vid], &frames, &fWidth, &fHeight, &iFirstFrame, &nPlanes, &fBufSize);
			if (pStream == NULL) {
				AVIStreamRelease(pStream);
				AVIFileExit();
				AfxMessageBox("Error: can't open avi stream", MB_ICONWARNING);
			} else {
				if (fWidth > 256 || fHeight > 256 || fWidth <= 0 || fHeight <= 0) {
					AVIStreamRelease(pStream);
					AVIFileExit();
					AfxMessageBox("Invalid video size. It is required to be 0 <width<=257 and 0<height<=257.", MB_ICONWARNING);
				} else {
					fWidthOffs = fWidth%4;
					aoslo_movie.nStimFrameNum  = frames;
					aoslo_movie.nStimVideoLength[vid] = frames;

					aoslo_movie.nStimVideoNX[vid]   = fWidth;			// stimulus width
					aoslo_movie.nStimVideoNY[vid]   = (fHeight%2==0) ? fHeight : fHeight+1;			// stimulus Height
					aoslo_movie.nStimVideoPlanes[vid] = nPlanes;
					aoslo_movie.stim_video[vid] = new unsigned short [frames*aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]*nPlanes];
					ZeroMemory(aoslo_movie.stim_video[vid], sizeof(unsigned short)*frames*aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]*nPlanes);
					tempBuff = new unsigned short [aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]*nPlanes];
					ZeroMemory(tempBuff, sizeof(unsigned short)*aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]*nPlanes);

					pFrame = AVIStreamGetFrameOpen(pStream, NULL);

					bytBuff   = new BYTE[fBufSize];//fWidth*fHeight*nPlanes];

					// read video stream to video buffer
					for (i = 0; i < frames; i ++)
					{
						// the returned is a packed DIB frame
						imgTemp = (BYTE*) AVIStreamGetFrame(pFrame, i);

						RtlMoveMemory(&bih.biSize, imgTemp, sizeof(BITMAPINFOHEADER));

						//now get the bitmap bits
						if (bih.biSizeImage < 1) {
							msg.Format("Error: can't read frame No. %d/%d.", i+1, frames);
							AfxMessageBox(msg, MB_ICONWARNING);
						}
						// get rid of the header information and retrieve the real image 
						nPlanes == 1?
						RtlMoveMemory(bytBuff, imgTemp+sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256, bih.biSizeImage)
						:RtlMoveMemory(bytBuff, imgTemp+sizeof(BITMAPINFOHEADER), bih.biSizeImage);	
						ind = i*aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]*nPlanes;
						for (j = 0; j < fHeight/*aoslo_movie.nStimVideoNY[vid]*/; j ++) {							
						//	idxd1 = ind + (j * aoslo_movie.nStimVideoNX[vid]);
							idxd1 = j*aoslo_movie.nStimVideoNX[vid];
							idxs = (fHeight-1-j) * (fWidth * nPlanes + fWidthOffs);
							if (nPlanes > 1) {
							//	idxd2 = ind + (aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid] + j * aoslo_movie.nStimVideoNX[vid]);
								idxd2 = idxd1+(aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]);
							//	idxd3 = ind + ((aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid] << 1) + j * aoslo_movie.nStimVideoNX[vid]);
								idxd3 = idxd2+(aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]);
							}
							for (k = 0; k < fWidth/*aoslo_movie.nStimVideoNX[vid]*/; k ++) {
								if (nPlanes > 1) {
									datain = bytBuff[idxs+k*nPlanes+2]<<6;
								//	aoslo_movie.stim_video[vid][idxd1+k] = datain;
									tempBuff[idxd1+k] = datain;
									datain = bytBuff[idxs+k*nPlanes+1]<<6;
								//	aoslo_movie.stim_video[vid][idxd2+k] = datain;
									tempBuff[idxd2+k] = datain;
									datain = bytBuff[idxs+k*nPlanes]<<6;
								//	aoslo_movie.stim_video[vid][idxd3+k] = datain;
									tempBuff[idxd3+k] = datain;
								} 
								else {
									datain = bytBuff[idxs + k*nPlanes]<<6;
								//	aoslo_movie.stim_video[vid][idxd1+k] = datain;
									tempBuff[idxd1+k] = datain;
								}								
							}
						}

						if (nPlanes > 1) {
							if (fHeight%2 != 0)
								VUS_equC(&tempBuff[aoslo_movie.nStimVideoNX[vid]* (aoslo_movie.nStimVideoNY[vid]*2 + fHeight)], aoslo_movie.nStimVideoNX[vid], 255<<6); 
							idxd1 = ind+aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid];
							idxd2 = idxd1+aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid];
							switch (g_ICANDIParams.FRAME_ROTATION)
							{
							case 0:
								memcpy(aoslo_movie.stim_video[vid]+ind, tempBuff, sizeof(unsigned short)*aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]);
								memcpy(aoslo_movie.stim_video[vid]+idxd1, tempBuff+(aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]), sizeof(unsigned short)*aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]);
								memcpy(aoslo_movie.stim_video[vid]+idxd2, tempBuff+(aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]*2), sizeof(unsigned short)*aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]);
								break;
							case 1:
								MUSrotate270( (aoslo_movie.stim_video[vid]+ind), tempBuff, aoslo_movie.nStimVideoNX[vid], aoslo_movie.nStimVideoNY[vid]);
								MUSRows_rev((aoslo_movie.stim_video[vid]+ind), aoslo_movie.nStimVideoNX[vid], aoslo_movie.nStimVideoNY[vid]);
								MUSrotate270((aoslo_movie.stim_video[vid]+(idxd1)), (tempBuff+(fWidth*fHeight)), aoslo_movie.nStimVideoNX[vid], aoslo_movie.nStimVideoNY[vid]);
								MUSRows_rev((aoslo_movie.stim_video[vid]+(idxd1)), aoslo_movie.nStimVideoNX[vid], aoslo_movie.nStimVideoNY[vid]);
								MUSrotate270((aoslo_movie.stim_video[vid]+(idxd2)), (tempBuff+(fWidth*fHeight*2)), aoslo_movie.nStimVideoNX[vid], aoslo_movie.nStimVideoNY[vid]);
								MUSRows_rev((aoslo_movie.stim_video[vid]+(idxd2)), aoslo_movie.nStimVideoNX[vid], aoslo_movie.nStimVideoNY[vid]);
								break;
							case 2: //flip along Y axis
								MUSrotate180( (aoslo_movie.stim_video[vid]+ind), tempBuff, aoslo_movie.nStimVideoNY[vid], aoslo_movie.nStimVideoNX[vid]);
								MUSrotate180((aoslo_movie.stim_video[vid]+(idxd1)), (tempBuff+(fWidth*fHeight)), aoslo_movie.nStimVideoNY[vid], aoslo_movie.nStimVideoNX[vid]);
								MUSrotate180((aoslo_movie.stim_video[vid]+(idxd2)), (tempBuff+(fWidth*fHeight*2)), aoslo_movie.nStimVideoNY[vid], aoslo_movie.nStimVideoNX[vid]);
								break;




							}
						}
						else {
							switch(g_ICANDIParams.FRAME_ROTATION) {
								case 0:
									memcpy(aoslo_movie.stim_video[vid]+ind, tempBuff, sizeof(unsigned short)*aoslo_movie.nStimVideoNX[vid]*aoslo_movie.nStimVideoNY[vid]);
									break;
								case 1:
									MUSrotate270((aoslo_movie.stim_video[vid]+ind), tempBuff, aoslo_movie.nStimVideoNX[vid], aoslo_movie.nStimVideoNY[vid]);
									MUSRows_rev((aoslo_movie.stim_video[vid]+ind), aoslo_movie.nStimVideoNX[vid], aoslo_movie.nStimVideoNY[vid]);
									break;
								case 2:
									MUSrotate180((aoslo_movie.stim_video[vid]+ind), tempBuff, aoslo_movie.nStimVideoNY[vid], aoslo_movie.nStimVideoNX[vid]);
									break;
							}
						}
					}

					switch(g_ICANDIParams.FRAME_ROTATION) {
						case 0:							
							break;
						case 1:
							aoslo_movie.nStimVideoNX[vid]   = (fHeight%2==0) ? fHeight : fHeight+1;			// stimulus width
							aoslo_movie.nStimVideoNY[vid]   = fWidth;			// stimulus Height
							break;
						case 2:							
							break;
					}

					AVIStreamGetFrameClose(pFrame);    
					AVIStreamRelease(pStream);
					AVIFileExit();
				}
				
				delete [] bytBuff;
				delete [] tempBuff;
			}
		}
		if (!g_bMatlabCtrl) {
			msg.Format("All %d stimulus videos have been loaded. Do you want to deliver this stimulus video?", aoslo_movie.nStimVideoNum);
			ret = AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION);
			if (ret == IDYES) {
				aoslo_movie.bWithStimVideo = TRUE;
				g_bStimulusOn = TRUE;
				unsigned short *sbuf;
				sbuf = new unsigned short [aoslo_movie.nStimVideoNX[0]*aoslo_movie.nStimVideoNY[0]];
				ZeroMemory(sbuf, sizeof(unsigned short)*aoslo_movie.nStimVideoNX[0]*aoslo_movie.nStimVideoNY[0]);
				g_objVirtex5BMD.AppLoadStimulus(g_hDevVirtex5, sbuf, aoslo_movie.nStimVideoNX[0], aoslo_movie.nStimVideoNY[0], 3);
				delete [] sbuf;
			} else {
				aoslo_movie.bWithStimVideo = FALSE;
			}
		}
		else { 
			g_bMatlabVideo = TRUE;
			if (g_bMatlabAVIsavevideo) { //start playing video while recording //12/15/2011
				g_viewMsgVideo->PostMessage(WM_MOVIE_SEND, 0, SAVE_VIDEO_FLAG);
			}
			else //just start playing video
				g_bMatlab_Update = TRUE; //12/15/2011
		}
	} while (TRUE);
	
	CloseHandle(g_EventReadStimVideo);
}

DWORD WINAPI CICANDIDoc::ThreadNetMsgProcess(LPVOID pParam)
{
	CICANDIDoc *parent = (CICANDIDoc *)pParam;
	CButton *wnd;
	float fGain = -1.;
	int i,len,ind, dewarp_sx, dewarp_sy;
	CString msg, initials, ext, folder;	
	char command;
	char seps[] = "\t", seps1[] = ","; //for parsing matlab sequence
	BOOL bUpdate, bLoop, bTrigger;



	while(TRUE) {
		switch(::WaitForMultipleObjects(3, parent->m_eNetMsg, FALSE, INFINITE)) {//Process the message
			case WAIT_OBJECT_0: //AO message
				msg = parent->m_strNetRecBuff[0];
				command = msg[0];
				msg = msg.Right(msg.GetLength()-1);
				switch (command) {
					case 'A': //disable 'Save Video'
						break;
					case 'S':
						if (parent->m_bPlayback == TRUE)
							parent->StopPlayback();
						//	if (parent->m_bCameraConnected == TRUE)
						//		parent->OnCameraDisconnect();				
						//	parent->UpdateAllViews(NULL);
						break;
					case 'L':
						if (parent->m_bPlayback == TRUE)
							parent->StopPlayback();
						if (parent->m_bCameraConnected == FALSE)
							parent->OnCameraConnect();				
						//				parent->PostMessage(NULL);
						break;
					case 'Q': //process close msg
						if (parent->m_bCameraConnected == TRUE)
							parent->OnCameraDisconnect();
						//	AfxGetMainWnd()->PostMessage(WM_CLOSE, 0, 0);
						break;
					case 'M': //process mark frame msg
						break;
					case 'C': //Create directory with new prefix				
						initials = msg.Left(msg.Find("\\",0)); //gives the prefix
						g_viewMsgVideo->SetDlgItemText(IDC_EDITBOX_PREFIX, initials);			
						CreateDirectory((g_ICANDIParams.VideoFolderPath+initials), NULL);
						parent->m_VideoFolder = g_ICANDIParams.VideoFolderPath +msg;
						CreateDirectory(parent->m_VideoFolder, NULL);
						initials.Empty();
						break;
					case 'G':
						ind = msg.ReverseFind('\\');
						g_viewMsgVideo->SetDlgItemText(IDC_EDITBOX_VIDEOLEN, msg.Right(msg.GetLength()-(ind+1)));
						msg = msg.Left(msg.GetLength() - (msg.GetLength()-ind));
						ind = msg.ReverseFind('\\');				
						parent->m_videoFileName = msg.Right(msg.GetLength()-(ind+1));	
						ind = parent->m_videoFileName.ReverseFind(_T('_V'));
						parent->m_bExtCtrl = TRUE;
						parent->m_nVideoNum = atoi(parent->m_videoFileName.Right(parent->m_videoFileName.GetLength()-(ind+2)));
						g_viewMsgVideo->PostMessage(WM_MOVIE_SEND, 0, SAVE_VIDEO_FLAG);
						break;
					case 'P':
						msg = g_ICANDIParams.VideoFolderPath + msg;
						g_viewMsgVideo->SetDlgItemText(IDL_VIDEO_FILENAME, msg);
						parent->PlaybackMovie(LPCTSTR(msg));
						break;
					case 'D':				
						//write_ScreenText(m_CFtxtfont2,m_CRDefocusValue,text,RGB(255,255,0));
						break;
					case 'F':
						if (parent->m_bPlayback == TRUE)
							g_AnimateCtrl->Seek(atoi(msg));
						break;
					case 'U': //start stabilization
						if (parent->m_bPlayback == TRUE)
							parent->StopPlayback();
						if (parent->m_bCameraConnected == FALSE)
							parent->OnCameraConnect();	
						g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, STABILIZATION_GO);
						break;
					case 'O': //stop stabilization
						parent->OnStablizeSuspend();
						break;
					case 'E': //reset reference frame
						g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, STABILIZATION_GO);
						break;
					default:
						break;
				}

		case WAIT_OBJECT_0+1: //Matlab message

			msg = parent->m_strNetRecBuff[1];
#if _DEBUG
			if (messageLogFile.is_open())
				messageLogFile << msg << std::endl;
#endif

			ext = msg.Left(msg.GetLength()-(msg.GetLength()-msg.Find('#',0)));
			msg = msg.Right(msg.GetLength()-msg.Find('#',0)-1);//remove the command part
			if (!msg.IsEmpty())
				if (msg[msg.GetLength()-1] == '#')//if command ended with delimtier'#' delete it
					msg.Delete(msg.GetLength()-1);			
			switch(ext[0])
				{
				case 'F': //fix the current tracking location or the center of the stabilized frame for the rest of the experiment run
					g_nStimPosBak_Matlab.x = g_StimulusPosBak.x;//aoslo_movie.dewarp_sx/2;
					g_nStimPosBak_Matlab.y = g_StimulusPosBak.y;//aoslo_movie.dewarp_sy/2;
					break;	
				case 'G': //start saving video and trigger sequence	
					if (ext == "Gain") {
						fGain = atof(msg.Left(msg.Find('#')));
						aoslo_movie.fStabGainStim = fGain;
						initials.Format("%3.2f", aoslo_movie.fStabGainStim);				
						g_viewMsgVideo->SetDlgItemText(ID_STIM_GAIN_VALUE_LBL, initials);
						fGain = -1.;
					}
					else if (ext == "Gain0Tracking") {
						g_bGain0Tracking = (atoi(msg)) == 1? TRUE:FALSE;
						wnd = (CButton*)g_viewMsgVideo->GetDlgItem(ID_BUTTON_GAIN0_TRACK);
						wnd->SetCheck(g_bGain0Tracking);
					}
					else if (ext == "GRVIDT") { //12/15/2011
						if (strcmp(msg, "-") != 0) {
							g_bMatlab_Update = FALSE;
							g_bMatlab_Loop?g_bMatlab_Loop=FALSE:0;
							g_bMatlab_Trigger = TRUE;
							g_nCurFlashCount = 0;
							parent->m_videoFileName = msg;
							parent->m_bExtCtrl = TRUE;
							g_bMatlabCtrl = TRUE;
							g_viewMsgVideo->PostMessage(WM_MOVIE_SEND, 0, SAVE_VIDEO_FLAG);
						}
						else { //command called by avi file mode to save video						
							g_nCurFlashCount = 0;
							g_bMatlabAVIsavevideo = TRUE;
						}
					}
					break;
				case 'L': //ignore first 3 parameters in this command and load stimuli
					if (ext == "LaserCalibration")
					{
						ind = atoi(msg); //Turn om/off flag
						if (ind)
							g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0 UPDATE_LASERCALIB_ON);
						else
							g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0 UPDATE_LASERCALIB_OFF);
					}
					else if (ext == "Load") {
						//	g_nStimPosBak_Matlab.x = g_StimulusPosBak.x;
						//	g_nStimPosBak_Matlab.y = g_StimulusPosBak.y;	
						g_bMatlabCtrl = FALSE;
						g_bMatlab_Update = FALSE;
						g_bMatlab_Trigger = FALSE;
						parent->Load_Default_Stimulus(true);
						aoslo_movie.WriteMarkFlag = FALSE;
						ext = msg.Right(3);//get stimuli file extension
						ext = '.'+ext;
						msg.Delete(msg.GetLength()-4, 4);
						ind = atoi(msg.Right(msg.GetLength()-msg.ReverseFind('#')-1));//get the ending count of stimuli
						msg = msg.Left(msg.GetLength()-(msg.GetLength()-msg.ReverseFind('#')));				
						i = atoi(msg.Right(msg.GetLength()-msg.ReverseFind('#')-1));//get the starting count number of stimuli
						msg = msg.Left(msg.GetLength()-(msg.GetLength()-msg.ReverseFind('#')));
						initials = msg.Right(msg.GetLength()-msg.ReverseFind('#')-1);//get the prefix of stimuli
						msg = msg.Left(msg.GetLength()-(msg.GetLength()-msg.ReverseFind('#')));				
						folder = msg.Right(msg.GetLength()-msg.ReverseFind('#')-1);//get the folder location of stimuli					
						msg = msg.Left(msg.GetLength()-(msg.GetLength()-msg.ReverseFind('#')));	
						len = atoi(msg.Right(msg.GetLength()-msg.ReverseFind('#')-1));
						if (parent->LoadMultiStimuli_Matlab(len, folder, initials, i, ind, ext) == TRUE)	//load into application buffers
							g_bMatlabCtrl = TRUE;//indicate application that matlab control is initiated
					}
					else if (ext == "Loop") {
						g_bMatlab_Loop = atoi(msg);
					}
					else if (ext == "LL") { //12/15/2011
						len = atoi(msg); //could be -1 or an integer
						g_nFlashCount = len; //-1 = infinite
						g_nCurFlashCount = 0;
					}
					else if (ext == "Locate") { //update stimulus position
						ind = atoi(msg.Left(msg.Find('#')));
						i = atoi(msg.Right(msg.GetLength()-msg.ReverseFind('#')-1));
						g_nStimPos.x = aoslo_movie.dewarp_sx - g_nStimPosBak_Matlab.y -1+ ind;//g_nStimPosBak_Matlab.x + atoi(msg.Left(msg.Find('#'))); //X location
						g_nStimPos.y = g_nStimPosBak_Matlab.x + i;// + atoi(msg.Right(msg.GetLength()-msg.ReverseFind('#')-1)); //Y location
						g_viewDewVideo->SendMessage(WM_MOVIE_SEND, 0, UPDATE_STIM_POS_MATLAB);
						if (parent->m_cpOldLoc.x != 0 || parent->m_cpOldLoc.y != 0) {
							parent->m_cpOldLoc_bk.x = parent->m_cpOldLoc.x;
							parent->m_cpOldLoc_bk.y = parent->m_cpOldLoc.y;
							parent->m_cpOldLoc.x = parent->m_cpOldLoc_bk.x+ind;
							parent->m_cpOldLoc.y = parent->m_cpOldLoc_bk.y+i;
						}
					}
					else if (ext == "LocUser") { //update stimulus location with user defined values
						ind = atoi(msg.Left(msg.Find('#')));
						i = atoi(msg.Right(msg.GetLength()-msg.ReverseFind('#')-1));
						dewarp_sx = (aoslo_movie.dewarp_sx - aoslo_movie.width) / 2;
						dewarp_sy = (aoslo_movie.dewarp_sy - aoslo_movie.height) / 2;
						g_nStimPos.x = ind + dewarp_sy -1;//g_nStimPosBak_Matlab.x + atoi(msg.Left(msg.Find('#'))); //X location
						g_nStimPos.y = i + dewarp_sx -1;// + atoi(msg.Right(msg.GetLength()-msg.ReverseFind('#')-1)); //Y location
						g_viewDewVideo->SendMessage(WM_MOVIE_SEND, 0, UPDATE_STIM_POS_MATLAB);
					}
					else if (ext == "LoadDefaults") { //load default stimulus
						ind = atoi(msg);
						parent->Load_Default_Stimulus(ind);
					}
					break;
				case 'M': //process mark frame msg 02/24/2012 new
					if (ext == "MarkFrame")
						g_bWritePsyStimulus = TRUE;
					break;
				case 'R':
					if (ext == "RunSLR" && g_bRunSLR) {
						if (parent->m_cpOldLoc_bk.x != parent->m_cpOldLoc.x || parent->m_cpOldLoc_bk.y != parent->m_cpOldLoc.y) {
							parent->m_cpOldLoc.x = parent->m_cpOldLoc_bk.x;
							parent->m_cpOldLoc.y = parent->m_cpOldLoc_bk.y;
						}
						PulseEvent(g_eRunSLR);
					}
					else if (ext == "Retrieve" && g_bFFTIsRunning == TRUE) {
						len = atoi(msg);
						if (len == 1 && g_nStimPosBak_Matlab_Retreive.x != -1) {
							g_nStimPos.x = g_nStimPosBak_Matlab_Retreive.x;
							g_nStimPos.y = g_nStimPosBak_Matlab_Retreive.y;
							g_viewDewVideo->SendMessage(WM_MOVIE_SEND, 0, UPDATE_STIM_POS);
						}
						else if (len == 0 && g_StimulusPos.x != -1) {
							g_nStimPosBak_Matlab_Retreive.x = aoslo_movie.dewarp_sy-g_StimulusPos.y-1;
							g_nStimPosBak_Matlab_Retreive.y = g_StimulusPos.x;
						}
					}
					break;
				case 'S': //load sequence g_nSequence_Matlab
					if (ext == "Sequence") {
						bUpdate = g_bMatlab_Update;
						bLoop = g_bMatlab_Loop;
						bTrigger = g_bMatlab_Trigger;
						g_bMatlab_Loop = FALSE;
						g_bMatlab_Trigger = FALSE;				
						g_bMatlab_Update = FALSE;
						g_nFlashCount = 0;
						g_nFlashCount = atoi(msg);
						//read the sequence file and parse it
						MATFile *pmat;
						const char* name=NULL;
						mxArray *pa;
						double* in;	

						pmat = matOpen("D:\\TempExpiriments\\Seqfile.mat", "r");
						if (pmat == NULL) 
							break;

						pa = matGetNextVariable(pmat, &name);
						while (pa!=NULL)
						{
							in = mxGetPr(pa);
							
#if _DEBUG
							if (messageLogFile.is_open())
							{
								messageLogFile << name << " " << *in << std::endl;
							}
#endif

							if (!strcmp(name, "aom0seq")) {
								VD_trunctoUS(g_nSequence_Matlab[0], in, g_nFlashCount);
							}
							else if (!strcmp(name, "aom0pow")) {
								VD_mulC(in, in, g_nFlashCount, 100.);
								VD_trunctoUS(g_nIRPower_Matlab, in, g_nFlashCount);
							}
							else if (!strcmp(name, "aom0locx")) {
								switch (g_ICANDIParams.FRAME_ROTATION)	{
										case 0: //no rotation or flip
											VD_trunctoSI(g_nLocX[0], in, g_nFlashCount);
											break;
										case 1: //rotate 90 deg
											VD_trunctoSI(g_nLocY[0], in, g_nFlashCount);
											break;
										case 2: //flip along Y axis
											VD_trunctoSI(g_nLocX[0], in, g_nFlashCount);
											break;
										default: ;
								}
								//	VD_trunctoSI(g_nLocX[0], in, g_nFlashCount);
								//	VSI_mulC(g_nLocX[0], g_nLocX[0], g_nFlashCount, -1); // for image rotation
							}
							else if (!strcmp(name, "aom0locy")) {
								switch (g_ICANDIParams.FRAME_ROTATION)	{
										case 0: //no rotation or flip
											VD_trunctoSI(g_nLocY[0], in, g_nFlashCount);
											break;
										case 1: //rotate 90 deg
											VD_trunctoSI(g_nLocX[0], in, g_nFlashCount);
											break;
										case 2: //flip along Y axis
											VD_trunctoSI(g_nLocY[0], in, g_nFlashCount);
											VSI_mulC(g_nLocY[0], g_nLocY[0], g_nFlashCount, -1);
											break;
										default: ;
								}
								//	VD_trunctoSI(g_nLocY[0], in, g_nFlashCount);
							}
							else if (!strcmp(name, "aom1seq")) {
								VD_trunctoUS(g_nSequence_Matlab[1], in, g_nFlashCount);
							}
							else if (!strcmp(name, "aom1pow")) {
								VD_mulC(in, in, g_nFlashCount, 1000.);
								VD_trunctoUS(g_nRedPower_Matlab, in, g_nFlashCount);
							}
							else if (!strcmp(name, "aom1offx")) {
								switch (g_ICANDIParams.FRAME_ROTATION)	{
										case 0: //no rotation or flip
											VD_trunctoSI(g_nLocX[1], in, g_nFlashCount);
											break;
										case 2: //flip along Y axis
											VD_trunctoSI(g_nLocX[1], in, g_nFlashCount);
											VSI_mulC(g_nLocX[1], g_nLocX[1], g_nFlashCount, -1);
											break;
										default: ;
								}
								//	VD_trunctoSI(g_nLocX[1], in, g_nFlashCount);
								//	VSI_mulC(g_nLocX[1], g_nLocX[1], g_nFlashCount, -1);
								VSI_limit( g_nLocX[1], g_nLocX[1], g_nFlashCount, -64, 64 );
							}
							else if (!strcmp(name, "aom1offy")) {
								switch (g_ICANDIParams.FRAME_ROTATION)	{
										case 0: //no rotation or flip
										case 2:
											VD_trunctoSI(g_nLocY[1], in, g_nFlashCount);
											break;
										case 1: //flip along X axis
											VD_trunctoSI(g_nLocY[1], in, g_nFlashCount);
											// VSI_mulC(g_nLocY[1], g_nLocY[1], g_nFlashCount, -1);
											break;
										default: ;
								}
								//	VD_trunctoSI(g_nLocY[1], in, g_nFlashCount);
								VSI_limit( g_nLocY[1], g_nLocY[1], g_nFlashCount, -64, 64 );
							}
							else if (!strcmp(name, "aom2seq")) {
								VD_trunctoUS(g_nSequence_Matlab[2], in, g_nFlashCount);
							}
							else if (!strcmp(name, "aom2pow")) {
								VD_mulC(in, in, g_nFlashCount, 1000.);
								VD_trunctoUS(g_nGreenPower_Matlab, in, g_nFlashCount);
							}
							else if (!strcmp(name, "aom2offx")) {
								VD_trunctoSI(g_nLocX[2], in, g_nFlashCount);
								//	VSI_mulC(g_nLocX[2], g_nLocX[2], g_nFlashCount, -1);
								VSI_limit( g_nLocX[2], g_nLocX[2], g_nFlashCount, -32, 32 );
							}
							else if (!strcmp(name, "aom2offy")) {
								VD_trunctoSI(g_nLocY[2], in, g_nFlashCount);
								VSI_limit( g_nLocY[2], g_nLocY[2], g_nFlashCount, -32, 32 );
							}
							else if (!strcmp(name, "gainseq")) {
								V_DtoF( g_fGain_Matlab, in, g_nFlashCount);
							}
							else if (!strcmp(name, "angleseq")) {
								VD_trunctoUB( g_ubAngle_Matlab, in, g_nFlashCount);
							}
							else if (!strcmp(name, "stimbeep")) {
								VD_trunctoUB( g_ubStimPresFlg_Matlab, in, g_nFlashCount);
							}
							//get next variable
							pa = matGetNextVariable(pmat,&name);					
						}
						//destroy allocated matrix
						mxDestroyArray(pa);
						matClose(pmat);

						g_nCurFlashCount = 0; //Set the sequence position to '0'
						g_bMatlab_Loop = bLoop;
						g_bMatlab_Trigger = bTrigger;
						g_bMatlab_Update = bUpdate;
					}
					else if (ext == "StimulusOn") {
						g_bStimulusOn = (atoi(msg)) == 1? TRUE:FALSE;					
					}
					break;
				case 'T'://terminate matlab
					if (ext == "TurnOn") {
						ind = atoi(msg.Left(msg.Find('#'))); //which channel
						i = atoi(msg.Right(msg.GetLength()-msg.Find('#')-1));
						if (ind == 0) {//IR						
							aoslo_movie.bIRAomOn = i; 
						}
						else if (ind == 1) { //Red
							aoslo_movie.bRedAomOn = i;
						}
						else if (ind == 2) { //Green
							aoslo_movie.bGrAomOn = i;
						}
						g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, UPDATE_AOMS_STATE);
					}
					else if (ext == "Trigger") { //12/15/2011
						g_nCurFlashCount = 0;
						if (msg == "AVI") 
							g_bMatlabAVIsavevideo = FALSE;
						else {
							g_bMatlab_Trigger = TRUE;
							g_bMatlab_Update = TRUE;					
						}
					}
					else {
						g_bMatlabCtrl = FALSE;
						g_bMatlab_Loop = FALSE;
						g_bMatlab_Update = FALSE;
						g_bMatlab_Trigger = FALSE;
						g_bStimulusOn = TRUE;
					}
					break;
				case 'U': //ignore the first parameter and update the active stimuli number #IRGB#
					if (ext == "Update") {
						ind = atoi(msg.Left(msg.Find('#'))); //IR
						msg = msg.Right(msg.GetLength()-msg.Find('#')-1);
						i = atoi(msg.Left(msg.Find('#'))); //Red
						msg = msg.Right(msg.GetLength()-msg.Find('#')-1);
						len = atoi(msg.Left(msg.Find('#'))); //Green
						if (aoslo_movie.stimuli_num > ind && aoslo_movie.stimuli_num > i && aoslo_movie.stimuli_num > len && !g_bMatlabVideo) {
							if (ind == -1)
								g_nSequence_Matlab[0][0] = aoslo_movie.stimuli_num-1;
							else if (ind > -1)
								g_nSequence_Matlab[0][0] = ind;
							if (i == -1)
								g_nSequence_Matlab[1][0] = aoslo_movie.stimuli_num-1;
							else if (i > -1)
								g_nSequence_Matlab[1][0] = i;
							if (len == -1)
								g_nSequence_Matlab[2][0] = aoslo_movie.stimuli_num-1;
							else if (len > -1)
								g_nSequence_Matlab[2][0] = len;
							g_sMatlab_Update_Ind[0] = g_sMatlab_Update_Ind[1] = g_sMatlab_Update_Ind[2] = 0;
							g_bMatlab_Loop = FALSE;
							g_bMatlab_Trigger = FALSE;
							g_bMatlab_Update = TRUE;
						}
					}
					else if (ext == "UpdatePower") {
						ind = atoi(msg.Left(msg.Find('#')));
						if (!ind) //IR
							g_ncurIRPos = (int)(atof(msg.Right(msg.GetLength()-msg.ReverseFind('#')-1))*100);
						else if (ind == 1) //Red
							g_ncurRedPos = (int)(atof(msg.Right(msg.GetLength()-msg.ReverseFind('#')-1))*1000);
						else if (ind == 2) //Green
							g_ncurGreenPos = (int)(atof(msg.Right(msg.GetLength()-msg.ReverseFind('#')-1))*1000);
						g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, UPDATE_LASERS);
					}
					else if (ext == "UpdateOffset") {
						//	ind = atoi(msg.Left(msg.Find('#'))); 
						if (!g_bTCAOverride) {
							g_bTCAOverride = 1;
							wnd = (CButton*)g_viewMsgVideo->GetDlgItem(IDC_TCA_OVERRIDE);
							wnd->SetCheck(g_bTCAOverride);
							wnd->EnableWindow(1);						
						}
						ind = atoi(msg.Left(msg.Find('#'))); //OffsX						
						g_RGBClkShiftsUser[0].x = ind;
						msg = msg.Right(msg.GetLength()-msg.Find('#')-1);
						ind = atoi(msg.Left(msg.Find('#'))); 
						g_RGBClkShiftsUser[0].y = ind;
						msg = msg.Right(msg.GetLength()-msg.Find('#')-1);
						ind = atoi(msg.Left(msg.Find('#'))); //OffsX						
						g_RGBClkShiftsUser[1].x = ind;
						msg = msg.Right(msg.GetLength()-msg.Find('#')-1);
						ind = atoi(msg.Left(msg.Find('#'))); 
						g_RGBClkShiftsUser[1].y = ind;
						g_viewMsgVideo->PostMessage(WM_MOVIE_SEND, 0, UPDATE_USER_TCA);
					}
					else if (ext == "UpdateLaserCalibration14Bit")
					{
						ind = atoi(msg.Left(msg.Find('#'))); //aom
						i = atof(msg.Right(msg.GetLength() - msg.ReverseFind('#') - 1)); //bit value
						switch (ind)
						{
						case 0: // IR
							g_objVirtex5BMD.AppUpdateIRLaser(g_hDevVirtex5, (BYTE)i);
							break;
						case 1: // Red
							g_objVirtex5BMD.AppUpdate14BITsLaserRed(g_hDevVirtex5, (unsigned short)i);
							break;
						case 2: // Green
							g_objVirtex5BMD.AppUpdate14BITsLaserGR(g_hDevVirtex5, (unsigned short)i);
							break;
						default:
							CString errMsg;
							errMsg.Format("Failed to update unknown channel (14 bit): %d.", ind);
							AfxMessageBox(errMsg, MB_ICONEXCLAMATION);
							break;
						}
					}
					else if (ext == "UpdateLaserCalibrationLUT")
					{
						ind = atoi(msg.Left(msg.Find('#'))); //aom
						i = atof(msg.Right(msg.GetLength() - msg.ReverseFind('#') - 1)); // LUT index value
						switch (ind)
						{
						case 0: // IR
							g_objVirtex5BMD.AppUpdateIRLaser(g_hDevVirtex5, g_usIR_LUT[i]);
							break;
						case 1: // Red
							g_objVirtex5BMD.AppUpdate14BITsLaserRed(g_hDevVirtex5, g_usRed_LUT[i]);
							break;
						case 2: // Green
							g_objVirtex5BMD.AppUpdate14BITsLaserGR(g_hDevVirtex5, g_usGreen_LUT[i]);
							break;
						default:
							CString errMsg;
							errMsg.Format("Failed to update unknown channel (LUT): %d.", ind);
							AfxMessageBox(errMsg, MB_ICONEXCLAMATION);
							break;
						}
					}
					break;
				case 'V':
					if (ext == "VPC") { //load prefix & create directory //12/15/2011
						parent->GetSysTime(parent->m_VideoTimeStamp);
						initials = msg.Left(msg.Find("\\",0)); //gives the prefix
						g_viewMsgVideo->SetDlgItemText(IDC_EDITBOX_PREFIX, initials);			
						CreateDirectory((g_ICANDIParams.VideoFolderPath+initials), NULL);
						parent->m_VideoFolder = g_ICANDIParams.VideoFolderPath + initials + "\\" + parent->m_VideoTimeStamp + parent->m_VideoFolderSuffix + "\\";
						//parent->m_VideoFolder = g_ICANDIParams.VideoFolderPath + msg; // was before michen+wolf
						CreateDirectory(parent->m_VideoFolder, NULL);
						initials.Empty();
						parent->m_nVideoNum = 0;
					}
					else if (ext == "VP") { //load prefix //12/15/2011				
						g_viewMsgVideo->SetDlgItemText(IDC_EDITBOX_PREFIX, msg);					
					}
					else if (ext == "VL") { //load video length
						g_viewMsgVideo->SetDlgItemText(IDC_EDITBOX_VIDEOLEN, msg);
					}
					break;

				}

	
			case WAIT_OBJECT_0+2: // getting remote controlled by IGUIDE
				msg = parent->m_strNetRecBuff[2];
				command = msg[0];
				msg = msg.Right(msg.GetLength()-1);
				switch (command) {
					case 'R': //reset reference frame
						g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, STABILIZATION_GO);
						break;
					case 'V': //record video
						if (parent->m_bCameraConnected == FALSE)
							parent->OnCameraConnect();	
						g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, SAVE_VIDEO_FLAG);
						
					break;
				}

			}

		msg.Empty();
	}
}

DWORD WINAPI CICANDIDoc::ThreadSLRProcess(LPVOID pParam)
{
	CICANDIDoc *parent = (CICANDIDoc *)pParam;
	double scale = 100.;
	double* result;
	result = (double*)calloc(2,sizeof(double));
	mwArray out(1, 2, mxDOUBLE_CLASS, mxREAL);
	mwArray in1(1, 1, mxDOUBLE_CLASS, mxREAL);
	mwArray in2(aoslo_movie.height, aoslo_movie.width, mxUINT8_CLASS, mxREAL);
	mwArray in3(aoslo_movie.height, aoslo_movie.width, mxUINT8_CLASS, mxREAL);
		
	while(TRUE) {
		::WaitForSingleObject(g_eRunSLR, INFINITE);		
				
		in1.SetData(&scale, 1);
		in2.SetData(parent->m_pOldRef, aoslo_movie.height*aoslo_movie.width);
		in3.SetData(aoslo_movie.video_ref_bk, aoslo_movie.height*aoslo_movie.width);
		out.SetData(result,2);
		
		SLR(1, out, in1, in2, in3);
		out.GetData(result,2);

		g_nStimPos.x = (int)(parent->m_cpOldLoc.x - result[1] + dewarp_x);
		g_nStimPos.y = (int)(parent->m_cpOldLoc.y - result[0] + dewarp_y);
		g_viewDewVideo->SendMessage(WM_MOVIE_SEND, 0, UPDATE_STIM_POS);
	}
}

DWORD WINAPI CICANDIDoc::ThreadSyncOCTVideo(LPVOID pParam)
{
	CICANDIDoc *parent = (CICANDIDoc *)pParam;
	CEvent WaitEvents;
	BYTE status;

	while(TRUE) {
		::WaitForSingleObject(g_eSyncOCT, INFINITE);		
		while (g_bSyncOCT) {
			::WaitForSingleObject(WaitEvents, 1);
			status = (BYTE)Inp32(0x379);
			if (status&(1<<6) && !g_bSyncOCTReady) {
				g_bSyncOCTReady = TRUE;
				g_viewMsgVideo->PostMessage(WM_MOVIE_SEND, 0, SAVE_VIDEO_FLAG);
			}
		}
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////////////
// CICANDIDoc

IMPLEMENT_DYNCREATE(CICANDIDoc, CDocument)

BEGIN_MESSAGE_MAP(CICANDIDoc, CDocument)
	//{{AFX_MSG_MAP(CICANDIDoc)
	ON_COMMAND(ID_SETUP_PARAMS, OnEditParams)
	ON_UPDATE_COMMAND_UI(ID_SETUP_PARAMS, OnUpdateEditParams)
	ON_COMMAND(ID_SETUP_DESINUSOID, OnSetupDesinusoid)
	ON_UPDATE_COMMAND_UI(ID_SETUP_DESINUSOID, OnUpdateSetupDesinusoid)
	ON_COMMAND(ID_SETUP_SAVEREF, OnSaveReference)
	ON_UPDATE_COMMAND_UI(ID_SETUP_SAVEREF, OnUpdateSaveReference)
	ON_COMMAND(ID_SETUP_MOVIENORMALIZE, OnMovieNormalize)
	ON_UPDATE_COMMAND_UI(ID_SETUP_MOVIENORMALIZE, OnUpdateMovieNormalize)
	ON_COMMAND(ID_STABLIZE_LOAD_EXT_REF, OnLoadExtRef)
	ON_UPDATE_COMMAND_UI(ID_STABLIZE_LOAD_EXT_REF, OnUpdateLoadExtRef)
	ON_COMMAND(ID_CAMERA_CONNECT, OnCameraConnect)
	ON_COMMAND(ID_CAMERA_DISCONNECT, OnCameraDisconnect)
	ON_UPDATE_COMMAND_UI(ID_CAMERA_CONNECT, OnUpdateCameraConnect)
	ON_UPDATE_COMMAND_UI(ID_CAMERA_DISCONNECT, OnUpdateCameraDisconnect)
	ON_COMMAND(ID_STABLIZE_GO, OnStablizeGo)
	ON_UPDATE_COMMAND_UI(ID_STABLIZE_GO, OnUpdateStablizeGo)
	ON_COMMAND(ID_STABLIZE_SUSPEND, OnStablizeSuspend)
	ON_UPDATE_COMMAND_UI(ID_STABLIZE_SUSPEND, OnUpdateStablizeSuspend)
	
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CICANDIDoc construction/destruction
CICANDIDoc::CICANDIDoc()
{
//	fopen_s(&g_fp, "data.txt", "w");

	g_bFFTIsRunning     = FALSE;
	m_bSymbol           = FALSE;
	m_nStimSizeX        = 0;
	m_nStimSizeY        = 0;

	//Laser controls
	g_dRedMax		= 1.0;	//max red power at the pupil
	g_dGreenMax		= 1.0;	//max IR power at the pupil
	g_dIRMax		= 1.0;	//max IR power at the pupil
	g_ncurRedPos	= 0;	//current red power slider position
	g_ncurGreenPos  = 0;	//current ir power slider position
	g_ncurIRPos     = 100;

	V_initMT( 8 );
	//Playback handler
	g_AnimateCtrl = NULL;
	m_bExtCtrl	  = FALSE;
	g_bPlaybackUpdate = FALSE;
	g_nStimPos.x = g_nStimPos.y = -1;
	g_nStimPosBak_Matlab.x = g_nStimPosBak_Matlab.y = -1;
	g_nStimPosBak_Matlab_Retreive.x = g_nStimPosBak_Matlab_Retreive.y = -1;
	g_nStimPosBak_Gain0Track.x = g_nStimPosBak_Gain0Track.y = -1;
	
	//Matlab controls
	g_bMatlabCtrl		= FALSE;
	g_bMatlabVideo		= FALSE;
	g_bMatlabAVIsavevideo = FALSE;
	g_sMatlab_Update_Ind[0] = g_sMatlab_Update_Ind[1] = g_sMatlab_Update_Ind[2] = 0;
	g_nSequence_Matlab[0][0] = 1;
	g_nSequence_Matlab[1][0] = 0;	
	g_nSequence_Matlab[2][0] = 0;
	g_bMatlab_Update	= FALSE;
	g_bMatlab_Loop		= FALSE;
	g_bRunSLR			= FALSE;
	g_bSyncOCT			= FALSE;
	g_bSyncOCTReady		= FALSE;

	g_usRed_LUT		= NULL;
	g_usGreen_LUT	= NULL;
	g_usIR_LUT		= NULL;

	// initialize movie information
	aoslo_movie.width       = 0;
	aoslo_movie.height      = 0;
	aoslo_movie.FlashOnFlag = TRUE;
	aoslo_movie.RandDelivering = FALSE;
	aoslo_movie.WriteMarkFlag  = FALSE;
	aoslo_movie.stimulus_loc_flag = FALSE;
	aoslo_movie.stimulus_audio_flag = FALSE;
	aoslo_movie.RandPathIndice = NULL;

	g_ICANDIParams.PATCH_CNT_OLD = 0;
	g_BlockCounts = 0;
	g_BlockIndex  = 0;
	g_PatchParamsA.UnMatrixIdx = NULL;
	g_PatchParamsA.UnMatrixVal = NULL;
	g_PatchParamsA.StretchFactor = NULL;
	g_bStimulusOn = TRUE;	
	g_bGain0Tracking = FALSE;
	g_bTimingTest = TIMING_FLAG_ON;
	g_fpTimeLog   = NULL;
	aoslo_movie.PredictionTime  = 3;
	aoslo_movie.DeliveryMode    = 0;		// single cone
	aoslo_movie.FlashFrequency  = 3;
	aoslo_movie.FlashDutyCycle  = 4;
	aoslo_movie.FlashDist       = GRABBER_FRAME_RATE/aoslo_movie.FlashFrequency;
	aoslo_movie.StimulusNum     = 0;
	for (int i = 0; i < MAX_STIMULUS_NUMBER; i ++)
		aoslo_movie.RandPathIndex[i] = i;
	aoslo_movie.memory_pool_ID  = 0;
	aoslo_movie.avi_handle_on_A = FALSE;
	aoslo_movie.avi_handle_on_B = FALSE;
	aoslo_movie.video_saveA1    = NULL;
	aoslo_movie.video_saveA2    = NULL;
	aoslo_movie.video_saveA3    = NULL;
	aoslo_movie.video_saveB1    = NULL;
	aoslo_movie.video_saveB2    = NULL;
	aoslo_movie.video_saveB3    = NULL;
	aoslo_movie.stim_rd_buffer  = new unsigned short [((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY];
	aoslo_movie.stim_gr_buffer  = new unsigned short [((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY];
	aoslo_movie.stim_ir_buffer  = new unsigned short [((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY];
	aoslo_movie.stim_rd_nx      = STIMULUS_SIZE_X;
	aoslo_movie.stim_rd_ny      = STIMULUS_SIZE_Y;
	aoslo_movie.stim_gr_nx      = STIMULUS_SIZE_X;
	aoslo_movie.stim_gr_ny      = STIMULUS_SIZE_Y;
	aoslo_movie.stim_ir_nx      = STIMULUS_SIZE_X;
	aoslo_movie.stim_ir_ny      = STIMULUS_SIZE_Y;
	aoslo_movie.stim_loc_y      = new int [((CICANDIApp*)AfxGetApp())->m_imageSizeY];
	aoslo_movie.stim_num_y      = 0;
	aoslo_movie.stim_loc_yir    = new int [((CICANDIApp*)AfxGetApp())->m_imageSizeY];
	aoslo_movie.stim_num_yir    = 0;
	aoslo_movie.stim_loc_yrd    = new int [((CICANDIApp*)AfxGetApp())->m_imageSizeY];
	aoslo_movie.stim_num_yrd    = 0;
	aoslo_movie.stim_loc_ygr    = new int [((CICANDIApp*)AfxGetApp())->m_imageSizeY];
	aoslo_movie.stim_num_ygr    = 0;
	aoslo_movie.x_cc_gr         = 0;
	aoslo_movie.x_center_gr     = 0;
	aoslo_movie.x_left_gr       = 0;
	aoslo_movie.x_right_gr      = 0;
	aoslo_movie.x_cc_ir         = 0;
	aoslo_movie.x_center_ir     = 0;
	aoslo_movie.x_left_ir       = 0;
	aoslo_movie.x_right_ir      = 0;
	aoslo_movie.x_cc_rd         = 0;
	aoslo_movie.x_center_rd     = 0;
	aoslo_movie.x_left_rd       = 0;
	aoslo_movie.x_right_rd      = 0;
	aoslo_movie.stimuli_buf     = NULL;
	aoslo_movie.stimuli_sx      = NULL;
	aoslo_movie.stimuli_sy      = NULL;
	aoslo_movie.stimuli_buf		= new unsigned short*[900]; 	
	aoslo_movie.stimuli_sx		= new unsigned short[900]; 
	aoslo_movie.stimuli_sy		= new unsigned short[900]; 
	aoslo_movie.stimuli_num     = 0;
	aoslo_movie.stim_video      = NULL;
	aoslo_movie.bStimRewind     = FALSE;
	aoslo_movie.fStabGainStim   = 1.0;
	aoslo_movie.fStabGainMask   = 1.5;
	aoslo_movie.bWithStimVideo  = FALSE;
	aoslo_movie.sampPoints      = 0;
	aoslo_movie.nLaserPowerRed  = 0x3FFF;
	aoslo_movie.nLaserPowerGreen= 0x3FFF;
	aoslo_movie.nLaserPowerIR   = 0xFF;
	aoslo_movie.weightsRed      = new UINT32 [1024];
	aoslo_movie.weightsGreen    = new UINT32 [1024];
	aoslo_movie.nRotateLocation = 0;
	aoslo_movie.bPupilMask      = FALSE;
	aoslo_movie.bOneFrameDelay  = FALSE;
	aoslo_movie.bIRAomOn		= TRUE;
	aoslo_movie.bRedAomOn		= TRUE;
	aoslo_movie.bGrAomOn		= TRUE;
//	m_strStimVideoName          = "";
	m_strStimVideoName          = NULL;
	m_nMovieLength				= GRABBER_FRAME_RATE;//30;
	g_StimulusPos0Gain.x        = 0;
	g_StimulusPos0Gain.y        = 0;

	aoslo_movie.TCAboxWidth     = 255;
	aoslo_movie.TCAboxHeight    = 128;
	g_bRecord					= FALSE;
	g_nViewID                   = 0;

	g_StimulusPos0G.x = g_StimulusPos0G.y = -1;

	CString filename, strCurrentDir;
	char    szAppPath[80] = "";
	
	::GetModuleFileName(0, szAppPath, sizeof(szAppPath) - 1);	
	// Extract directory
	strCurrentDir = szAppPath;
	strCurrentDir = strCurrentDir.Left(strCurrentDir.ReverseFind('\\'));
	filename = strCurrentDir.Right(strCurrentDir.Find('\\'));
	if (filename.Find("ug") >= 0 || filename.Find("se") >= 0)
		strCurrentDir = strCurrentDir.Left(strCurrentDir.ReverseFind('\\'));	
	g_ICANDIParams.m_strConfigPath = strCurrentDir;
	g_ICANDIParams.LoadConfigFile();
	g_ICANDIParams.LoadWavelengthOptions();

	// status of camera connection
	m_bCameraConnected = FALSE;
	// status of video saving
	m_bDumpingVideo    = FALSE;
	m_bUpdateDutyCycle = FALSE;

	g_bEOFHandler      = FALSE;
	g_bMultiStimuli    = FALSE;
	g_bStimProjection  = FALSE;
	g_bStimProjectionIR= FALSE;
	g_bStimProjectionRD= FALSE;
	g_bStimProjectionGR= FALSE;
	g_bStimProjSel	   = TRUE;
	g_bCalcStimPos     = FALSE;

	m_aviFileNameA     = _T("");
	m_aviFileNameB     = _T("");
	m_videoFileName	   = _T("");
	m_VideoFolder	   = g_ICANDIParams.VideoFolderPath;
	m_VideoFolderSuffix= g_ICANDIParams.VideoFolderSuffix;
	m_bValidAviHandleA = FALSE;
	m_bValidAviHandleB = FALSE;
	m_iSavedFramesA    = 0;
	m_iSavedFramesB    = 0;
	m_fpStimPos        = NULL;

	g_bEOF             = TRUE;
	m_bPlayback		   = FALSE;
	
	g_Channel1Shift.y  = 0;
	g_Channel1Shift.x  = 0;
	g_Channel2Shift.y  = 0;
	g_Channel2Shift.x  = 0;	

	g_Motion_ScalerX = 0.;
	g_Motion_ScalerY = 0.;

	g_EventLoadStim = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_LOADSTIM_EVENT_RED");
	if (!g_EventLoadStim) {
		AfxMessageBox("Failed to create an event for loading stimulus", MB_ICONEXCLAMATION);
	}
	g_EventLoadLUT = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_LOADLUT_EVENT");
	if (!g_EventLoadLUT) {
		AfxMessageBox("Failed to create an event for loading lookup table for warping stimulus pattern", MB_ICONEXCLAMATION);
	}
	g_EventReadStimVideo = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_READSTIMVIDEO_EVENT");
	if (!g_EventReadStimVideo) {
		AfxMessageBox("Failed to create an event for reading communication status for AO and Matlab", MB_ICONEXCLAMATION);
	}
	g_EventSendViewMsg = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_SENDMSG_EVENT");
	if (!g_EventSendViewMsg) {
		AfxMessageBox("Failed to create an event for sending messages to update views", MB_ICONEXCLAMATION);
	}
	g_EventStimPresentFlag = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_STIMPRESENT_EVENT");
	if (!g_EventStimPresentFlag) {
		AfxMessageBox("Failed to create an event for reading communication status for AO and Matlab", MB_ICONEXCLAMATION);
	}

	thd_handle[4] = CreateThread(NULL, 0, ThreadLoadData2FPGA, this, 0, &thdid_handle[4]);
	SetThreadPriority(thd_handle[4], THREAD_PRIORITY_HIGHEST);

	thd_handle[5] = CreateThread(NULL, 0, ThreadReadStimVideo, this, 0, &thdid_handle[5]);
	SetThreadPriority(thd_handle[5], THREAD_PRIORITY_LOWEST);

	thd_handle[6] = CreateThread(NULL, 0, ThreadPlayStimPresentFlag, this, 0, &thdid_handle[6]);
	SetThreadPriority(thd_handle[6], THREAD_PRIORITY_LOWEST);
	
	thd_handle[8] = CreateThread(NULL, 0, ThreadSendViewMsg, this, 0, &thdid_handle[8]);
	SetThreadPriority(thd_handle[8], THREAD_PRIORITY_LOWEST);
	
//	g_bKernelPlugin             = TRUE;

	g_VideoInfo.end_line_ID     = 16;//600;		// update this variable dynamically
	g_VideoInfo.img_width       = 520;//512;//800;
	g_VideoInfo.img_height      = (int)((CICANDIApp*)AfxGetApp())->m_imageSizeY;//600;
	g_VideoInfo.line_spacing    = 16;
	g_VideoInfo.line_start_addr = 0x0000;//65535;
	g_VideoInfo.line_end_addr   = 0;				// update this variable dynamically
	g_VideoInfo.addr_interval   = 1056;
	g_VideoInfo.offset_line     = 0x00;//0x17;
	g_VideoInfo.offset_pixel    = 0x1a;;//0xb9
	g_VideoInfo.tlp_counts      = 67;//2090;//3760;
	g_VideoInfo.video_in        = NULL;
	g_VideoInfo.video_out       = NULL;
	g_VideoInfo.video_in  = new unsigned char [g_VideoInfo.img_width*g_VideoInfo.img_height];
//	g_VideoInfo.video_out = new unsigned char [g_VidenidraoInfo.img_width*g_VideoInfo.img_height];
//	if (g_VideoInfo.video_in == NULL || g_VideoInfo.video_out == NULL) 
	if (g_VideoInfo.video_in == NULL) 
		AfxMessageBox("Error! Can't allocate memory space for video.");

	if (((CICANDIApp*)AfxGetApp())->m_bInvalidDevice == FALSE)
		UpdateImageGrabber();
	LINE_INTERVAL = g_VideoInfo.line_spacing;

	// initialize stimulus information
	g_objVirtex5BMD.AppInitStimulus(g_hDevVirtex5, g_VideoInfo, aoslo_movie.stim_rd_nx, aoslo_movie.stim_rd_ny);
	
	Load_Default_Stimulus(false);

	// write default LUT for warping stimulus pattern
	unsigned short *lut_buf;
	lut_buf = new unsigned short [aoslo_movie.stim_rd_nx];
	for (int i = 0; i < aoslo_movie.stim_rd_nx; i ++) lut_buf[i] = i;
	g_objVirtex5BMD.AppWriteWarpLUT(g_hDevVirtex5, aoslo_movie.stim_rd_nx, aoslo_movie.stim_rd_nx, lut_buf, 0);
	delete [] lut_buf;

	g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, 0, ((CICANDIApp*)AfxGetApp())->m_imageSizeX, 0, 0, 0, -1, -100);
	g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, false, 0, 0, 0, 0, 0, 0, 0, ((CICANDIApp*)AfxGetApp())->m_imageSizeX, 0, 
										0, 0, 0, 0, 0, 0, 0, 0, 0);

	//g_objVirtex5BMD.AppShiftRedY(g_hDevVirtex5, 5);
	//g_objVirtex5BMD.AppShiftGreenY(g_hDevVirtex5, -5);

	aoslo_movie.width     = ((CICANDIApp*)AfxGetApp())->m_imageSizeX;
	aoslo_movie.height    = ((CICANDIApp*)AfxGetApp())->m_imageSizeY;	
	m_frameSizeA = cv::Size(static_cast<int>(aoslo_movie.width), static_cast<int>(aoslo_movie.height));
	aoslo_movie.dewarp_sx = ((CICANDIApp*)AfxGetApp())->m_imageSizeX + dewarp_x*2;
	aoslo_movie.dewarp_sy = ((CICANDIApp*)AfxGetApp())->m_imageSizeY + dewarp_y*2;
	m_frameSizeB = cv::Size(static_cast<int>(aoslo_movie.dewarp_sx), static_cast<int>(aoslo_movie.dewarp_sy));

	// create a timeout timer for video sampling
    TIMECAPS tc;
    timeGetDevCaps(&tc, sizeof(TIMECAPS));
    m_uResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
    timeBeginPeriod(m_nTimerRes);

    // create the timer
    m_idTimerEvent = timeSetEvent(500, m_nTimerRes, g_TimeoutTimerFunc, (DWORD)this, TIME_PERIODIC);

	if (!g_usRed_LUT && !g_usGreen_LUT && !g_usIR_LUT)
	{
		g_usRed_LUT	= new unsigned short [1001];
		g_usGreen_LUT	= new unsigned short [1001];
		g_usIR_LUT	= new unsigned short [101];
		Initialize_LUT();
	}

	//Enable network message processing thread and create network listening sockets for AO, Matlab and IGUIDE
	m_eNetMsg = new HANDLE[3];
	m_eNetMsg[0] = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_NETCOMMMSGAO_EVENT");
	if (!m_eNetMsg[0]) {
		AfxMessageBox("Failed to create an event for monitoring AO Comm", MB_ICONEXCLAMATION);
		return;
	}	
	m_eNetMsg[1] = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_NETCOMMMSGAOM_EVENT");
	if (!m_eNetMsg[1]) {
		AfxMessageBox("Failed to create an event for monitoring AOM Comm", MB_ICONEXCLAMATION);
		return;
	}

	m_eNetMsg[2] = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_NETCOMMMSGIGUIDE_EVENT");
	if (!m_eNetMsg[2]) {
		AfxMessageBox("Failed to create an event for monitoring IGUIDE Comm", MB_ICONEXCLAMATION);
		return;
	}

	m_strNetRecBuff = new CString[3];	
	m_ncListener_AO = NULL;
	m_ncListener_Matlab = NULL;
	m_ncListener_IGUIDE = NULL;

	if (CSockClient::SocketInit() != 0)
	{
		AfxMessageBox("Unable to initialize Windows Sockets", MB_OK|MB_ICONERROR, 0);
		return;
	}
	//Create a listener for AO
	m_ncListener_AO = new CSockListener(&m_strNetRecBuff[0], &m_eNetMsg[0]);
	if (!m_ncListener_AO->InitPort("192.168.0.1", 23))
	//if (!m_ncListener_AO->InitPort("153.90.109.35", 23))
	{
	//	AfxMessageBox("Unable to Open port 23 for AO comm", MB_OK|MB_ICONERROR, 0);
	//	return;
	}
	else if (!m_ncListener_AO->Listen())
	{
	//	AfxMessageBox("Unable to Listen on port 23 for AO comm", MB_OK|MB_ICONERROR, 0);
	//	return;
	}
	//Create a listener for Matlab
	m_ncListener_Matlab = new CSockListener(&m_strNetRecBuff[1], &m_eNetMsg[1]);
	if (!m_ncListener_Matlab->InitPort("127.0.0.1", 1300))
	{
		AfxMessageBox("Unable to Open port 1300 for Matlab comm", MB_OK|MB_ICONERROR, 0);
	//	return;
	}
	else if (!m_ncListener_Matlab->Listen())
	{
		AfxMessageBox("Unable to Listen on port 1300 for Matlab comm", MB_OK|MB_ICONERROR, 0);
	//	return;
	}
	//Create a listener for IGUIDE
	m_ncListener_IGUIDE = new CSockListener(&m_strNetRecBuff[2], &m_eNetMsg[2]);
	if (!m_ncListener_IGUIDE->InitPort("127.0.0.1", 1400))
	{
		AfxMessageBox("Unable to Open port 1400 for IGUIDE comm", MB_OK|MB_ICONERROR, 0);
	//	return;
	}
	else if (!m_ncListener_IGUIDE->Listen())
	{
		AfxMessageBox("Unable to Listen on port 1400 for IGUIDE comm", MB_OK|MB_ICONERROR, 0);
	//	return;
	}

 	thd_handle[6] = CreateThread(NULL, 0, ThreadNetMsgProcess, this, 0, &thdid_handle[6]);
	SetThreadPriority(thd_handle[6], THREAD_PRIORITY_NORMAL);

	m_bMatlab = false;	
	const char* args[] = { "-nojvm" };
	const int count = sizeof(args) / sizeof(args[0]);
	if (mclInitializeApplication(args, count))
		if (SLRInitialize())
		{
			m_bMatlab = true;
			m_pOldRef = NULL;
			m_cpOldLoc.x = m_cpOldLoc.y = m_cpOldLoc_bk.x = m_cpOldLoc_bk.y = 0;
			m_pOldRef = new unsigned char[aoslo_movie.width*aoslo_movie.height];
			memset(m_pOldRef, 0, aoslo_movie.width*aoslo_movie.height);	
			g_eRunSLR = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_SLRPROCESS_EVENT");
			thd_handle[7] = CreateThread(NULL, 0, ThreadSLRProcess, this, 0, &thdid_handle[7]);
		}	

	g_eSyncOCT = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_SYNCOCTVIDEO_EVENT");
	thd_handle[8] = CreateThread(NULL, 0, ThreadSyncOCTVideo, this, 0, &thdid_handle[8]);

	g_objVirtex5BMD.AppMotionTraces(g_hDevVirtex5, 0.f, 0.f, 0.0f, 0.0f, NULL); // reset mirror motion

	g_bNoGPUdevice = FALSE;
	int dev_counts;
	if (g_objStabFFT.GetCUDADeviceCounts(&dev_counts) != 0) {
		AfxMessageBox("No GPU device is found on your system. The software won't run.", MB_ICONSTOP);
		g_bNoGPUdevice = TRUE;
		return;
	}

	// initialize FFT/CUDA libraries
	g_StabParams.cenFFTx1  = FFT_WIDTH;
	g_StabParams.cenFFTy1  = FFT_HEIGHT128;
	g_StabParams.cenFFTx2  = FFT_WIDTH;
	g_StabParams.cenFFTy2  = FFT_HEIGHT256;
	g_StabParams.fineX     = FFT_WIDTH*2;
	g_StabParams.fineY     = FFT_HEIGHT064;
	g_StabParams.imgWidth  = g_ICANDIParams.F_XDIM;
	g_StabParams.imgHeight = g_ICANDIParams.F_YDIM;
	g_StabParams.patchCnt  = g_ICANDIParams.PATCH_CNT;
	g_StabParams.osY       = new int [g_StabParams.patchCnt];
	g_StabParams.convKernel= g_ICANDIParams.Filter;
	g_StabParams.slice_height = LINE_INTERVAL;
	for (int i = 0; i < g_StabParams.patchCnt; i ++)
		g_StabParams.osY[i] = g_ICANDIParams.L_YDIM0[i];

	g_objStabFFT.CUDA_FFTinit(g_StabParams);

	// Load default black and white stimulus frames into stimulus buffers
	aoslo_movie.stimuli_num = 2;
	aoslo_movie.stimuli_buf[0] = new unsigned short[256*256];
	aoslo_movie.stimuli_buf[1] = new unsigned short[256*256];
	FillMemory(aoslo_movie.stimuli_buf[0], sizeof(unsigned short)*256*256, 0); //off frame
	VUS_equC(aoslo_movie.stimuli_buf[1], 256*256, 16383); //on frame
	aoslo_movie.stimuli_sx[0] = 256;
	aoslo_movie.stimuli_sy[0] = 256;
	aoslo_movie.stimuli_sx[1] = 256;
	aoslo_movie.stimuli_sy[1] = 256;	

	temptest = NULL;
	temptest = fopen("c:\\locations.txt","w+");
	//Testing section

	// for constant size or constant power option (M.L. & N.D. 2019-10-10)
	
	g_bConstPwr = false;
	linStretch = new int [aoslo_movie.width];
	for (int i = 0; i < 512; i++)
		linStretch[i] = i;

#if _DEBUG
	messageLogFile.open("D:\\MessageLog.txt");
#endif
	// for testing purpose only
	// stack stimuli on raw image
//	g_objVirtex5BMD.AppSetADCchannel(g_hDevVirtex5, 3);
}

void CICANDIDoc::Initialize_LUT()
{
	int i;
	short count = 0;
    char *LUTfile = "utils\\Red_LUT.txt";
	std::ifstream inREDfile(LUTfile);	
    while (inREDfile >> i) 
		g_usRed_LUT[count++] = (unsigned short)i;

	count = 0;
	LUTfile = "utils\\Green_LUT.txt";
	std::ifstream inGreenfile(LUTfile);	
    while (inGreenfile >> i) 
		g_usGreen_LUT[count++] = (unsigned short)i;

	count = 0;
	LUTfile = "utils\\IR_LUT.txt";
	std::ifstream inIRfile(LUTfile);	
    while (inIRfile >> i) 
		g_usIR_LUT[count++] = (unsigned short)i;
}

CICANDIDoc::~CICANDIDoc()
{
//	fclose(g_fp);
	V_closeMT();

//	g_objVirtex5BMD.AppRemoveTCAbox(g_hDevVirtex5);
	g_objVirtex5BMD.AppMotionTraces(g_hDevVirtex5, 0.f, 0.f, 0.0f, 0.0f, NULL); // reset mirror motion

	if (!g_bNoGPUdevice) {
		// release CUDA resources
		g_objStabFFT.CUDA_FFTrelease();
		delete [] g_StabParams.osY;
	}

	if (aoslo_movie.RandPathIndice != NULL) 
		delete [] aoslo_movie.RandPathIndice;
	if (g_VideoInfo.video_in != NULL) 
		delete [] g_VideoInfo.video_in;
	if (g_VideoInfo.video_out != NULL) 
		delete [] g_VideoInfo.video_out;

	if (aoslo_movie.stimuli_num > 0) {
		for (int i = 0; i < aoslo_movie.stimuli_num; i ++) 
			delete [] aoslo_movie.stimuli_buf[i];
		delete [] aoslo_movie.stimuli_buf;
		delete [] aoslo_movie.stimuli_sx;
		delete [] aoslo_movie.stimuli_sy;
	}
	if (aoslo_movie.stim_video != NULL) {
		for (int i = 0; i < aoslo_movie.nStimVideoNum; i ++) 
			delete [] aoslo_movie.stim_video[i];
		delete [] aoslo_movie.stim_video;
		delete [] aoslo_movie.nStimVideoNX;
		delete [] aoslo_movie.nStimVideoNY;
		delete [] aoslo_movie.nStimVideoPlanes;
	}

	delete [] aoslo_movie.stim_rd_buffer;
	delete [] aoslo_movie.stim_gr_buffer;
	delete [] aoslo_movie.stim_ir_buffer;
	delete [] aoslo_movie.stim_loc_y;
	delete [] aoslo_movie.weightsRed;
	delete [] aoslo_movie.weightsGreen;
	
	delete [] g_usRed_LUT;
	delete [] g_usGreen_LUT;
	delete [] g_usIR_LUT;

	// turn off all AOM signals
	g_objVirtex5BMD.AppAOM_Off(g_hDevVirtex5);
	
	delete m_ncListener_AO;
	delete m_ncListener_Matlab;
	delete m_ncListener_IGUIDE;
	CloseHandle(m_eNetMsg[0]);
	CloseHandle(m_eNetMsg[1]);
	CloseHandle(m_eNetMsg[2]);
	delete [] m_eNetMsg;

	if (m_bMatlab) {
		if (m_pOldRef) delete [] m_pOldRef;
		
		SLRTerminate();
		mclTerminateApplication();	
	}

	g_ICANDIParams.SaveConfigFile();

	if (temptest != NULL) fclose(temptest);
	delete [] linStretch;

    timeKillEvent(m_idTimerEvent);
    // reset the timer
    timeEndPeriod (m_uResolution);

#if _DEBUG
	messageLogFile.close();
#endif
}

BOOL CICANDIDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CICANDIDoc serialization

void CICANDIDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CICANDIDoc diagnostics

#ifdef _DEBUG
void CICANDIDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CICANDIDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CICANDIDoc commands

// open the dialog for editing ICANDI parameters
void CICANDIDoc::OnEditParams() 
{
	CICANDIParamsDlg  dlg;

	// here we need to load parameters in the .ini file to the popup window
	// and to the global structure g_ICANDIParams	
	char   *dev_name;	

	int i, dev_counts;
	if (g_objStabFFT.GetCUDADeviceCounts(&dev_counts) != 0) {
		dlg.m_gpu_dev_list = NULL;
		dlg.m_gpu_counts   = 0;
	} else {		
		if (dlg.m_gpu_dev_list != NULL) delete [] dlg.m_gpu_dev_list;
		dlg.m_gpu_dev_list = new CString [dev_counts];
		dlg.m_gpu_counts   = dev_counts;

		dev_name = new char [240];
		for (i = 0; i < dev_counts; i ++) {
			if (g_objStabFFT.GetCUDADeviceNames(i, dev_name) != 0) {
				dlg.m_gpu_dev_list[i] = _T("Unknown device....");
			} else {
				dlg.m_gpu_dev_list[i] = dev_name;
			}
		}
		delete [] dev_name;
	}


	// initialize items on the dialog
	dlg.m_PATCH_CNT = g_ICANDIParams.PATCH_CNT;
	dlg.m_F_XDIM = g_ICANDIParams.F_XDIM;
	dlg.m_F_YDIM = g_ICANDIParams.F_YDIM;
	dlg.m_LAYER_CNT = g_ICANDIParams.L_CT;
	dlg.m_ITERNUM = g_ICANDIParams.ITERFINAL;
	dlg.m_K1 = g_ICANDIParams.K1;
	dlg.m_GTHRESH = g_ICANDIParams.GTHRESH;
	dlg.m_SRCTHRESH = g_ICANDIParams.SRCTHRESH;	
	dlg.m_intDesinusoid = g_ICANDIParams.Desinusoid;
	dlg.m_intInvertTraces = g_InvertMotionTraces;
	dlg.m_intFilterID   = g_ICANDIParams.Filter;
	dlg.m_LUTname = g_ICANDIParams.fnameLUT;
	memcpy(dlg.m_boolWriteMarkFlags, g_ICANDIParams.WriteMarkFlags, sizeof(BOOL)*9);
	if (dlg.DoModal() == IDOK) {
		// update the .ini file and update the global structure g_ICANDIParams
		g_ICANDIParams.PATCH_CNT = dlg.m_PATCH_CNT;
		g_ICANDIParams.F_XDIM = dlg.m_F_XDIM;
		g_ICANDIParams.F_YDIM = dlg.m_F_YDIM;
		g_ICANDIParams.L_CT = dlg.m_LAYER_CNT;
		g_ICANDIParams.ITERFINAL = dlg.m_ITERNUM;
		g_ICANDIParams.K1 = dlg.m_K1;
		g_ICANDIParams.GTHRESH = dlg.m_GTHRESH;
		g_ICANDIParams.SRCTHRESH = dlg.m_SRCTHRESH;
		g_ICANDIParams.Desinusoid = dlg.m_intDesinusoid;
		g_InvertMotionTraces = dlg.m_intInvertTraces;
		g_ICANDIParams.Filter = dlg.m_intFilterID;
		g_ICANDIParams.fnameLUT = dlg.m_LUTname;
		memcpy(g_ICANDIParams.WriteMarkFlags, dlg.m_boolWriteMarkFlags, sizeof(BOOL)*9);
		
		g_ICANDIParams.SaveConfigFile();

		g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, UPDATE_PROCESSOR);
	}
}

void CICANDIDoc::OnUpdateEditParams(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!g_bFFTIsRunning);
}

void CICANDIDoc::OnSetupDesinusoid() 
{
	CCalDesinu   dlg;
	char         temp[80];
	CString      filename;

	dlg.m_strCurrentDir = m_VideoFolder;
	if (dlg.DoModal() == TRUE) {
	//	SetCurrentDirectory(m_strCurrentDir);
		filename = dlg.m_LUTfname;
		filename.TrimLeft(' ');
		filename.TrimRight(' ');
		if (filename.GetLength() == 0) return;

		// get ICANDI parameter .ini file name
		filename = _T("\\AppParams.ini");
		filename = g_ICANDIParams.m_strConfigPath + filename;

		g_ICANDIParams.fnameLUT = dlg.m_LUTfname;
		// desinusoiding look up table
		::WritePrivateProfileString("FrameInfo", "DesinusoidLUT", g_ICANDIParams.fnameLUT, filename);
		// Pixels Per Degree X
		g_ICANDIParams.PixPerDegX = (float)dlg.m_PixPerDegX;
		sprintf_s(temp, "%1.3f", g_ICANDIParams.PixPerDegX);
		::WritePrivateProfileString("ApplicationInfo", "PixelsperDegX", temp, filename);
		g_Motion_ScalerX = (float)(g_ICANDIParams.VRangePerDegX*128./g_ICANDIParams.PixPerDegX);
		// Pixels Per Degree Y
		g_ICANDIParams.PixPerDegY = (float)dlg.m_PixPerDegY;
		sprintf_s(temp, "%1.3f", g_ICANDIParams.PixPerDegY);
		::WritePrivateProfileString("ApplicationInfo", "PixelsperDegY", temp, filename);
		g_Motion_ScalerY = (float)(g_ICANDIParams.VRangePerDegY*128./g_ICANDIParams.PixPerDegY);
	}
//	SetCurrentDirectory(m_strCurrentDir);
}

void CICANDIDoc::OnUpdateSetupDesinusoid(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here	
	pCmdUI->Enable(!g_bFFTIsRunning);
}



void CICANDIDoc::OnCameraConnect() 
{
	// CHECK if there is valid h-sync and v-sync.
	if (g_objVirtex5BMD.DetectSyncSignals(g_hDevVirtex5) == FALSE) {
		AfxMessageBox("No sync signals found, sampling aborted.", MB_ICONEXCLAMATION);
		return;
	}

	QueryPerformanceFrequency(&g_ticksPerSecond);

	m_bCameraConnected = !m_bCameraConnected;

	aoslo_movie.width  = ((CICANDIApp*)AfxGetApp())->m_imageSizeX;
	aoslo_movie.height = ((CICANDIApp*)AfxGetApp())->m_imageSizeY;
	aoslo_movie.width = (aoslo_movie.width%4 == 0)? aoslo_movie.width : (aoslo_movie.width/4+1)*4;
	g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, MOVIE_HEADER_ID);
	if (g_AnimateCtrl == NULL) g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, INIT_ANIMATE_CTRL);

	// If there is a grab in a view, halt the grab before starting a new one
	if(((CICANDIApp*)AfxGetApp())->m_isGrabStarted) {
		((CICANDIApp*)AfxGetApp())->m_isGrabStarted = FALSE;

		//Reset current frame counter
		g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, ID_GRAB_STOP);
	}

	// Update the variable GrabIsStarted
	((CICANDIApp*)AfxGetApp())->m_isGrabStarted = TRUE;	

	g_sampling_counter = 0;


	// testing
	g_EventEOB = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_EOB_EVENT");
	if (!g_EventEOB) {
		AfxMessageBox("Failed to create EOF event sender, sampling aborted.", MB_ICONEXCLAMATION);
		return;
	}
	g_EventEOF = CreateEvent(NULL, FALSE, FALSE, "ICANDI_GUI_EOF_EVENT");
	if (!g_EventEOF) {
		AfxMessageBox("Failed to create EOF event sender, sampling aborted.", MB_ICONEXCLAMATION);
		return;
	}

	thd_handle[0] = CreateThread(NULL, 0, ThreadVideoSampling, this, 0, &thdid_handle[0]);
	SetThreadPriority(thd_handle[0], THREAD_PRIORITY_LOWEST);
	thd_handle[1] = CreateThread(NULL, 0, ThreadSaveVideoHDD, this, 0, &thdid_handle[1]);
	SetThreadPriority(thd_handle[1], THREAD_PRIORITY_LOWEST);	
	thd_handle[2] = CreateThread(NULL, 0, ThreadStablizationFFT, this, 0, &thdid_handle[2]);
	SetThreadPriority(thd_handle[2], THREAD_PRIORITY_TIME_CRITICAL);
}

void CICANDIDoc::OnCameraDisconnect() 
{
	// TODO: Add your command handler code here
	m_bCameraConnected = !m_bCameraConnected;
	g_bFFTIsRunning = FALSE;

	// Halt the grab
	((CICANDIApp*)AfxGetApp())->m_isGrabStarted = FALSE;

	//Reset current frame counter
	g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, ID_GRAB_STOP);
	g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, ID_GRAB_STOP);

	g_sampling_counter = 0;

}

void CICANDIDoc::OnUpdateCameraConnect(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(!m_bCameraConnected && !((CICANDIApp*)AfxGetApp())->m_bInvalidDevice);
}

void CICANDIDoc::OnUpdateCameraDisconnect(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(m_bCameraConnected);
}

void CICANDIDoc::OnStablizeGo() 
{
	g_bFFTIsRunning    = TRUE;
	g_frameIndex       = 0;
	g_BlockIndex       = 0;
	
	g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, STABILIZATION_GO);
}

void CICANDIDoc::OnUpdateStablizeGo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CICANDIApp*)AfxGetApp())->m_isGrabStarted & !g_bFFTIsRunning);
}

void CICANDIDoc::OnStablizeSuspend() 
{
	g_bFFTIsRunning   = FALSE;

	g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, STABILIZATION_STOP);
}

void CICANDIDoc::OnUpdateStablizeSuspend(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CICANDIApp*)AfxGetApp())->m_isGrabStarted & g_bFFTIsRunning);
}

void CICANDIDoc::OnLoadExtRef() 
{
	char     BASED_CODE szFilter[] = "TIF Files (*.tif)|*.tif|";
	CString oldreffname;
	// open an old ref file
	CFileDialog fd(TRUE, "tif", NULL, NULL, szFilter);
	if (fd.DoModal() != IDOK) return;
	
	// get the path and file name of this bmp file
	oldreffname = fd.GetPathName();	
		
	try {
		Mat extRef;
		CT2CA szaviFileA(_T(oldreffname));
		std::string straviFileA(szaviFileA);
		extRef = imread(straviFileA, 0);
		memcpy(aoslo_movie.video_ins, extRef.data, aoslo_movie.height*aoslo_movie.width);			
	}

	catch(...) {
		AfxMessageBox("Unable to load reference image", MB_ICONWARNING);
	}
	return;
}

void CICANDIDoc::OnUpdateLoadExtRef(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CICANDIApp*)AfxGetApp())->m_isGrabStarted && g_bFFTIsRunning);
}

void CICANDIDoc::OnSaveReference() 
{
	saveBitmap();
}

void CICANDIDoc::OnUpdateSaveReference(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CICANDIApp*)AfxGetApp())->m_isGrabStarted && g_bFFTIsRunning && g_StimulusPosBak.x != -1);
}



void CICANDIDoc::OnMovieNormalize() 
{
	char     BASED_CODE szFilter[] = "Uncompressed AVI Files (*.avi)|*.avi|";
	CString  aviFileName;
	int      nframes=0, fWidth, fHeight;	
	PAVISTREAM  pStream;
	CString          errMsg;
	PGETFRAME        pFrame;
	BITMAPINFOHEADER bih;
	BYTE            *imgTemp;
	BYTE			*m_dispBufferRaw;
	double			*m_sumframe;
	double			*m_sumbinary;
	double			*m_binary;
	double			*m_dframe;
	double			maxval = 0.;
	int				frameIdx = 0;
	int				framesize;
	m_dispBufferRaw = NULL;
	m_sumframe = m_sumbinary = m_binary = m_dframe = NULL;

	CFileDialog fd(TRUE,"avi",NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOREADONLYRETURN | OFN_ALLOWMULTISELECT, "AVI files (*.avi)|*.avi||");
	
	fd.m_ofn.lpstrInitialDir  = m_VideoFolder;

	if (fd.DoModal() == IDOK)
	{
		POSITION pos ( fd.GetStartPosition() );
		while( pos )
		{
			// get the path and file name of this .avi file
			aviFileName = fd.GetNextPathName(pos);

			// read .avi file header and the pointer to .avi stream
			pStream = g_GetAviStream(aviFileName, &nframes, &fWidth, &fHeight, NULL, NULL, NULL);
			//close the stream after finishing the task
			if (pStream == NULL || nframes==-1) {
				AVIStreamRelease(pStream);
				AVIFileExit();		
				AfxMessageBox("Error: can't open avi stream", MB_ICONWARNING);
				break;
			}

			//	Normalize_movie
			framesize = fWidth*fHeight;
			pFrame = AVIStreamGetFrameOpen(pStream, NULL);
			// read video stream to video buffer
			// the returned is a packed DIB frame
			if (m_dispBufferRaw == NULL) m_dispBufferRaw = new BYTE[framesize];
			if (m_sumframe == NULL) m_sumframe = new double[framesize];
			ZeroMemory(m_sumframe, framesize*sizeof(double));
			if (m_binary == NULL) m_binary = new double[framesize];
			ZeroMemory(m_binary, framesize*sizeof(double));
			if (m_sumbinary == NULL) m_sumbinary = new double[framesize];
			VD_equ1(m_sumbinary, framesize);
			if (m_dframe == NULL) m_dframe = new double[framesize];
			ZeroMemory(m_dframe, framesize*sizeof(double));
			maxval = 0.;

			for (frameIdx=0; frameIdx<nframes; frameIdx++)
			{		
				imgTemp = (BYTE*) AVIStreamGetFrame(pFrame, frameIdx);
				RtlMoveMemory(&bih.biSize, imgTemp, sizeof(BITMAPINFOHEADER));
				//now get the bitmap bits
				if (bih.biSizeImage < 1) {
					errMsg.Format("Error: can't read frame No. %d/%d.", frameIdx+1, nframes);			
					AfxMessageBox(errMsg, MB_ICONWARNING);
				}
				// get rid of the header information and retrieve the real image 
				RtlMoveMemory(m_dispBufferRaw, imgTemp+sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256, bih.biSizeImage);
				V_UBtoD(m_dframe,m_dispBufferRaw, framesize); 
				VD_addV(m_sumframe, m_sumframe, m_dframe, framesize);
				VD_cmp_gtC(m_binary, m_dframe, framesize, 1.);
				VD_addV(m_sumbinary, m_sumbinary, m_binary, framesize);
			}
			VD_divV(m_sumframe, m_sumframe, m_sumbinary, framesize);
			maxval = VD_max(m_sumframe, framesize);	
			VD_divC(m_sumframe, m_sumframe, framesize, maxval);
			VD_mulC(m_sumframe, m_sumframe, framesize, 255.);
			VD_roundtoUB(m_dispBufferRaw, m_sumframe, framesize);
			MUB_Cols_rev(&m_dispBufferRaw, fWidth, fHeight);	

			AVIStreamGetFrameClose(pFrame);
			// END movie normalization
			AVIStreamRelease(pStream);
			AVIFileExit();
			//	save normalized image as tiff
			Mat			frameA;
			frameA = Mat(fHeight, fWidth, CV_8UC1, m_dispBufferRaw, 0);

			aviFileName.Replace(".avi",".tiff");
			CT2CA szaviFileA(_T(aviFileName));
			std::string straviFileA(szaviFileA);
			std::vector<int> compression_params;
			compression_params.push_back(IMWRITE_PNG_COMPRESSION);
			compression_params.push_back(0);
			try {
				imwrite(straviFileA, frameA, compression_params);
			}
			catch (...) {
				AfxMessageBox("Error: tiff creation error", MB_ICONWARNING);
				break;
			}
			ShellExecute(0, 0, aviFileName, 0, 0 , SW_SHOW );
		}
		if (m_dispBufferRaw != NULL) delete [] m_dispBufferRaw;
		if (m_sumframe != NULL) delete [] m_sumframe;
		if (m_binary != NULL) delete [] m_binary;
		if (m_sumbinary != NULL) delete [] m_sumbinary;
		if (m_dframe != NULL) delete [] m_dframe;
	}
}

void CICANDIDoc::OnUpdateMovieNormalize(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(1);
}

void CICANDIDoc::UpdateImageGrabber()
{
	FILE* fp;
	unsigned short width, height, offset_line, offset_pixel;
	unsigned char  spacing, * adc_regs;
	UINT32   interval;//, reg32, regTemp;
	int      ret;
	BOOL     bFrameByLine, bVedge;

	adc_regs = new unsigned char[48];
	if (adc_regs == NULL) {
		((CICANDIApp*)AfxGetApp())->m_bInvalidDevice = TRUE;
		AfxMessageBox("Failed to allocate memory for Image Grabber Configuration File", MB_ICONEXCLAMATION);
		return;
	}

	fopen_s(&fp, g_ICANDIParams.ImgGrabberDCF, "rb");
	if (fp == NULL) {
		((CICANDIApp*)AfxGetApp())->m_bInvalidDevice = TRUE;
		AfxMessageBox("Failed to open Image Grabber Configuration File", MB_ICONEXCLAMATION);
		return;
	}

	// image width
	ret = fread(&width, sizeof(unsigned short), 1, fp);
	if (ret != 1)
		((CICANDIApp*)AfxGetApp())->m_bInvalidDevice = TRUE;
	else
		g_VideoInfo.img_width = width;
	// image height
	ret = fread(&height, sizeof(unsigned short), 1, fp);
	if (ret != 1) ((CICANDIApp*)AfxGetApp())->m_bInvalidDevice = TRUE;
	g_VideoInfo.img_height = 512;
	// line spacing
	ret = fread(&spacing, sizeof(unsigned char), 1, fp);
	if (ret != 1)
		((CICANDIApp*)AfxGetApp())->m_bInvalidDevice = TRUE;
	else
		g_VideoInfo.line_spacing = spacing;
	// address interval
	ret = fread(&interval, sizeof(UINT32), 1, fp);
	if (ret != 1)
		((CICANDIApp*)AfxGetApp())->m_bInvalidDevice = TRUE;
	else
		g_VideoInfo.addr_interval = interval;
	// offset line
	ret = fread(&offset_line, sizeof(unsigned short), 1, fp);
	if (ret != 1)
		((CICANDIApp*)AfxGetApp())->m_bInvalidDevice = TRUE;
	else
		g_VideoInfo.offset_line = offset_line;
	// offset pixel
	ret = fread(&offset_pixel, sizeof(unsigned short), 1, fp);
	if (ret != 1)
		((CICANDIApp*)AfxGetApp())->m_bInvalidDevice = TRUE;
	else
		g_VideoInfo.offset_pixel = offset_pixel;
	ret = fread(&bFrameByLine, sizeof(BOOL), 1, fp);
	g_objVirtex5BMD.AppSetFrameCounter(g_hDevVirtex5, bFrameByLine);

	ret = fread(&bVedge, sizeof(BOOL), 1, fp);
	g_objVirtex5BMD.AppVsyncTrigEdge(g_hDevVirtex5, bVedge);

	ret = fread(adc_regs, sizeof(unsigned char), 47, fp);

	if (ret != 47) ((CICANDIApp*)AfxGetApp())->m_bInvalidDevice = TRUE;

	fclose(fp);

	if (((CICANDIApp*)AfxGetApp())->m_bInvalidDevice == TRUE) {
		AfxMessageBox("Invalid data in Image Grabber Configuration File", MB_ICONEXCLAMATION);
		return;
	}

	((CICANDIApp*)AfxGetApp())->m_bInvalidDevice = FALSE;

	if (g_VideoInfo.video_in != NULL) delete[] g_VideoInfo.video_in;
	g_VideoInfo.video_in = new unsigned char[g_VideoInfo.img_width * g_VideoInfo.img_height];
	if (g_VideoInfo.video_in == NULL)
		AfxMessageBox("Error! Can't allocate memory space for video.");

	// number of PCIe packets in each transaction
	g_VideoInfo.tlp_counts = g_VideoInfo.img_width * g_VideoInfo.line_spacing / 128 + 1;
	// end line ID of the first image block/stripe
	g_VideoInfo.end_line_ID = g_VideoInfo.line_spacing;

	// update sample information
	g_objVirtex5BMD.AppUpdateSampleInfo(g_hDevVirtex5, g_VideoInfo);

	// update ADC on-chip registers
	g_objVirtex5BMD.UpdateRuntimeRegisters(g_hDevVirtex5, adc_regs);

	// 3. Brightness (0x0B, 0x0C)
	ret = adc_regs[0x0B];
	ret = ret << 8;
	ret = ret + adc_regs[0x0C];
	ret = ret >> 6;
	g_nBrightness = ret;

	// 4. Constrast  (0x05, 0x06)
	ret = adc_regs[0x05];
	ret = ret << 8;
	ret = ret + adc_regs[0x06];
	ret = ret >> 6;
	g_nContrast = ret;

	//g_objVirtex5BMD.WritePupilMask(g_hDevVirtex5, 0, 16, g_VideoInfo.img_width-32-16, g_VideoInfo.img_height-32);
}

BOOL CICANDIDoc::Load14BITbuffer(CString filename, unsigned short **stim_buffer, int *width, int *height)
{
	BOOL status = TRUE;
	double *buffer;
	unsigned short temp;
	CString msg;

	FILE *fp = NULL;
	fopen_s(&fp, filename, "rb");
	if (fp == NULL) {
		AfxMessageBox("Failed to load stimulus file", MB_ICONEXCLAMATION);
		status = FALSE;
	} else {	
		fread(&temp, sizeof(unsigned short), 1, fp);		// width of stimulus pattern becomes height when rotated
		*width = temp;
		fread(&temp, sizeof(unsigned short), 1, fp);		// height of stimulus pattern becomes width when rotated
		*height = temp;
		
		if (*width * *height > 32768) {
			msg.Format("Stimulus Pattern is too big (W-%d,H-%d)", *width, *height);
			AfxMessageBox(msg, MB_ICONEXCLAMATION);
			status = FALSE;
		} else {
			buffer = new double[*width * *height];			
			
			if (fread(buffer, sizeof(double), *width * *height, fp) != *width * *height) {
				AfxMessageBox("Stimulus data does not have correct size.\n", MB_ICONEXCLAMATION);
				status = FALSE;
			} else {
				MD_transpose(&buffer, &buffer, *height, *width);
				VD_mulC(buffer, buffer, *width * *height, 1000.);
				
				if (!*stim_buffer) 
					*stim_buffer = new unsigned short [((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY];
				ZeroMemory(*stim_buffer, ((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY);			
								
				switch (g_ICANDIParams.FRAME_ROTATION)	{
					case 0: //no rotation or flip
						break;
					case 1: //rotate 90 deg
						MD_rotate270(&buffer, &buffer, *width, *height);
						MD_Rows_rev(&buffer, *width, *height);
						temp = *height;
						*height = *width;
						*width= temp;
						break;
					case 2: //flip along Y axis						
						MD_rotate180(&buffer, &buffer, *height, *width);	
						break;
					default:
						;
				}

				VD_choptoUS(*stim_buffer, buffer, *width * *height);
			}			
			delete [] buffer;
		}
	}
	fp != NULL?fclose(fp):0;
	return status;
}

BOOL CICANDIDoc::Load8BITbmp(CString filename, unsigned short**stim_buffer, int *width, int *height, BOOL checksize)
{
	BOOL status = TRUE;
	BYTE     *buffer;
	unsigned short *buf;
	CBitmap   bmp;
	BITMAP    bitmap;
	int       retVal, n, i, k;
	buf = NULL;
	
	// read stimulus pattern from a bitmap file
	HBITMAP hBmp = (HBITMAP)::LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (hBmp == NULL) {
		AfxMessageBox(_T("Open file <") + filename + _T("> error"), MB_ICONEXCLAMATION);
		status = FALSE;
	} else {
		CBitmap *pBmp = bmp.FromHandle(hBmp);
		pBmp->GetBitmap(&bitmap);
		
		if (bitmap.bmHeight*bitmap.bmWidth > 65536 && checksize) { //256x256
			//if (bitmap.bmHeight>361 || bitmap.bmWidth>361) {
			AfxMessageBox("Stimulus Pattern is too big. Set size <= 32768", MB_ICONEXCLAMATION);
			status = FALSE;
		} else {
			if (bitmap.bmHeight<=0 || bitmap.bmWidth<= 0) {
				AfxMessageBox("Invalid stimulus pattern.", MB_ICONEXCLAMATION);
				status = FALSE;
			} else {
				
				buffer = new BYTE [bitmap.bmWidthBytes*bitmap.bmHeight];
				buf = new unsigned short [bitmap.bmWidth*bitmap.bmHeight];
				
				retVal = pBmp->GetBitmapBits(bitmap.bmWidthBytes*bitmap.bmHeight, buffer);	
				
				n = bitmap.bmWidthBytes/bitmap.bmWidth;
				
				for (i = 0; i < bitmap.bmWidth*bitmap.bmHeight; i ++) {
					k = (buffer[n*i]+(n>=2?buffer[n*i+1]:0)+(n>=3?buffer[n*i+2]:0))/(n==4?3:n);
					buf[i] = (unsigned short)((k / 255.) * 1000.);
				}

				switch (g_ICANDIParams.FRAME_ROTATION)	{
					case 0: //no rotation or flip
						*width  = bitmap.bmWidth;
						*height = bitmap.bmHeight;
						break;
					case 1: //rotate 90 deg
						MUS_rotate270(&buf, &buf, bitmap.bmWidth, bitmap.bmHeight);
						MUS_Rows_rev(&buf, bitmap.bmWidth, bitmap.bmHeight);
						*width  = bitmap.bmHeight;
						*height = bitmap.bmWidth;
						break;
					case 2: //flip along Y axis						
						MUS_rotate180(&buf, &buf, bitmap.bmHeight, bitmap.bmWidth);						
						*width  = bitmap.bmWidth;
						*height = bitmap.bmHeight;
						break;
					default:
						;
				}				

				if (!*stim_buffer) 
					*stim_buffer = new unsigned short [((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY];
				ZeroMemory(*stim_buffer, sizeof(unsigned short)*((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY);
				memcpy(*stim_buffer, buf, sizeof(unsigned short)*bitmap.bmWidth*bitmap.bmHeight);
				
				delete [] buffer;
				delete [] buf;
			}
		}
	}
	
	return TRUE;
}

void CICANDIDoc::LoadSymbol(CString filename, unsigned short* stim_buffer, int width, int height)
{
	short temp;

	if (filename && !stim_buffer)
	{	
		filename.MakeLower();
		if (filename.Right(3) == _T("buf")) {
			if (Load14BITbuffer(filename, &stim_buffer, &width, &height) == FALSE) {
				AfxMessageBox("Failed to load stimulus", MB_ICONEXCLAMATION);
				return;
			}
		} else if (filename.Right(3) == _T("bmp")) {
			if (Load8BITbmp(filename, &stim_buffer, &width, &height, TRUE) == FALSE) {
				AfxMessageBox("Failed to load stimulus", MB_ICONEXCLAMATION);
				return;
			}
		} else {
			AfxMessageBox("Unknown file type. No stimulus is loaded", MB_ICONEXCLAMATION);
			return;
		}
		g_ICANDIParams.StimuliPath = filename.Left(filename.ReverseFind('\\'));
	}
	else if (stim_buffer)
	{
		switch (g_ICANDIParams.FRAME_ROTATION)	{
			case 0: //no rotation or flip				
				break;
			case 1: //rotate 90 deg
				MUS_rotate270(&stim_buffer, &stim_buffer, width, height);
				MUS_Rows_rev(&stim_buffer, width, height);
				temp = height;
				height = width;
				width = height;
				break;
			case 2: //flip along Y axis						
				MUS_rotate180(&stim_buffer, &stim_buffer, height, width);	
				break;
			default:
				;
		}
	}

	int i, k, n, m, sizeX, sizeY;
	sizeX = width + 1;					// conserve stimulus width (ND, 20191017)
	sizeY = height;

	aoslo_movie.stim_rd_nx = sizeX;
	aoslo_movie.stim_rd_ny = sizeY;
	g_iStimulusSizeX_RD    = sizeX;
	aoslo_movie.x_center_rd= 0;
	aoslo_movie.x_cc_rd    = 0;
	aoslo_movie.stim_gr_nx = sizeX;
	aoslo_movie.stim_gr_ny = sizeY;
	g_iStimulusSizeX_GR    = sizeX;
	aoslo_movie.x_center_gr= 0;
	aoslo_movie.x_cc_gr    = 0;
	aoslo_movie.stim_ir_nx = sizeX;
	aoslo_movie.stim_ir_ny = sizeY;
	g_iStimulusSizeX_IR    = sizeX;
	aoslo_movie.x_center_ir= 0;
	aoslo_movie.x_cc_ir    = 0;

	ZeroMemory(aoslo_movie.stim_rd_buffer, sizeof(unsigned short)*((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY);
	if (sizeX == width && sizeY == height) {
		memcpy(aoslo_movie.stim_rd_buffer, stim_buffer, sizeof(unsigned short)*sizeX*sizeY);
	} else {
		for (k = 0; k < height; k ++) {
			n = k * sizeX;
			m = k * width;
			for (i = 0; i < width; i ++) {
				aoslo_movie.stim_rd_buffer[n+i] = stim_buffer[m+i];
			}
		}
	}

	if (filename && stim_buffer) 
		delete [] stim_buffer;

	// upload stimulus pattern to FPGA, and saves it in a block RAM
	memcpy(aoslo_movie.stim_gr_buffer, aoslo_movie.stim_rd_buffer, sizeof(unsigned short)*sizeX*sizeY);
	memcpy(aoslo_movie.stim_ir_buffer, aoslo_movie.stim_rd_buffer, sizeof(unsigned short)*sizeX*sizeY);

	// upload stimulus pattern to FPGA, and saves it in a block RAM
	SetEvent(g_EventLoadStim);

	SetEvent(g_EventLoadLUT);

	g_bStimulusOn = TRUE;
	m_bSymbol = true;
}


BOOL CICANDIDoc::LoadStimVideo(CString *filename, int file_num)
{
	int i;

	if (m_strStimVideoName != NULL) delete [] m_strStimVideoName;
	m_strStimVideoName = new CString [file_num];

	if (aoslo_movie.stim_video != NULL) {
		for (i = 0; i < aoslo_movie.nStimVideoNum; i ++) 
			delete [] aoslo_movie.stim_video[i];
		delete [] aoslo_movie.stim_video;
		delete [] aoslo_movie.nStimVideoNX;
		delete [] aoslo_movie.nStimVideoNY;
		delete [] aoslo_movie.nStimVideoPlanes;
	}

	aoslo_movie.stim_video       = new unsigned short *[file_num];
	aoslo_movie.nStimVideoNX     = new int [file_num];
	aoslo_movie.nStimVideoNY     = new int [file_num];
	aoslo_movie.nStimVideoLength = new int [file_num];
	aoslo_movie.nStimVideoPlanes = new int [file_num];

	for (i = 0; i < file_num; i ++) {
		m_strStimVideoName[i] = filename[i];
	}
	aoslo_movie.nStimVideoNum = file_num;
	
	SetEvent(g_EventReadStimVideo);

	g_ICANDIParams.StimuliPath = filename[0].Left(filename[0].ReverseFind('\\'));

	return TRUE;
}

BOOL CICANDIDoc::LoadMultiStimuli_Matlab(int clear, CString folder, CString prefix, short startind, short endind, CString ext)
{
	CString	 *filename, msg, initials;
	char	ind[3];
	int      width, height, sizeX, sizeY;
	int      i, j, k, m, n, offsind;
	unsigned short* stim_buffer;
	BOOL	success = TRUE;
	
	stim_buffer = NULL;
	
	if (ext == ".avi") {
		g_bMatlabVideo = FALSE;
		if (startind == endind)
			filename = new CString[1];
		else
			filename = new CString[endind-startind+1];
		for (i=startind; i<=endind; i++) {
			//generate filename
			msg = folder+prefix;
			if(endind) {
				itoa(i,ind,10);
				initials.Format("%s%s", msg, ind);
				//	strcat(filename.GetBuffer(filename.GetLength()),ind.GetBuffer(0));
				msg = initials;
			}
			filename[i-startind] = msg+ext;
		}						
		g_bMatlabCtrl = TRUE;
		LoadStimVideo(filename, (endind-startind+1));
	}
	
	else {
		g_bMatlabVideo = FALSE;
		stim_buffer = new unsigned short [((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY];
		ZeroMemory(stim_buffer, ((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY*sizeof(unsigned short));
		
		//do we have to clear any previous data in buffer
		if (clear) {
			for (i = 2; i < aoslo_movie.stimuli_num; i ++) 
				delete [] aoslo_movie.stimuli_buf[i];
			offsind = aoslo_movie.stimuli_num = 2;
		}
		else
			offsind = aoslo_movie.stimuli_num;

		filename = new CString[1];
		for (i=startind; i<=endind; i++) {
			//generate filename
			filename[0] = folder+prefix;
			if(endind) {
				itoa(i,ind,10);
				initials.Format("%s%s", filename[0], ind);
				//	strcat(filename.GetBuffer(filename.GetLength()),ind.GetBuffer(0));
				filename[0] = initials;
			}
			filename[0] += ext;
			if (ext == ".buf") {				
				if (Load14BITbuffer(filename[0], &stim_buffer, &width, &height) == FALSE) {					
					msg.Format("Error loading <%s> file. Please check for its validity", filename[0]);
					AfxMessageBox(msg, MB_ICONEXCLAMATION);
					return FALSE;
				}
			}		
			else if(ext == ".bmp") {
				if (Load8BITbmp(filename[0], &stim_buffer, &width, &height, TRUE) == FALSE) {
					msg.Format("Error loading <%s> file. Please check for its validity", filename[0]);
					AfxMessageBox(msg, MB_ICONEXCLAMATION);
					return FALSE;
				}				
			}
			else {
				AfxMessageBox("Unknown file type. No stimulus is loaded", MB_ICONEXCLAMATION);
				return FALSE;
			}
			
			sizeX = width + 1;					// conserve stimulus width (ND, 20191021)
			sizeY = height;
			
			aoslo_movie.stimuli_buf[i-startind+offsind] = new unsigned short[sizeX*sizeY];
			aoslo_movie.stimuli_sx[i-startind+offsind] = sizeX;
			aoslo_movie.stimuli_sy[i-startind+offsind] = sizeY;			

			ZeroMemory(aoslo_movie.stimuli_buf[i-startind+offsind], sizeof(unsigned short)*(sizeX*sizeY));
		
			if (sizeX == width && sizeY == height) 
				memcpy(aoslo_movie.stimuli_buf[i-startind+offsind], stim_buffer, sizeof(unsigned short)*(sizeX*sizeY));
			else {
				for (k = 0; k < height; k ++) {
					n = k * sizeX;
					m = k * width;
					for (j = 0; j < width; j ++) 
						aoslo_movie.stimuli_buf[i-startind+offsind][n+j] = stim_buffer[m+j];
				}
			}
			aoslo_movie.stimuli_num++;
		}
		delete [] stim_buffer;
	}

	return TRUE;
}

//
// added on 6-15-2009
// this routine is used to test multiple stimuli delivery
//

void CICANDIDoc::LoadMultiStimuli()
{
	CString  ini_name, stim_name, msg;
	int      stim_num, width, height, sizeX, sizeY;
	int      i, j, k, m, n;
	char     temp[80], stim_field[80];
	unsigned short	*stim_buffer;

	g_bMultiStimuli = FALSE;

	ini_name = g_ICANDIParams.m_strConfigPath + _T("\\AppParams.ini");

	::GetPrivateProfileString("StimPatterns", "StimNum", "", temp, 80, ini_name);
	stim_num = atoi(temp);
	if (stim_num <= 0) {
		AfxMessageBox("No stimulus pattern is found. Please check setup of the file AppParams.ini", MB_ICONEXCLAMATION);
		return;
	}

	if (aoslo_movie.stimuli_num > 0) {
		for (i = 0; i < aoslo_movie.stimuli_num; i ++) 
			delete [] aoslo_movie.stimuli_buf[i];
	//	delete [] aoslo_movie.stimuli_buf;
		delete [] aoslo_movie.stimuli_sx;
		delete [] aoslo_movie.stimuli_sy;
	}
/*	aoslo_movie.stimuli_sx  = new int [stim_num];
	aoslo_movie.stimuli_sy  = new int [stim_num];
	aoslo_movie.stimuli_buf = new unsigned short *[stim_num];
	for (i = 0; i < stim_num; i ++) 
		aoslo_movie.stimuli_buf[i] = new unsigned short [((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY];*/
	
	stim_buffer = new unsigned short [((CICANDIApp*)AfxGetApp())->m_imageSizeX*((CICANDIApp*)AfxGetApp())->m_imageSizeY];

	aoslo_movie.stimuli_num = stim_num;

	for (i = 0; i < stim_num; i ++) {
		sprintf(stim_field, "StimFile%02d", i+1);
		::GetPrivateProfileString("StimPatterns", stim_field, "", temp, 80, ini_name);
		stim_name.Format("%s", temp);
		stim_name.TrimLeft();
		stim_name.TrimRight();
		if (stim_name.GetLength() == 0) {			
			msg.Format("File No. %d <%s> is invalid. Please check setup of the file AppParams.ini", i+1, temp);
			AfxMessageBox(msg, MB_ICONEXCLAMATION);
			return;
		}
		if (Load8BITbmp(stim_name, &stim_buffer, &width, &height, TRUE) == FALSE) {
			msg.Format("Unable to load file <%s>. Please check for validity", stim_name);
			AfxMessageBox(msg, MB_ICONEXCLAMATION);			
			return;
		}

		sizeX = width + 1;					// conserve stimulus width (ND, 20191021)
		sizeY = height;

		aoslo_movie.stimuli_sx[i] = sizeX;
		aoslo_movie.stimuli_sy[i] = sizeY;
		aoslo_movie.stimuli_buf[i] = new unsigned short [sizeX*sizeY];
		ZeroMemory(aoslo_movie.stimuli_buf[i], sizeof(unsigned short)*(sizeX*sizeY));

		for (k = 0; k < height; k ++) {
			n = k * sizeX;
			m = k * width;
			for (j = 0; j < width; j ++) {
				aoslo_movie.stimuli_buf[i][n+j] = (unsigned short)(stim_buffer[m+j]);
			}
		}
	}

	delete [] stim_buffer;
	g_bMultiStimuli = TRUE;
}



BOOL CICANDIDoc::LoadMaskTraces(CString filename)
{
	char     line[150];
	int      totaltrials;

	
	FILE* fp;
	fp = fopen(filename, "r");
	if (!fp)
	{
		AfxMessageBox("Unable to open sequence file for pupil mask", MB_ICONWARNING);
		return FALSE;      
	}

	totaltrials = 0;
	while (!feof(fp)) {
		fgets(line, 150, fp);
		totaltrials++;
	} 
	fclose(fp);

	//
	// load mask data below
	//
	return TRUE;
}

void CICANDIDoc::PlaybackMovie(LPCTSTR filename) {
	m_bPlayback == TRUE ? StopPlayback():0;
	g_bPlaybackUpdate = TRUE;
	g_bFFTIsRunning == TRUE ? OnStablizeSuspend():0;
	if (g_AnimateCtrl == NULL)
		g_viewRawVideo->SendMessage(WM_MOVIE_SEND, 0, INIT_ANIMATE_CTRL);
	g_AnimateCtrl->Open(filename);
	g_AnimateCtrl->Play(0,-1,-1);
	g_AnimateCtrl->ShowWindow(TRUE);
	m_bPlayback = TRUE;

}

void CICANDIDoc::StopPlayback() {
	if (g_AnimateCtrl != NULL && m_bPlayback == TRUE)
	{
		m_bPlayback = FALSE;
		g_AnimateCtrl->Stop();
		g_AnimateCtrl->Close();
		g_AnimateCtrl->ShowWindow(FALSE);
		g_bPlaybackUpdate = FALSE;
		g_viewMsgVideo->SendMessage(WM_MOVIE_SEND, 0, STABILIZATION_GO);
	}
}

void CICANDIDoc::DrawStimulus()
{
	int         i, x, y, x0, x_ir, x_gr, x_rd, x0ir, x0gr, x0rd;
	int         x1ir, x2ir, x1rd, x2rd, x1gr, x2gr, power_rd, power_gr, nw1, nw2;
	int         rdShiftY, grShiftY;
	float       cent, w1, w2;

	// first load desinusoiding LUT
	if (g_ICANDIParams.Desinusoid == 1) {
		if (g_PatchParamsA.UnMatrixIdx == NULL && g_PatchParamsA.UnMatrixVal == NULL && g_PatchParamsA.StretchFactor == NULL) { 
			g_PatchParamsA.UnMatrixIdx = new int   [(aoslo_movie.width+aoslo_movie.height)*2];
			g_PatchParamsA.UnMatrixVal = new float [(aoslo_movie.width+aoslo_movie.height)*2];
			g_PatchParamsA.StretchFactor = new float [aoslo_movie.width];
			if (g_ReadLUT(aoslo_movie.width, aoslo_movie.height, g_PatchParamsA.UnMatrixIdx, g_PatchParamsA.UnMatrixVal, g_PatchParamsA.StretchFactor) == FALSE) {
				delete [] g_PatchParamsA.UnMatrixIdx;
				delete [] g_PatchParamsA.UnMatrixVal;
				delete [] g_PatchParamsA.StretchFactor;
				g_PatchParamsA.UnMatrixIdx = NULL;
				g_PatchParamsA.UnMatrixVal = NULL;
				g_PatchParamsA.StretchFactor = NULL;
				g_ICANDIParams.Desinusoid = 0;
			}
		}
	}

	x = g_StimulusPosBak.x - dewarp_x;		// position X on the reference frame
	y = g_StimulusPosBak.y - dewarp_y;		// position Y on the reference frame

	// -------------------------------------------------
	// we now add stimulus delivery here
	// -------------------------------------------------
	power_rd = 4*aoslo_movie.nLaserPowerRed;
	power_gr = 4*aoslo_movie.nLaserPowerGreen;
	x1rd = x2rd = aoslo_movie.stim_rd_nx/2;
	x1gr = x2gr = aoslo_movie.stim_gr_nx/2;
	x1ir = x2ir = aoslo_movie.stim_ir_nx/2;
	//g_ICANDIParams.Desinusoid = 1;
	if (g_ICANDIParams.Desinusoid == 1) {
		x0 = x;
		x0ir = x;

//		x0gr = x+g_Channel2Shift.x;
//		x0rd = x+g_Channel1Shift.x;
		if (g_ICANDIParams.FRAME_ROTATION) {
			x0gr = x-g_Channel2Shift.y-g_ICANDIParams.RGBClkShifts[1].y;
			x0rd = x-g_Channel1Shift.y-g_ICANDIParams.RGBClkShifts[0].y;
		} else {
			x0gr = x+g_Channel2Shift.x+g_ICANDIParams.RGBClkShifts[1].x;
			x0rd = x+g_Channel1Shift.x+g_ICANDIParams.RGBClkShifts[0].x;
		}

		for (i = 0; i < aoslo_movie.width; i ++) {
			if (x0 < g_PatchParamsA.StretchFactor[i]) break;
		}
		x = i;

		// presinusoid red channel stimulus
		aoslo_movie.x_cc_rd = x0rd;
		for (i = 0; i < aoslo_movie.width; i ++) {
			if (x0rd < g_PatchParamsA.StretchFactor[i]) break;
		}
		x_rd = i;

		x1rd = x0rd-aoslo_movie.stim_rd_nx/2;
		if (x1rd >= 0) {
			for (i = 0; i < aoslo_movie.width; i ++) {
				if (x1rd < g_PatchParamsA.StretchFactor[i]) break;
			}
			x1rd = x_rd - i;
		} else {
			x1rd = x_rd;
		}
		x2rd = x0rd+aoslo_movie.stim_rd_nx/2;
		if (x2rd < aoslo_movie.width) {
			for (i = 0; i < aoslo_movie.width; i ++) {
				if (x2rd < g_PatchParamsA.StretchFactor[i]) break;
			}
			x2rd = i - x_rd - 1;
		} else {
			x2rd = aoslo_movie.width-1-x_rd;
		}

		// calculate mulplication of pixel weights and laser power
		for (i = x_rd-x1rd; i <= x_rd+x2rd; i ++) {
			cent = g_PatchParamsA.StretchFactor[i];
			w1 = ceil(cent)-cent;
			w2 = cent-floor(cent);
			nw1 = (int)(w1*power_rd);
			nw2 = (int)(w2*power_rd);
			aoslo_movie.weightsRed[i-x_rd+x1rd] = (nw1 << 16) | nw2;
		}

/*
		aoslo_movie.x_center_rd= x_rd;
		aoslo_movie.x_left_rd  = x1rd;
		aoslo_movie.x_right_rd = x2rd;
		g_iStimulusSizeX_RD    = x2rd+x1rd+1;
*/

		// presinusoid green channel stimulus
		aoslo_movie.x_cc_gr = x0gr;
		for (i = 0; i < aoslo_movie.width; i ++) {
			if (x0gr < g_PatchParamsA.StretchFactor[i]) break;
		}
		x_gr = i;

		x1gr = x0gr-aoslo_movie.stim_gr_nx/2;
		if (x1gr >= 0) {
			for (i = 0; i < aoslo_movie.width; i ++) {
				if (x1gr < g_PatchParamsA.StretchFactor[i]) break;
			}
			x1gr = x_gr - i;
		} else {
			x1gr = x_gr;
		}
		x2gr = x0gr+aoslo_movie.stim_gr_nx/2;
		if (x2gr < aoslo_movie.width) {
			for (i = 0; i < aoslo_movie.width; i ++) {
				if (x2gr < g_PatchParamsA.StretchFactor[i]) break;
			}
			x2gr = i - x_gr - 1;
		} else {
			x2gr = aoslo_movie.width-1-x_gr;
		}

		// calculate mulplication of pixel weights and laser power
		for (i = x_gr-x1gr; i <= x_gr+x2gr; i ++) {
			cent = g_PatchParamsA.StretchFactor[i];
			w1 = ceil(cent)-cent;
			w2 = cent-floor(cent);
			nw1 = (int)(w1*power_gr);
			nw2 = (int)(w2*power_gr);
			aoslo_movie.weightsGreen[i-x_gr+x1gr]  = (nw1 << 16) | nw2;
		}
/*
		aoslo_movie.x_center_gr= x_gr;
		aoslo_movie.x_left_gr  = x1gr;
		aoslo_movie.x_right_gr = x2gr;
		g_iStimulusSizeX_GR    = x2gr+x1gr+1;
*/

		// presinusoid IR channel stimulus
		aoslo_movie.x_cc_ir = x0ir;
		for (i = 0; i < aoslo_movie.width; i ++) {
			if (x0ir < g_PatchParamsA.StretchFactor[i]) break;
		}
		x_ir = i;

		x1ir = x0ir-aoslo_movie.stim_ir_nx/2;
		if (x1ir >= 0) {
			for (i = 0; i < aoslo_movie.width; i ++) {
				if (x1ir < g_PatchParamsA.StretchFactor[i]) break;
			}
			x1ir = x_ir - i;
		} else {
			x1ir = x_ir;
		}
		x2ir = x0ir+aoslo_movie.stim_ir_nx/2;
		if (x2ir < aoslo_movie.width) {
			for (i = 0; i < aoslo_movie.width; i ++) {
				if (x2ir < g_PatchParamsA.StretchFactor[i]) break;
			}
			x2ir = i - x_ir - 1;
		} else {
			x2ir = aoslo_movie.width-1-x_ir;
		}
/*
		// calculate mulplication of pixel weights and laser power
		for (i = x_gr-x1gr; i <= x_gr+x2gr; i ++) {
			cent = g_PatchParamsA.StretchFactor[i];
			w1 = ceil(cent)-cent;
			w2 = cent-floor(cent);
			nw1 = (int)(w1*power_gr);
			nw2 = (int)(w2*power_gr);
			aoslo_movie.weightsGreen[i-x_gr+x1gr]  = (nw1 << 16) | nw2;
		}
*/
		/*
		aoslo_movie.x_center_ir= x_ir;
		aoslo_movie.x_left_ir  = x1ir;
		aoslo_movie.x_right_ir = x2ir;
		g_iStimulusSizeX_IR    = x2ir+x1ir+1;
*/
	} else {
		x_ir = x;

//		x0gr = x+g_Channel2Shift.x;
//		x0rd = x+g_Channel1Shift.x;
		if (g_ICANDIParams.FRAME_ROTATION) {
			x0gr = x-g_Channel2Shift.y-g_ICANDIParams.RGBClkShifts[1].y;
			x0rd = x-g_Channel1Shift.y-g_ICANDIParams.RGBClkShifts[0].y;
		} else {
			x0gr = x+g_Channel2Shift.x+g_ICANDIParams.RGBClkShifts[1].x;
			x0rd = x+g_Channel1Shift.x+g_ICANDIParams.RGBClkShifts[0].x;
		}

		aoslo_movie.x_cc_gr = x0gr;
		x_gr = x0gr;
		x1gr = aoslo_movie.stim_gr_nx/2;
		x2gr = aoslo_movie.stim_gr_nx/2-1;

		aoslo_movie.x_cc_rd = x0rd;
		x_rd = x0rd;
		x1rd = aoslo_movie.stim_rd_nx/2;
		x2rd = aoslo_movie.stim_rd_nx/2-1;

		x0ir = x;
		aoslo_movie.x_cc_ir = x0ir;
		x_ir = x0ir;
		x1ir = aoslo_movie.stim_ir_nx/2;
		x2ir = aoslo_movie.stim_ir_nx/2-1;


		for (i = 0; i < g_iStimulusSizeX_RD; i ++) {
			nw1 = power_rd >> 1;
			nw2 = nw1 << 16;
			aoslo_movie.weightsRed[i] = nw1 | nw2;
		}
		for (i = 0; i < g_iStimulusSizeX_GR; i ++) {
			nw1 = power_gr >> 1;
			nw2 = nw1 << 16;
			aoslo_movie.weightsGreen[i]  = nw1 | nw2;
		}
	}

	aoslo_movie.x_center_gr= x_gr;
	aoslo_movie.x_left_gr  = x1gr;
	aoslo_movie.x_right_gr = x2gr;
	g_iStimulusSizeX_GR    = x2gr+x1gr+1;

	aoslo_movie.x_center_rd= x_rd;
	aoslo_movie.x_left_rd  = x1rd;
	aoslo_movie.x_right_rd = x2rd;
	g_iStimulusSizeX_RD    = x2rd+x1rd+1;

	aoslo_movie.x_center_ir= x_ir;
	aoslo_movie.x_left_ir  = x1ir;
	aoslo_movie.x_right_ir = x2ir;
	g_iStimulusSizeX_IR    = x2ir+x1ir+1;

	if (g_ICANDIParams.FRAME_ROTATION) {
		rdShiftY = -g_ICANDIParams.RGBClkShifts[0].x - g_Channel1Shift.x;
		grShiftY = -g_ICANDIParams.RGBClkShifts[1].x - g_Channel2Shift.x;
	} else {
		rdShiftY = -g_ICANDIParams.RGBClkShifts[0].y + g_Channel1Shift.y;
		grShiftY = -g_ICANDIParams.RGBClkShifts[1].y + g_Channel2Shift.y;
	}

	g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, 0, aoslo_movie.width, 0, 0, 0, -1, -100);
	g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, false, 0, 0, 0, 0, 0, 0, 0, aoslo_movie.width, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	if (aoslo_movie.stim_gr_nx==aoslo_movie.stim_rd_nx && aoslo_movie.stim_gr_ny==aoslo_movie.stim_rd_ny &&
		aoslo_movie.stim_ir_nx==aoslo_movie.stim_rd_nx && aoslo_movie.stim_ir_ny==aoslo_movie.stim_rd_ny) {
		g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, y-aoslo_movie.stim_rd_ny/2, y+aoslo_movie.stim_rd_ny/2, rdShiftY, grShiftY, aoslo_movie.stim_rd_nx, -1, -100);
		g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, true, x_ir, x_ir, x_gr, x_gr, x_rd, x_rd, y-aoslo_movie.stim_rd_ny/2, y+aoslo_movie.stim_rd_ny/2, 0, rdShiftY, grShiftY, g_ICANDIParams.Desinusoid, x1ir, x2ir, x1gr, x2gr, x1rd, x2rd);
	} else {
		g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, y, rdShiftY, grShiftY, aoslo_movie.stim_ir_nx, aoslo_movie.stim_ir_ny, aoslo_movie.stim_gr_nx, aoslo_movie.stim_gr_ny, aoslo_movie.stim_rd_nx, aoslo_movie.stim_rd_ny);
		g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, true, x_ir, y, x_gr, y+grShiftY, x_rd, y+rdShiftY, aoslo_movie.stim_ir_ny, aoslo_movie.stim_rd_ny, aoslo_movie.stim_gr_ny, g_ICANDIParams.Desinusoid, x1ir, x2ir, x1rd, x2rd, x1gr, x2gr);
	}

	fprintf(g_fp, "no stable: (%d,%d,%d,%d)(%d,%d,%d,%d)(%d,%d,%d,%d)\n", x_ir, x1ir, x2ir, y, x_rd, x1rd, x2rd, y+rdShiftY, x_gr, x1gr, x2gr, y+grShiftY);

	SetEvent(g_EventLoadLUT);
}

int CICANDIDoc::saveBitmap()
{	
	CString oldreffname, oldreffname_bk;
	//aoslo_movie.video_ref_bk;

		// open an old ref file
	char     BASED_CODE szFilter[] = "TIF Files (*.tif)|*.tif|";
	CString newreffname;
	CFileDialog fd(FALSE, "tif", NULL, NULL, szFilter);
	if (fd.DoModal() != IDOK) return FALSE;
		
	// get the path and file name of this bmp file
	newreffname = fd.GetPathName();		
	newreffname = newreffname.Left(newreffname.GetLength()-(newreffname.GetLength()-newreffname.ReverseFind('\\'))+1);
	CString fname_ext, fname;
	fname = fd.GetFileTitle();
	switch (g_ICANDIParams.FRAME_ROTATION)	{
		case 0: //no rotation or flip	
			fname_ext.Format("%s_%d_%d.tif", fname, g_StimulusPosBak.x-dewarp_x-1, g_StimulusPosBak.y-1-dewarp_y);
			break;
		case 1: //rotate 90 deg
			fname_ext.Format("%s_%d_%d.tif", fname, aoslo_movie.dewarp_sy-g_StimulusPosBak.y-1-dewarp_y, g_StimulusPosBak.x-dewarp_x);
		/*	i = point.x;
			point.x = point.y;
			point.y = aoslo_movie.dewarp_sx-i-1;*/
			break;
		case 2: //flip along Y axis
			fname_ext.Format("%s_%d_%d.tif", fname, aoslo_movie.dewarp_sx-g_StimulusPosBak.x-dewarp_x-1, g_StimulusPosBak.y-dewarp_y);
			break;
		default:
			;
	}
		

	newreffname += fname_ext;	

	Mat			frameA;
	frameA = Mat(aoslo_movie.height, aoslo_movie.width, CV_8UC1, aoslo_movie.video_ref_bk, 0);
	
	CT2CA szaviFileA(_T(newreffname));
	std::string straviFileA(szaviFileA);
	std::vector<int> compression_params;
    compression_params.push_back(IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(0);
	try {
        imwrite(straviFileA, frameA, compression_params);
    }
    catch (...) {
        AfxMessageBox("Error: tiff creation error", MB_ICONWARNING);
        return FALSE;
    }
	return TRUE;
}

void CICANDIDoc::Load_Default_Stimulus(bool inv)
{
	// this is default stimulus pattern	
	BOOL g_bStimulusOn_bk = g_bStimulusOn;
	g_bStimulusOn = false;
	unsigned short *stim_buffer;
	int i;
	aoslo_movie.stim_rd_nx      = STIMULUS_SIZE_X;
	aoslo_movie.stim_rd_ny      = STIMULUS_SIZE_Y;
	aoslo_movie.stim_gr_nx      = STIMULUS_SIZE_X;
	aoslo_movie.stim_gr_ny      = STIMULUS_SIZE_Y;
	aoslo_movie.stim_ir_nx      = STIMULUS_SIZE_X;
	aoslo_movie.stim_ir_ny      = STIMULUS_SIZE_Y;
	stim_buffer = new unsigned short [aoslo_movie.stim_rd_nx*aoslo_movie.stim_rd_ny];
	for (i = 0; i < aoslo_movie.stim_rd_nx*aoslo_movie.stim_rd_ny; i ++) stim_buffer[i] = inv? 0x0 : 0x3fff;
	g_objVirtex5BMD.AppLoadStimulus(g_hDevVirtex5, stim_buffer, aoslo_movie.stim_rd_nx, aoslo_movie.stim_rd_ny, 1);
	memcpy(aoslo_movie.stim_rd_buffer, stim_buffer, aoslo_movie.stim_rd_nx*aoslo_movie.stim_rd_ny*sizeof(short));
	for (i = 0; i < aoslo_movie.stim_rd_nx*aoslo_movie.stim_rd_ny; i ++) stim_buffer[i] = inv? 0x0 : 0x3fff;
	g_objVirtex5BMD.AppLoadStimulus(g_hDevVirtex5, stim_buffer, aoslo_movie.stim_rd_nx, aoslo_movie.stim_rd_ny, 2);
	memcpy(aoslo_movie.stim_gr_buffer, stim_buffer, aoslo_movie.stim_gr_nx*aoslo_movie.stim_gr_ny*sizeof(short));	
	for (i = 0; i < aoslo_movie.stim_ir_nx*aoslo_movie.stim_ir_ny; i ++) stim_buffer[i] = inv? 0x3fff : 0x0;
	g_objVirtex5BMD.AppLoadStimulus(g_hDevVirtex5, stim_buffer, aoslo_movie.stim_rd_nx, aoslo_movie.stim_rd_ny, 3);
	memcpy(aoslo_movie.stim_ir_buffer, stim_buffer, aoslo_movie.stim_ir_nx*aoslo_movie.stim_ir_ny*sizeof(short));
	delete [] stim_buffer;

	g_iStimulusSizeX_RD    = aoslo_movie.stim_rd_nx;
	g_iStimulusSizeX_GR    = aoslo_movie.stim_rd_nx;
	g_iStimulusSizeX_IR    = aoslo_movie.stim_rd_nx;

	g_bStimulusOn = g_bStimulusOn_bk;
}

bool CICANDIDoc::SendNetMessage(CString message){

	if (m_ncListener_IGUIDE->IsConnected())
	{	
		char data[256];
		sprintf(data, "%s", message);
		int res = m_ncListener_IGUIDE->Send(data, 256, 0);
		if (res > 0)
			return true;
	}

	return false;

}