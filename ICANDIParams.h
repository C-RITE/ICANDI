#include <iostream>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
//#include <stdio.h>
#include "windows.h"
#include <bitset>
#include <IO.h>

using std::bitset;


// ========== define message handler for windows GUI ==============
#define WM_MOVIE_SEND			WM_USER+1
#define ID_GRAB_STOP           -101
#define SEND_MOVIE_DONE        -102
#define MOVIE_HEADER_ID        -103
#define SENDING_MOVIE          -104
#define MSC_ON_IDLE            -105
#define SEARCHING_REF_FRAME    -106
#define SEND_BLOCK_ID          -107
#define STABILIZATION_GO       -108
#define STABILIZATION_STOP     -109
#define INIT_ANIMATE_CTRL      -110
#define UPDATE_AOMS_STATE	   -111
#define UPDATE_PROCESSOR	   -112
#define UPDATE_LASERCALIB_ON   -113
#define UPDATE_LASERCALIB_OFF   -114
//#define CLEAR_STIMULUS_POINT    127
#define DELIVERY_MODE_FLAG      276
#define SAVE_VIDEO_FLAG         277
#define RESET_REF_FRAME         278
#define RANDOM_DELIVERY_FLAG    279
#define RANDOM_DELIVERY_DONE    280
#define UPDATE_LASERS			281
#define UPDATE_STIM_POS		    282
#define UPDATE_STIM_POS_MATLAB  283
#define UPDATE_USER_TCA			284

#define HORIZONTAL_FREQ			13900 //15785
#define GRABBER_FRAME_RATE      int(HORIZONTAL_FREQ/512)  //was 30	// define frame rate of image grabber
#define MAX_STIMULUS_NUMBER     99									// define frame rate of image grabber
#define MEMORY_POOL_LENGTH      int(HORIZONTAL_FREQ/512)  //was 30	// length of each memory pool, in frame number

// #define GRABBER_FRAME_RATE      30			// define frame rate of image grabber
// #define MAX_STIMULUS_NUMBER     99			// define frame rate of image grabber
// #define MEMORY_POOL_LENGTH      30			// length of each memory pool, in frame number

extern int			g_InvertMotionTraces;

class AOSLO_MOVIE
{
public:
	int     width;
	int     height;
	BYTE   *video_in;
	BYTE   *video_out;
	int     dewarp_sx;
	int     dewarp_sy;
//	short  *deltaX;
//	short  *deltaY;

	float   StimulusDeltaX;
	float   StimulusDeltaY;
	float   StimulusDeltaXOld;
	float   StimulusDeltaYOld;
	BOOL    bOneFrameDelay;
	BOOL	bIRAomOn;
	BOOL	bRedAomOn;
	BOOL	bGrAomOn;

	float   StimulusResX;
	float   StimulusResY;
	int     CenterX;
	int     CenterY;
	unsigned short    TCAboxWidth;
	unsigned short    TCAboxHeight;

	BOOL    no_dewarp_video;
	BOOL    stimulus_flag;		// flag to indicate that stimulus has been sent to FPGA	
	BOOL	WriteMarkFlag;		// flag to indicate whether to draw digital marks on both frames (user selectable)
	BOOL	stimulus_loc_flag;	// flag to indicate dynamic mark on both frames for stimulus location
	BOOL	stimulus_audio_flag; //flag to indicate presentation of stimulus via audio (user selectable)	

	BYTE   *video_saveA1;		// storing raw video
	BYTE   *video_saveA2;		// storing raw video
	BYTE   *video_saveA3;		// storing raw video
	BYTE   *video_saveB1;		// storing stabilized video
	BYTE   *video_saveB2;		// storing stabilized video
	BYTE   *video_saveB3;		// storing stabilized video
	BYTE    memory_pool_ID;
	BOOL    avi_handle_on_A;
	BOOL    avi_handle_on_B;

	BYTE   *video_ref_g;
	BYTE   *video_ref_l;
	BYTE   *video_sparse;
	BYTE   *video_ins;			// because stabilization is not synchronized with stimulus predication,
								// we need another array to hold the currently sampled frame which will 
								// used for stabilization.
	BYTE   *video_ref_bk;
	
	unsigned short  *stim_rd_buffer;		// to hold RED stimulus pattern
	int     stim_rd_nx;			// stimulus width
	int     stim_rd_ny;			// stimulus height
	int     x_center_rd;
	int     x_cc_rd;
	int     x_left_rd;
	int     x_right_rd;

	unsigned short  *stim_gr_buffer;		// to hold Green stimulus pattern
	int     stim_gr_nx;			// stimulus width
	int     stim_gr_ny;			// stimulus height
	int     x_center_gr;
	int     x_cc_gr;
	int     x_left_gr;
	int     x_right_gr;

	unsigned short  *stim_ir_buffer;		// to hold IR stimulus pattern
	int     stim_ir_nx;			// stimulus width
	int     stim_ir_ny;			// stimulus height
	int     x_center_ir;
	int     x_cc_ir;
	int     x_left_ir;
	int     x_right_ir;

	unsigned short **stimuli_buf;		// to hold multiple stimulus pattern
	unsigned short    *stimuli_sx;			// with of muttiple stimuli
	unsigned short    *stimuli_sy;			// height of multiple stimuli
	int     stimuli_num;		// number of stimulus
	int    *stim_loc_y;			// patch center of stimuli
	int     stim_num_y;			// number of critical patches when doing interstimulus
	int    *stim_loc_yir;			// patch center of stimuli
	int     stim_num_yir;			// number of critical patches when doing interstimulus
	int    *stim_loc_yrd;			// patch center of stimuli
	int     stim_num_yrd;			// number of critical patches when doing interstimulus
	int    *stim_loc_ygr;			// patch center of stimuli
	int     stim_num_ygr;			// number of critical patches when doing interstimulus
	unsigned short **stim_video;			// hold stimulus video
	BOOL    bWithStimVideo;		// flag with stimulus video
	int     nStimFrameIdx;
	int     nStimFrameNum;
	int    *nStimVideoNX;
	int    *nStimVideoNY;
	int	   *nStimVideoLength;	
	int	   *nStimVideoPlanes;
	int	    nStimVideoNum;
	int	    nStimVideoIdx;
	BOOL    bStimRewind;
	float   fStabGainStim;
	float   fStabGainMask;
	int     nStimHeight;
	int     nLaserPowerRed;
	int     nLaserPowerGreen;
	int     nLaserPowerIR;
	UINT32 *weightsRed;
	UINT32 *weightsGreen;

	int     sampPoints;

	BOOL    bPupilMask;

	// parameters for delivering stimuli	
	int     nRotateLocation;
	int     PredictionTime;
	int     FlashFrequency;
	int     FlashDutyCycle;
	int     DeliveryMode;
	int     FlashDist;
	BOOL    FlashOnFlag;
	BOOL    RandDelivering;	
	POINT   StimulusPos[MAX_STIMULUS_NUMBER];
	int     StimulusNum;
	int     RandPathIndex[MAX_STIMULUS_NUMBER];
	int    *RandPathIndice;
	int     FrameCountOffset;
	FILE   *fpTesting;

	//parameters for saving sequence 
};

class ICANDIParams
{
public:
	CString	m_strConfigPath;
	int    FRAME_ROTATION; // flag to indicate whether the aoslo frame needs to be rotated for fundus orientation
	int    PATCH_CNT;	// patch counts
	int    PATCH_CNT_OLD;
	int   *patchWidth;	// patch width
	int   *patchHeight;	// patch height
	int    F_XDIM;		// image width
	int    F_YDIM;		// image height
	int   *L_XDIM;
	int   *L_YDIM;
	int   *L_YDIM0;		// start position of L_YDIM
	int   *X_OFFSET;
	int   *Y_OFFSET;
	int    TOT_L_CT;
	int    L_CT;
	int   *FRA_X_OFFSET;
	int   *FRA_Y_OFFSET;
	int    ITERFINAL;
	int    K1;
	int    GTHRESH;
	int    SRCTHRESH;
	int    Desinusoid;		// 1: do desinusoid, 0: don't do it
	float  PixPerDegX;
	float  PixPerDegY;
	float  VRangePerDegX;
	float  VRangePerDegY;
	float  MotionAngleX;
	float  MotionAngleY;
	BOOL   Imaging840;
	CString   fnameLUT;
	char   ImgGrabberDCF[120];	
	BOOL   WriteMarkFlags[9];
	CString VideoFolderPath;
	CString	VideoFolderSuffix;
	CString   StimuliPath;
//	int	   RedShiftY;
//	int	   GreenShiftX;
//	int	   GreenShiftY;
	BOOL   ApplyTCA;
	POINT  RGBClkShifts[3];
	CString waveLength[12];
	
	int    Filter;		// apply filter on raw video for FFT

public:
	void LoadConfigFile()
	{
		char     temp[80];
		CString  filename;
		int      patch_count;
		int		 flags;

		filename = _T("\\AppParams.ini");
		filename = m_strConfigPath + filename;

		// Application Info
		// Grabber XGF
		::GetPrivateProfileString("ApplicationInfo", "GrabberDCF", "", temp, 80, filename);
		strcpy_s(ImgGrabberDCF, temp);
		// Stimuli Path
		::GetPrivateProfileString("ApplicationInfo", "StimuliPath", "", temp, 80, filename);
		StimuliPath.Format("%s", temp);
		if (GetFileAttributes(StimuliPath) == INVALID_FILE_ATTRIBUTES) // directory is not present, load default
			StimuliPath = m_strConfigPath + _T("\\symbols");
		// Video Folder Path
		::GetPrivateProfileString("ApplicationInfo", "VideoFolderPath", "", temp, 80, filename);
		VideoFolderPath.Format("%s", temp);
		if (GetFileAttributes(VideoFolderPath) == INVALID_FILE_ATTRIBUTES) // directory is not present, load default
			VideoFolderPath = _T("D:\\Video_Folder\\");
		else
			VideoFolderPath = VideoFolderPath + _T("\\");
		// Video Folder Path Suffix (for TSLO / AOSLO distinction)
		::GetPrivateProfileString("ApplicationInfo", "VideoFolderSuffix", "", temp, 80, filename);
		VideoFolderSuffix.Format("%s", temp);
		// PixPerDegX
		::GetPrivateProfileString("ApplicationInfo", "PixelsperDegX", "", temp, 40, filename);
		PixPerDegX = (float)atof(temp);
		// PixPerDegY
		::GetPrivateProfileString("ApplicationInfo", "PixelsperDegY", "", temp, 40, filename);
		PixPerDegY = (float)atof(temp);
		// VoltsPerDegX
		::GetPrivateProfileString("ApplicationInfo", "VoltsPerDegX", "", temp, 40, filename);
		VRangePerDegX = (float)atof(temp);
		if (VRangePerDegX > 0.6 || VRangePerDegX < 0.2)
		VRangePerDegX = (float)0.4;
		// VoltsPerDegY
		::GetPrivateProfileString("ApplicationInfo", "VoltsPerDegY", "", temp, 40, filename);
		VRangePerDegY = (float)atof(temp);
		if (VRangePerDegY > 0.6 || VRangePerDegY < 0.2)
		VRangePerDegY = (float)0.4;
		// MotionAngleX
		::GetPrivateProfileString("ApplicationInfo", "MotionAngleX", "", temp, 40, filename);
		MotionAngleX = (float)atof(temp);
		if (MotionAngleX < -30. || MotionAngleX > 30.)
		MotionAngleX = 0.;
		// MotionAngleY
		::GetPrivateProfileString("ApplicationInfo", "MotionAngleY", "", temp, 40, filename);
		MotionAngleY = (float)atof(temp);
		if (MotionAngleY < -30. || MotionAngleY > 30.)
		MotionAngleY = 0.;
		// OCT traces invert flag
		::GetPrivateProfileString("ApplicationInfo", "InvertTraces", "", temp, 40, filename);
		g_InvertMotionTraces = atoi(temp);
		// TCA Override flag
		::GetPrivateProfileString("ApplicationInfo", "ApplyTCA", "", temp, 40, filename);
		ApplyTCA = atoi(temp);
		// TCA Override X values for 3 channels (RGB)
		::GetPrivateProfileString("ApplicationInfo", "RGBClkShiftsX", "", temp, 40, filename);
		patch_count = atoi(temp);
		RGBClkShifts[2].x = (patch_count & 0xFF)-64;
		RGBClkShifts[1].x = ((patch_count>>8) & 0xFF)-64;
		RGBClkShifts[0].x = ((patch_count>>16) & 0xFF)-64;
		patch_count = 0;
		// TCA Override Y values for 3 channels (RGB)
		::GetPrivateProfileString("ApplicationInfo", "RGBClkShiftsY", "", temp, 40, filename);
		patch_count = atoi(temp);
		RGBClkShifts[2].y = (patch_count & 0xFF)-64;
		RGBClkShifts[1].y = ((patch_count>>8) & 0xFF)-64;
		RGBClkShifts[0].y = ((patch_count>>16) & 0xFF)-64;
		if (!RGBClkShifts[0].x & !RGBClkShifts[1].y & !RGBClkShifts[1].x & !!RGBClkShifts[1].y) 
			ApplyTCA = FALSE;

		// Imaging Wavelength
		::GetPrivateProfileString("ApplicationInfo", "Imaging Wavelength", "", temp, 6, filename);
		if(strcmp(temp, "840nm") == 0)
			Imaging840 = true;
		else
			Imaging840 = false;

		// Frame Info
		// Frame rotation
		::GetPrivateProfileString("FrameInfo", "Frame_Rotation", "", temp, 40, filename);
		FRAME_ROTATION = atoi(temp);
		// frame width
		::GetPrivateProfileString("FrameInfo", "F_XDIM", "", temp, 40, filename);
		F_XDIM = atoi(temp);
		// frame height
		::GetPrivateProfileString("FrameInfo", "F_YDIM", "", temp, 40, filename);
		F_YDIM = atoi(temp);
		// filtering
		::GetPrivateProfileString("FrameInfo", "Filter", "", temp, 40, filename);
		Filter = atoi(temp);
		// desinusoiding
		::GetPrivateProfileString("FrameInfo", "Desinusoid", "", temp, 40, filename);
		Desinusoid = atoi(temp);
		// lookup table
		::GetPrivateProfileString("FrameInfo", "DesinusoidLUT", "", temp, 80, filename);
		fnameLUT.Format("%s", temp);
		//check for LUT file existance
		if (_access (fnameLUT, 0) != 0) {
			Desinusoid = 0;
			fnameLUT = "";
		}		
		// Digital marks enable flags
		::GetPrivateProfileString("FrameInfo", "WriteMarkFlags", "", temp, 40, filename);
		flags = atoi(temp);	
		for (int i=0; i<9; i++)
			WriteMarkFlags[i] = getBit(flags, i);

	};

	void SaveConfigFile()
	{
		char     temp[80];
		int		 flags;
		CString  filename;

		filename = _T("\\AppParams.ini");
		filename = m_strConfigPath + filename;

		// Application Info
		// Stimuli Path
		::WritePrivateProfileString("ApplicationInfo", "StimuliPath", StimuliPath, filename);
		// Pixels Per Degree X
		sprintf_s(temp, "%1.3f", PixPerDegX);
		::WritePrivateProfileString("ApplicationInfo", "PixelsperDegX", temp, filename);
		// Pixels Per Degree Y
		sprintf_s(temp, "%1.3f", PixPerDegY);
		// Motion Trace rotation X
		sprintf_s(temp, "%2.2f", MotionAngleX);
		::WritePrivateProfileString("ApplicationInfo", "MotionAngleX", temp, filename);
		// Motion Trace rotation Y
		sprintf_s(temp, "%2.2f", MotionAngleY);
		::WritePrivateProfileString("ApplicationInfo", "MotionAngleY", temp, filename);
		// motion trace inversion
		sprintf_s(temp, "%d", g_InvertMotionTraces);
		::WritePrivateProfileString("ApplicationInfo", "InvertTraces", temp, filename);
		// TCA Apply flag
		sprintf_s(temp, "%d", ApplyTCA);
		::WritePrivateProfileString("ApplicationInfo", "ApplyTCA", temp, filename);
		// TCA values X
		flags = (((((short)(64+RGBClkShifts[0].x)<<8) | (short)(64+RGBClkShifts[1].x))<<8) | (short)(64+RGBClkShifts[2].x));
		sprintf_s(temp, "%d", flags);
		::WritePrivateProfileString("ApplicationInfo", "RGBClkShiftsX", temp, filename);
		// TCA Values Y
		flags = 0;
		flags = (((((short)(64+RGBClkShifts[0].y)<<8) | (short)(64+RGBClkShifts[1].y))<<8) | (short)(64+RGBClkShifts[2].y));
		sprintf_s(temp, "%d", flags);
		::WritePrivateProfileString("ApplicationInfo", "RGBClkShiftsY", temp, filename);
		// Imaging Wavelength
		CString wl;
		Imaging840 ? wl="840nm" : wl="788nm";
		::WritePrivateProfileString("ApplicationInfo", "Imaging Wavelength", wl, filename);
		

		// Frame Info
		// frame width
		sprintf_s(temp, "%d", F_XDIM);
		::WritePrivateProfileString("FrameInfo", "F_XDIM", temp, filename);
		// frame height
		sprintf_s(temp, "%d", F_YDIM);
		::WritePrivateProfileString("FrameInfo", "F_YDIM", temp, filename);
		// filtering
		sprintf_s(temp, "%d", Filter);
		::WritePrivateProfileString("FrameInfo", "Filter", temp, filename);
		// desinusoiding
		sprintf_s(temp, "%d", Desinusoid);
		::WritePrivateProfileString("FrameInfo", "Desinusoid", temp, filename);
		// desinusoiding look up table
		::WritePrivateProfileString("FrameInfo", "DesinusoidLUT", fnameLUT, filename);
		bitset<9> WriteMarkFlagsbt;
		for (int i=0; i<9; i++)
			WriteMarkFlagsbt[i] = WriteMarkFlags[i];
		flags = WriteMarkFlagsbt.to_ulong();
		// Write Mark flags status
		sprintf_s(temp, "%d", flags);
		::WritePrivateProfileString("FrameInfo", "WriteMarkFlags", temp, filename);

	};

	BOOL getBit(int number, unsigned whichBit)
	{
		return (((number & (1 << whichBit)) >> whichBit)?TRUE:FALSE);
	}

	void LoadWavelengthOptions(){

		char			temp1[3], temp2[2];
		CString			filename;
		int				wl_index = 12;

		filename = _T("\\WaveLengths.ini");
		filename = m_strConfigPath + filename;

		for (int i = 0; i < wl_index; i++) {
			itoa(i,temp2,10);
			::GetPrivateProfileString("WaveLengths", temp2, "", temp1, 4, filename);
			waveLength[i] = temp1;
			}
		   
	}

};

/*
   the part below is a model for accessing Window XP physical memory
 */
/*
class CPhyMemAccess {

private:
	typedef LONG    NTSTATUS;
 
	typedef struct _UNICODE_STRING
	{
		USHORT Length;
		USHORT MaximumLength;
		PWSTR Buffer;
	} UNICODE_STRING, *PUNICODE_STRING;
 
	typedef enum _SECTION_INHERIT
	{
		ViewShare = 1,
		ViewUnmap = 2
	} SECTION_INHERIT, *PSECTION_INHERIT;
 
	typedef struct _OBJECT_ATTRIBUTES
	{
		ULONG Length;
		HANDLE RootDirectory;
		PUNICODE_STRING ObjectName;
		ULONG Attributes;
		PVOID SecurityDescriptor;
		PVOID SecurityQualityOfService;
	} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

	void InitializeObjectAttributes(OBJECT_ATTRIBUTES *p, PUNICODE_STRING n, ULONG a, HANDLE r, PVOID s ) {  
		(p)->Length = sizeof( OBJECT_ATTRIBUTES );  
		(p)->RootDirectory = r;  
		(p)->Attributes = a;  
		(p)->ObjectName = n;  
		(p)->SecurityDescriptor = s;  
		(p)->SecurityQualityOfService = NULL;  
	}


	// Interesting functions in NTDLL
	typedef NTSTATUS (WINAPI *ZwOpenSectionProc)
	(
		PHANDLE SectionHandle,
		DWORD DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes
	);
	typedef NTSTATUS (WINAPI *ZwMapViewOfSectionProc)
	(
		HANDLE SectionHandle,
		HANDLE ProcessHandle,
		PVOID *BaseAddress,
		ULONG ZeroBits,
		ULONG CommitSize,
		PLARGE_INTEGER SectionOffset,
		PULONG ViewSize,
		SECTION_INHERIT InheritDisposition,
		ULONG AllocationType,
		ULONG Protect
	);
	typedef NTSTATUS (WINAPI *ZwUnmapViewOfSectionProc)
	(
		HANDLE ProcessHandle,
		PVOID BaseAddress
	);
	typedef VOID (WINAPI *RtlInitUnicodeStringProc)
	(
		IN OUT PUNICODE_STRING DestinationString,
		IN PCWSTR SourceString
	);
 
	// Global variables
	//static HMODULE hModule = NULL;
	//static HANDLE hPhysicalMemory = NULL;
	static HMODULE hModule;
	static HANDLE hPhysicalMemory;
	static ZwOpenSectionProc ZwOpenSection;
	static ZwMapViewOfSectionProc ZwMapViewOfSection;
	static ZwUnmapViewOfSectionProc ZwUnmapViewOfSection;
	static RtlInitUnicodeStringProc RtlInitUnicodeString;

public:

	// initialize
	BOOL InitPhysicalMemory()
	{
		if (!(hModule = LoadLibrary("ntdll.dll")))
		{
			return FALSE;
		}
 
		// We need to obtain several function pointers from NTDLL
		if (!(ZwOpenSection = (ZwOpenSectionProc)GetProcAddress(hModule, "ZwOpenSection")))
		{
			return FALSE;
		}
 
		if (!(ZwMapViewOfSection = (ZwMapViewOfSectionProc)GetProcAddress(hModule, "ZwMapViewOfSection")))
		{
			return FALSE;
		}
 
		if (!(ZwUnmapViewOfSection = (ZwUnmapViewOfSectionProc)GetProcAddress(hModule, "ZwUnmapViewOfSection")))
		{
			return FALSE;
		}
  
		if (!(RtlInitUnicodeString = (RtlInitUnicodeStringProc)GetProcAddress(hModule, "RtlInitUnicodeString")))
		{
			return FALSE;
		}
 
		// open kernel objects
		WCHAR PhysicalMemoryName[] = L"\\Device\\PhysicalMemory";
		UNICODE_STRING PhysicalMemoryString;
		OBJECT_ATTRIBUTES attributes;
		RtlInitUnicodeString(&PhysicalMemoryString, PhysicalMemoryName);
		InitializeObjectAttributes(&attributes, &PhysicalMemoryString, 0, NULL, NULL);
		NTSTATUS status = ZwOpenSection(&hPhysicalMemory, SECTION_MAP_READ, &attributes );
 
		return (status >= 0);
	};

	// terminate -- free handles
	void ExitPhysicalMemory()
	{
		if (hPhysicalMemory != NULL)
		{
			CloseHandle(hPhysicalMemory);
		}
 
		if (hModule != NULL)
		{
			FreeLibrary(hModule);
		}
	};

	BOOL ReadPhysicalMemory(PVOID buffer, DWORD address, DWORD length)
	{
		DWORD outlen;            // output length, possibly larger than required length depending on the size of memory page
		PVOID vaddress;          // mapped virtual address
		NTSTATUS status;         // returned status of NTDLL functions
		LARGE_INTEGER base;      // physical memory address
 
		vaddress = 0;
		outlen = length;
		base.QuadPart = (ULONGLONG)(address);
 
		// map physical memory address to the current virtual memory address
		status = ZwMapViewOfSection(hPhysicalMemory,
			(HANDLE) -1,
			(PVOID *)&vaddress,
			0,
			length,
			&base,
			&outlen,
			ViewShare,
			0,
			PAGE_READONLY);
 
		if (status < 0)
		{
			return FALSE;
		}
 
		// copy data to output buffer from the current virtual memory address space
		memmove(buffer, vaddress, length);
 
		// finish physical memory accessing, cancel address mapping
		status = ZwUnmapViewOfSection((HANDLE)-1, (PVOID)vaddress);
 
		return (status >= 0);
	};
};
*/

class PatchParams
{
public:
//	int        *teleHeader;			// telegraph header
	int        *UnMatrixIdx;		// lookup table for possible desinusoiding
	float      *UnMatrixVal;
	float      *shift_xy;
//	float      *shift_xy0;
	float      *shift_xi;			// interpolated delta x and delta y
	float      *shift_yi;

	BYTE       *img_sliceR;			// raw image block from Matrox grabber
	BYTE       *img_slice;
	float      *StretchFactor;
};


