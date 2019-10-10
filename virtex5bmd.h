#ifndef AFX_VIRTEX5BMD_H
#define AFX_VIRTEX5BMD_H
#include <iostream>
#include <afx.h>

#include "utils/include/wdc_lib.h"
#include "utils/samples/shared/pci_regs.h"
#include "utils/samples/shared/bits.h"
#include "utils/include/wdc_defs.h"
#include "utils/include/utils.h"
#include "utils/include/status_strings.h"
//#include <stdio.h>

#define VIRTEX5_DEFAULT_VENDOR_ID 0x10EE /* Vendor ID */
#define VIRTEX5_DEFAULT_DEVICE_ID 0x7 /* Device ID */
#define KP_VRTX5_DRIVER_NAME "KP_VRTX5"
#define VIRTEX5_SPACE AD_PCI_BAR0

#define  SYSTEM_LATENCY_DAC8  13	// system latency with 8-bit DAC, in # of data clocks
#define  SYSTEM_LATENCY_DAC14 9		// system latency with 14-bit DAC, in # of data clocks,
#define  STIM_CHANNEL_IR      1		//	.... DAC14 changed from 9 to X, 24.5.2017, WMH
#define  STIM_CHANNEL_GR      2
#define  STIM_CHANNEL_RD      3
#define  STIM_CHANNEL_NU      4
#define  AOM_LATENCYX_IR	  10	//changed from 2 to 13, 24.5.2017, WMH; 11 for 788-driver, 10.09.2019, ND
#define  AOM_LATENCYX_RED	  9		//changed from 12 to 10, 09.06.2017, ND; from 10 to -3, 18.07.2017 ND; from -3 to 10, 02.02.2018 ND; from 10 to 8, 18.07.2017 ND; from 8 to 10, 16.01.2019 ND; from 10 to 9, 10.09.2019 ND;
#define  AOM_LATENCYX_GR	  4		//changed from 0 to X, 24.5.2017, WMH; from 0 to -8, 18.07.2017 ND; from -8 to 5, 02.02.2018 ND; from 5 to 3, 18.07.2017 ND; from 3 to 5, 16.01.2019 ND; from 5 to 4, 10.09.2019 ND;

enum {
    VIRTEX5_DSCR_OFFSET       = 0x0,
    VIRTEX5_DDMACR_OFFSET     = 0x4,
    VIRTEX5_WDMATLPA_OFFSET   = 0x8,
    VIRTEX5_WDMATLPS_OFFSET   = 0xc,
    VIRTEX5_WDMATLPC_OFFSET   = 0x10,
    VIRTEX5_WDMATLPP_OFFSET   = 0x14,
    VIRTEX5_RDMATLPP_OFFSET   = 0x18,
    VIRTEX5_RDMATLPA_OFFSET   = 0x1c,
    VIRTEX5_RDMATLPS_OFFSET   = 0x20,
    VIRTEX5_RDMATLPC_OFFSET   = 0x24,
    //VIRTEX5_WDMAPERF_OFFSET = 0x28,
	VIRTEX5_PUPILMASK_OFFSET  = 0x28,
    VIRTEX5_TCABOX_OFFSET     = 0x2c,
    VIRTEX5_RDMASTAT_OFFSET   = 0x30,
   // VIRTEX5_NRDCOMP_OFFSET = 0x34,			// laser power 2
    //VIRTEX5_RCOMPDSIZE_OFFSET = 0x38,		// stimulus boundary 2
	VIRTEX5_LASER_POWER2      = 0x34,
	VIRTEX5_STIMULUS_X_BOUND2 = 0x38,
    VIRTEX5_DLWSTAT_OFFSET    = 0x3c,
    VIRTEX5_DLTRSSTAT_OFFSET  = 0x40, 
    VIRTEX5_DMISCCONT_OFFSET  = 0x44,
	VIRTEX5_I2CINFO_OFFSET    = 0x48,
	VIRTEX5_LINEINFO_OFFSET   = 0x4c,
	VIRTEX5_IMAGESIZE_OFFSET  = 0x50,
	VIRTEX5_IMAGEOS_OFFSET    = 0x54,
	VIRTEX5_LINECTRL_OFFSET   = 0x58,
	VIRTEX5_STIMULUS_LOC      = 0x5c,
	VIRTEX5_LASER_POWER       = 0x60,
	VIRTEX5_STIM_ADDRESS      = 0x64,
	VIRTEX5_STIM_DATA         = 0x68,
	VIRTEX5_STIMULUS_X_BOUND  = 0x6c
};

/* Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user-mode) /
 * KP_VRTX5_Call() (kernel-mode) */
enum {
    KP_VRTX5_MSG_VERSION = 1, /* Query the version of the Kernel PlugIn */
	KP_VRTX5_MSG_DATA = 2, /* Pass user-mode data to Kernel Plugin */
};

/* Kernel Plugin messages status */
enum {
    KP_VRTX5_STATUS_OK = 0x1,
	KP_VRTX5_DATA_RECEIVED = 0x2,
    KP_VRTX5_STATUS_MSG_NO_IMPL = 0x1000,
};

// Register information struct 
typedef struct {
    DWORD dwAddrSpace;       // Number of address space in which the register resides 
                             // For PCI configuration registers, use WDC_AD_CFG_SPACE 
    DWORD dwOffset;          // Offset of the register in the dwAddrSpace address space 
    DWORD dwSize;            // Register's size (in bytes) 
    WDC_DIRECTION direction; // Read/write access mode - see WDC_DIRECTION options 
    CHAR  sName[MAX_NAME];   // Register's name 
    CHAR  sDesc[MAX_DESC];   // Register's description 
} WDC_REG;

// Direct Memory Access (DMA) 
typedef struct {
    WD_DMA *pDma;
    WDC_DEVICE_HANDLE hDev;
} VIRTEX5_DMA_STRUCT, *VIRTEX5_DMA_HANDLE;
 
// Kernel PlugIn version information struct 
typedef struct {
    DWORD dwVer;
    CHAR cVer[100];
} KP_VRTX5_VERSION;

// Address space information struct 
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} VIRTEX5_ADDR_SPACE_INFO;

// Interrupt result information struct 
typedef struct
{
    DWORD dwCounter; /* Number of interrupts received */
    DWORD dwLost;    /* Number of interrupts not yet handled */
    WD_INTERRUPT_WAIT_RESULT waitResult; /* See WD_INTERRUPT_WAIT_RESULT values
                                            in windrvr.h */
    BOOL fIsMessageBased;
    DWORD dwLastMessage;

    PVOID pBuf;
    UINT32 u32Pattern;
    DWORD dwTotalCount;
    BOOL fIsRead;
} VIRTEX5_INT_RESULT;

// VIRTEX5 diagnostics interrupt handler function type 
typedef void (*VIRTEX5_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    VIRTEX5_INT_RESULT *pIntResult);

// VIRTEX5 diagnostics plug-and-play and power management events handler function type 
typedef void (*VIRTEX5_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    DWORD dwAction);

// VIRTEX5 device information struct 
typedef struct {
    VIRTEX5_INT_HANDLER funcDiagIntHandler;
    VIRTEX5_EVENT_HANDLER funcDiagEventHandler;
    VIRTEX5_DMA_HANDLE hDma;
    PVOID pBuf;
    BOOL fIsRead;
    UINT32 u32Pattern;
    DWORD dwTotalCount;
} VIRTEX5_DEV_CTX, *PVIRTEX5_DEV_CTX;


typedef struct {
    VIRTEX5_DMA_HANDLE hDma;
    PVOID pBuf;
} DIAG_DMA, *PDIAG_DMA;


typedef struct {
	UINT32    line_start_addr;
	UINT32    line_end_addr;
	UINT32    addr_interval;
	unsigned short   end_line_ID;
	unsigned short   img_width;
	unsigned short   img_height;
	unsigned short   offset_line;
	unsigned short   offset_pixel;
	unsigned short   tlp_counts;
	unsigned char    line_spacing;
	unsigned char   *video_in;
	unsigned char   *video_out;
} VIDEO_INFO;

class CVirtex5BMD {
private:
	char     *m_errMsg;
	DWORD     m_errCode;

public:
//	void AppAddTCAbox(WDC_DEVICE_HANDLE hDev, int sxIR, int syIR, int dxIR, int dyIR,
//											  int sxRd, int syRd, int dxRd, int dyRd,
//											  int sxGr, int syGr, int dxGr, int dyGr);
	void AppMotionTraces(WDC_DEVICE_HANDLE hDev, float deltaX, float deltaY, float scalerX, float scalerY, FILE*);
	void AppGreenAOM_Off(WDC_DEVICE_HANDLE hDev);
	void AppGreenAOM_On(WDC_DEVICE_HANDLE hDev);
	void AppRedAOM_Off(WDC_DEVICE_HANDLE hDev);
	void AppRedAOM_On(WDC_DEVICE_HANDLE hDev);
//	void AppShiftGreenY(WDC_DEVICE_HANDLE hDev, int lines);
//	void AppShiftRedY(WDC_DEVICE_HANDLE hDev, int lines);
//	void AppRemoveTCAbox(WDC_DEVICE_HANDLE hDev);
	void AppWriteStimLoc(WDC_DEVICE_HANDLE hDev, int x, int y, int latency);
	BOOL DetectSyncSignals(WDC_DEVICE_HANDLE hDev);
	void AppUpdate14BITsLaserGR(WDC_DEVICE_HANDLE hDev, int power);
	void AppUpdate14BITsLaserRed(WDC_DEVICE_HANDLE hDev, int power);
	void StimLinesOff(WDC_DEVICE_HANDLE hDev);
	void StimLinesOn(WDC_DEVICE_HANDLE hDev);
	void AppStopADC(WDC_DEVICE_HANDLE hDev);
	void AppStartADC(WDC_DEVICE_HANDLE hDev);
	void AppWriteStimAddrShift(WDC_DEVICE_HANDLE hDev, int y1, int y2, int y3, int stimWidth, int stripeH, int channelID);
	void AppWriteStimAddrShift(WDC_DEVICE_HANDLE hDev, int yc, int ys_red, int ys_gr, int dx_ir, int dy_ir, int dx_gr, int dy_gr, int dx_rd, int dy_rd);
	void AppWriteStimAddrShift(WDC_DEVICE_HANDLE hDev, int y1, int y2, int y3, int y4, int dx_rd, int stripeH, int stripeID);
	void AppWriteStimLUT(WDC_DEVICE_HANDLE hDev, bool latency, int x1ir, int x2ir, int x1gr, int x2gr, int x1rd, int x2rd, int y1, int y2, int y3, int ysRD, int ysGR, int sinusoid, int xLir, int xRir, int xLgr, int xRgr, int xLrd, int xRrd);
	void AppWriteStimLUT(WDC_DEVICE_HANDLE hDev, bool latency, int xcIR, int ycIR, int xcGR, int ycGR, int xcRD, int ycRD, int iry, int rdy, int gry, int sinusoid, int xLir, int xRir, int xLgr, int xRgr, int xLrd, int xRrd);
	void AppWriteStimLUT(WDC_DEVICE_HANDLE hDev, bool latency, int x0, int x1, int xLp, int xRp, int y1, int y2, int y3, int sinusoid, int channelID);
	void RedCalibrationOn(WDC_DEVICE_HANDLE hDev);
	void RedCalibrationOff(WDC_DEVICE_HANDLE hDev);
	void PupilMaskOn(WDC_DEVICE_HANDLE hDev);
	void PupilMaskOff(WDC_DEVICE_HANDLE hDev);
	void WritePupilMask(WDC_DEVICE_HANDLE hDev, UINT32 sx, UINT32 sy, UINT32 nx, UINT32 ny);
	void ReadPupilMask(WDC_DEVICE_HANDLE hDev, UINT32 *sx, UINT32 *sy, UINT32 *nx, UINT32 *ny);

	//BOOL AppReadCriticalInfo(WDC_DEVICE_HANDLE hDev);
	//void AppClearCriticalInfo(WDC_DEVICE_HANDLE hDev);
	//void AppWriteCriticalInfo(WDC_DEVICE_HANDLE hDev, int targetY, float predictionTime);

	void AppLoadStimulus14bits(WDC_DEVICE_HANDLE hDev, unsigned short *buffer, int nx, int ny, int channelID);
	void AppLoadStimulus8bits(WDC_DEVICE_HANDLE hDev, unsigned short *buffer, int nx, int ny);
	void AppWriteWarpLUT(WDC_DEVICE_HANDLE hDev, int nSizeX, int nStretchedX, unsigned short *lut_buf, int nChannelID);
	void AppWriteWarpWeights(WDC_DEVICE_HANDLE hDev, int nSizeX, UINT32 *stim_buf, int channelID);
	void AppSamplePatch(WDC_DEVICE_HANDLE hDev, VIDEO_INFO VideoInfo, UINT32 lineID);
	void AppIRAOM_Off(WDC_DEVICE_HANDLE hDev);
	void AppIRAOM_On(WDC_DEVICE_HANDLE hDev);
//	void AppShiftRedX(WDC_DEVICE_HANDLE hDev, int clocks);
//	void AppShiftGreenX(WDC_DEVICE_HANDLE hDev, int clocks);
	void AppUpdateIRLaser(WDC_DEVICE_HANDLE hDev, BYTE power);
//	void AppUpdateRedLaser(WDC_DEVICE_HANDLE hDev, BYTE power);
	int  I2CController(WDC_DEVICE_HANDLE hDev, UINT32 regDataW, BYTE *reg_val, BOOL bI2Cread);
	int  UpdateRuntimeRegisters(WDC_DEVICE_HANDLE hDev, unsigned char *m_adcVals);
	int  ReadWriteI2CRegister(WDC_DEVICE_HANDLE hDev, BOOL bRead, UINT32 slave_addr, BYTE m_regAddr, BYTE regValI, BYTE *regValO);
	void AppUpdateSampleInfo(WDC_DEVICE_HANDLE hDev, VIDEO_INFO VideoInfo);
	void AppAOM_Off(WDC_DEVICE_HANDLE hDev);
	void AppAOM_On(WDC_DEVICE_HANDLE hDev);
	void AppInitStimulus(WDC_DEVICE_HANDLE hDev, VIDEO_INFO g_VideoInfo, int nx, int ny);
	void AppLoadStimulus(WDC_DEVICE_HANDLE hDev, unsigned short *buffer, int nx, int ny, int channelID);
//	void AppWriteStimulus(WDC_DEVICE_HANDLE hDev, int x0, int x, int y, int clk_delay, int sinusoid, BYTE x1, BYTE x2);
	void AppSetFrameCounter(WDC_DEVICE_HANDLE hDev, BOOL bByLines);
	void AppVsyncTrigEdge(WDC_DEVICE_HANDLE hDev, BOOL bStatus);
	void AppSetADCchannel(WDC_DEVICE_HANDLE hDev, BYTE chID);
	void UpdateBrightnessContrast();
	CVirtex5BMD();
	~CVirtex5BMD();

	DWORD VIRTEX5_LibInit(void);

	WDC_DEVICE_HANDLE DeviceFindAndOpen(DWORD dwVendorId, DWORD dwDeviceId);
	BOOL DeviceFind(DWORD dwVendorId, DWORD dwDeviceId, WD_PCI_SLOT *pSlot);
	WDC_DEVICE_HANDLE DeviceOpen(const WD_PCI_SLOT *pSlot);
	void DeviceClose(WDC_DEVICE_HANDLE hDev, PDIAG_DMA pDma);
	WDC_DEVICE_HANDLE VIRTEX5_DeviceOpen(const WD_PCI_CARD_INFO *pDeviceInfo);
	BOOL VIRTEX5_DeviceClose(WDC_DEVICE_HANDLE hDev);
	void DIAG_DMAClose(WDC_DEVICE_HANDLE hDev, PDIAG_DMA pDma);
	DWORD VIRTEX5_IntDisable(WDC_DEVICE_HANDLE hDev);
	void VIRTEX5_DMAClose(VIRTEX5_DMA_HANDLE hDma);
	BOOL IsValidDmaHandle(VIRTEX5_DMA_HANDLE hDma, CHAR *sFunc);
	BOOL VIRTEX5_IntIsEnabled(WDC_DEVICE_HANDLE hDev);
	DWORD VIRTEX5_LibUninit(void);
	BOOL ReadWriteReg(WDC_DEVICE_HANDLE hDev, WDC_REG *pRegs, DWORD dwReg,
						DWORD dwNumRegs, WDC_DIRECTION direction, BOOL fPciCfg,
						UINT64 *u64Rdata, UINT u64Wdata);
	WORD DMAGetMaxPacketSize(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);
	UINT32 VIRTEX5_ReadReg32(WDC_DEVICE_HANDLE hDev, DWORD offset);
	WORD code2size(BYTE bCode);
	DWORD VIRTEX5_DMAOpen(WDC_DEVICE_HANDLE hDev, PVOID *ppBuf, DWORD dwOptions,
						DWORD dwBytes, VIRTEX5_DMA_HANDLE *ppDmaHandle);
	void VIRTEX5_DmaIntEnable(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);
	void VIRTEX5_DmaIntDisable(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);
	void VIRTEX5_WriteReg32(WDC_DEVICE_HANDLE hDev, DWORD offset, UINT32 data);
	void VIRTEX5_DMADevicePrepare(VIRTEX5_DMA_HANDLE hDma, BOOL fIsRead, WORD wSize,
							WORD wCount, UINT32 u32Pattern, BOOL fEnable64bit, BYTE bTrafficClass);
	void VIRTEX5_WriteReg16(WDC_DEVICE_HANDLE hDev, DWORD offset, WORD data);
	DWORD VIRTEX5_IntEnable(WDC_DEVICE_HANDLE hDev, VIRTEX5_INT_HANDLER funcIntHandler);


	char   *GetErrMsg();
	DWORD   GetErrCode();
};

#endif
