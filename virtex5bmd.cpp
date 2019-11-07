
#if defined (__KERNEL__)
    #include "include/kpstdlib.h"
#endif 
//#include "include/wdc_defs.h"
//#include "include/utils.h"
//#include "include/status_strings.h"

#include "virtex5bmd.h"
#include <cmath.h>
#include "math.h"
#include "ICANDIParams.h"

#define VIRTEX5_DEFAULT_LICENSE_STRING "6f1eafddeade6025f0620c070c601c684c5341b7ce19f2.Montana State University"
#define VIRTEX5_DEFAULT_DRIVER_NAME "windrvr6"

int __inline roundf(const float x)
{
  __asm cvtss2si eax, x
}

extern void DLLCALLCONV VIRTEX5_IntHandler(PVOID pData);
extern BOOL g_bKernelPlugin;
extern VIDEO_INFO        g_VideoInfo;
extern ICANDIParams    g_ICANDIParams;

BOOL DeviceValidate(PWDC_DEVICE pDev)
{
    DWORD i, dwNumAddrSpaces = pDev->dwNumAddrSpaces;

    // Verify that the device has at least one active address space 
    for (i = 0; i < dwNumAddrSpaces; i++)
    {
        if (WDC_AddrSpaceIsActive(pDev, i))
            return TRUE;
    }
    
    return FALSE;
}

BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !WDC_GetDevContext(pDev))
    {
        return FALSE;
    }

    return TRUE;
}



CVirtex5BMD::CVirtex5BMD() {
	m_errMsg          = new char [256];
	m_errCode         = WD_STATUS_SUCCESS;
}
CVirtex5BMD::~CVirtex5BMD() {
	//delete [] m_errMsg;
}

/*************************************************************
  Functions implementation
 *************************************************************/
/* -----------------------------------------------
    VIRTEX5 and WDC library initialize/uninit
   ----------------------------------------------- */
DWORD CVirtex5BMD::VIRTEX5_LibInit(void)
{
    DWORD dwStatus;
 
    // Set WDC library's debug options
    // (default: level TRACE, output to Debug Monitor) 
    dwStatus = WDC_SetDebugOptions(WDC_DBG_DEFAULT, NULL);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        //sprintf(m_errMsg, "Failed to initialize debug options for WDC library.\n"
		//				  "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
        m_errCode = dwStatus;

        return dwStatus;
    }

    // Open a handle to the driver and initialize the WDC library 
    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT,
        VIRTEX5_DEFAULT_LICENSE_STRING);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
//        sprintf(m_errMsg, "Failed to initialize the WDC library. Error 0x%lx - %s\n",
//						  dwStatus, Stat2Str(dwStatus));
		m_errCode = dwStatus;
        
        return dwStatus;
    }

    return WD_STATUS_SUCCESS;
}


char * CVirtex5BMD::GetErrMsg() {
	return m_errMsg;
}

DWORD CVirtex5BMD::GetErrCode() {
	return m_errCode;
}


// Find and open a VIRTEX5 device 
WDC_DEVICE_HANDLE CVirtex5BMD::DeviceFindAndOpen(DWORD dwVendorId, DWORD dwDeviceId)
{
    WD_PCI_SLOT slot;
    
    if (!DeviceFind(dwVendorId, dwDeviceId, &slot))
        return NULL;

    return DeviceOpen(&slot);
}

// Find a VIRTEX5 device 
BOOL CVirtex5BMD::DeviceFind(DWORD dwVendorId, DWORD dwDeviceId, WD_PCI_SLOT *pSlot)
{
    DWORD dwStatus;
    //DWORD i, dwNumDevices;
	DWORD dwNumDevices;
    WDC_PCI_SCAN_RESULT scanResult;

    BZERO(scanResult);
    dwStatus = WDC_PciScanDevices(dwVendorId, dwDeviceId, &scanResult);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
//        sprintf(m_errMsg, "DeviceFind: Failed scanning the PCI bus.\n"
  //          "Error: 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
		m_errCode = dwStatus;

        return FALSE;
    }

    dwNumDevices = scanResult.dwNumDevices;
    if (!dwNumDevices)
    {
//        sprintf(m_errMsg, "No matching device was found for search criteria "
  //          "(Vendor ID 0x%lX, Device ID 0x%lX)\n", dwVendorId, dwDeviceId);
		m_errCode = dwStatus;

        return FALSE;
    }
    
    if (dwNumDevices > 1)
    {
		// found multiple devices
    }

    *pSlot = scanResult.deviceSlot[0];

    return TRUE;
}

// Open a handle to a VIRTEX5 device 
WDC_DEVICE_HANDLE CVirtex5BMD::DeviceOpen(const WD_PCI_SLOT *pSlot)
{
    WDC_DEVICE_HANDLE hDev;
    DWORD dwStatus;
    WD_PCI_CARD_INFO deviceInfo;
    
    // Retrieve the device's resources information 
    BZERO(deviceInfo);
    deviceInfo.pciSlot = *pSlot;
    dwStatus = WDC_PciGetDeviceInfo(&deviceInfo);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
//        sprintf(m_errMsg, "DeviceOpen: Failed retrieving the device's resources "
  //          "information.\nError 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
		m_errCode = dwStatus;

        return NULL;
    }

    // Open a handle to the device 
    hDev = VIRTEX5_DeviceOpen(&deviceInfo);
    if (!hDev)
    {
        return NULL;
    }

    return hDev;
}

// Close handle to a VIRTEX5 device 
void CVirtex5BMD::DeviceClose(WDC_DEVICE_HANDLE hDev, PDIAG_DMA pDma)          
{
    if (!hDev)
        return;

     if (pDma)   
         DIAG_DMAClose(hDev, pDma);      
         
    if (!VIRTEX5_DeviceClose(hDev))
    {
//        sprintf(m_errMsg, "DeviceClose: Failed closing VIRTEX5 device");
    }
}


/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
WDC_DEVICE_HANDLE CVirtex5BMD::VIRTEX5_DeviceOpen(const WD_PCI_CARD_INFO *pDeviceInfo)
{
    DWORD dwStatus;
    PVIRTEX5_DEV_CTX pDevCtx = NULL;
    WDC_DEVICE_HANDLE hDev = NULL;

    // Validate arguments 
    if (!pDeviceInfo)
    {
//        sprintf(m_errMsg, "VIRTEX5_DeviceOpen: Error - NULL device information struct. pointer\n");
        return NULL;
    }

    // Allocate memory for the VIRTEX5 device context 
    pDevCtx = (PVIRTEX5_DEV_CTX)calloc(1, sizeof(VIRTEX5_DEV_CTX));
    if (!pDevCtx)
    {
//        sprintf(m_errMsg, "Failed allocating memory for VIRTEX5 device context\n");
        return NULL;
    }

    // Open a Kernel PlugIn WDC device handle 
	g_bKernelPlugin = TRUE;
    dwStatus = WDC_PciDeviceOpen(&hDev, pDeviceInfo, pDevCtx, NULL, KP_VRTX5_DRIVER_NAME, &hDev);
    
    // if failed, try opening a WDC device handle without using Kernel PlugIn 
    if(dwStatus != WD_STATUS_SUCCESS)
    {
		g_bKernelPlugin = FALSE;
        dwStatus = WDC_PciDeviceOpen(&hDev, pDeviceInfo, pDevCtx, NULL, NULL, NULL);
    }
    if (WD_STATUS_SUCCESS != dwStatus)
    {
//        sprintf(m_errMsg, "Failed opening a WDC device handle. Error 0x%lx - %s\n",
  //          dwStatus, Stat2Str(dwStatus));
		m_errCode = dwStatus;

        goto Error;
    }

    // Validate device information 
    if (!DeviceValidate((PWDC_DEVICE)hDev)) {
//		sprintf(m_errMsg, "Device does not have any active memory or I/O address spaces\n");
        goto Error;
	}

    // Return handle to the new device 
/*    TraceLog("VIRTEX5_DeviceOpen: Opened a VIRTEX5 device (handle 0x%p)\n"
        "Device is %s using a Kernel PlugIn driver (%s)\n", hDev,
        (WDC_IS_KP(hDev))? "" : "not" , KP_VRTX5_DRIVER_NAME);
*/
    return hDev;

Error:    
    if (hDev)
        VIRTEX5_DeviceClose(hDev);
    else
        free(pDevCtx);
    
    return NULL;
}

BOOL CVirtex5BMD::VIRTEX5_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PVIRTEX5_DEV_CTX pDevCtx;
    
    //TraceLog("VIRTEX5_DeviceClose entered. Device handle: 0x%p\n", hDev);

    if (!hDev)
    {
//        sprintf(m_errMsg, "VIRTEX5_DeviceClose: Error - NULL device handle\n");
        return FALSE;
    }

    pDevCtx = (PVIRTEX5_DEV_CTX)WDC_GetDevContext(pDev);
    
    // Disable interrupts 
    if (WDC_IntIsEnabled(hDev))
    {
        dwStatus = VIRTEX5_IntDisable(hDev);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
//            sprintf(m_errMsg, "Failed disabling interrupts. Error 0x%lx - %s\n", dwStatus,
  //              Stat2Str(dwStatus));
			m_errCode = dwStatus;
        }
    }

    // Close the device 
    dwStatus = WDC_PciDeviceClose(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
//        sprintf(m_errMsg, "Failed closing a WDC device handle (0x%p). Error 0x%lx - %s\n",
  //          hDev, dwStatus, Stat2Str(dwStatus));
		m_errCode = dwStatus;
    }

    // Free VIRTEX5 device context memory 
    if (pDevCtx)
        free(pDevCtx);
    
    return (WD_STATUS_SUCCESS == dwStatus);
}


DWORD CVirtex5BMD::VIRTEX5_IntDisable(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    if (!IsValidDevice(pDev, "VIRTEX5_IntDisable")) {
//		sprintf(m_errMsg, "VIRTEX5_IntDisable: NULL device %s\n", !pDev ? "handle" : "context");
        return WD_INVALID_PARAMETER;
	}

    if (!WDC_IntIsEnabled(hDev))
    {
//        sprintf(m_errMsg, "Interrupts are already disabled ...\n");
		m_errCode = WD_OPERATION_ALREADY_DONE;
        return WD_OPERATION_ALREADY_DONE;
    }

    // Disable the interrupts 
    dwStatus = WDC_IntDisable(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
//        sprintf(m_errMsg, "Failed disabling interrupts. Error 0x%lx - %s\n",
  //          dwStatus, Stat2Str(dwStatus));
		m_errCode = WD_OPERATION_ALREADY_DONE;
    }

    return dwStatus;
}


void CVirtex5BMD::DIAG_DMAClose(WDC_DEVICE_HANDLE hDev, PDIAG_DMA pDma)
{
    DWORD dwStatus;

    if (!pDma)
        return;
    
    if (VIRTEX5_IntIsEnabled(hDev))
    {
        dwStatus = VIRTEX5_IntDisable(hDev);
		m_errCode = dwStatus;
    }

    if (pDma->hDma)
    {
        VIRTEX5_DMAClose(pDma->hDma);
    }
    
    BZERO(*pDma);
}


void CVirtex5BMD::VIRTEX5_DMAClose(VIRTEX5_DMA_HANDLE hDma)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    PVIRTEX5_DEV_CTX pDevCtx;

//    TraceLog("VIRTEX5_DMAClose entered.");
    
    if (!IsValidDmaHandle(hDma, "VIRTEX5_DMAClose"))
        return;
    
//    TraceLog(" Device handle: 0x%p\n", hDma->hDev);
    
    if (hDma->pDma)
    {
        dwStatus = WDC_DMABufUnlock(hDma->pDma);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
//            sprintf(m_errMsg, "VIRTEX5_DMAClose: Failed unlocking DMA buffer. "
  //              "Error 0x%lX - %s\n", dwStatus, Stat2Str(dwStatus));
			m_errCode = dwStatus;
        }
    }
    else
    {
        //TraceLog("VIRTEX5_DMAClose: DMA is not currently open ... "
         //   "(device handle 0x%p, DMA handle 0x%p)\n", hDma->hDev, hDma);
    }
     
    pDevCtx = (PVIRTEX5_DEV_CTX)WDC_GetDevContext(hDma->hDev);
    pDevCtx->hDma = NULL;
    pDevCtx->pBuf = NULL;

    free(hDma);
}

// -----------------------------------------------
//   Direct Memory Access (DMA)
//   ----------------------------------------------- 
BOOL CVirtex5BMD::IsValidDmaHandle(VIRTEX5_DMA_HANDLE hDma, CHAR *sFunc)
{
    BOOL ret = (hDma && IsValidDevice((PWDC_DEVICE)hDma->hDev, sFunc)) ? TRUE : FALSE;

 //   if (!hDma)
   //     sprintf_s(m_errMsg, "%s: NULL DMA Handle\n", sFunc);

    return ret;
}


BOOL CVirtex5BMD::VIRTEX5_IntIsEnabled(WDC_DEVICE_HANDLE hDev)
{
    if (!IsValidDevice((PWDC_DEVICE)hDev, "VIRTEX5_IntIsEnabled")) {
//		sprintf(m_errMsg, "VIRTEX5_IntIsEnabled: NULL device %s\n", !hDev ? "handle" : "context");
        return FALSE;
	}

    return WDC_IntIsEnabled(hDev);
}


DWORD CVirtex5BMD::VIRTEX5_LibUninit(void)
{
    DWORD dwStatus;

    // Uninit the WDC library and close the handle to WinDriver 
    dwStatus = WDC_DriverClose();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
//        sprintf(m_errMsg, "Failed to uninit the WDC library. Error 0x%lx - %s\n",
  //          dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}



// write/read pre-defined run-time or PCI configuration registers
BOOL CVirtex5BMD::ReadWriteReg(WDC_DEVICE_HANDLE hDev, 
							   WDC_REG *pRegs, 
							   DWORD dwReg,
							   DWORD dwNumRegs, 
							   WDC_DIRECTION direction, 
							   BOOL fPciCfg,
							   UINT64 *u64Rdata, UINT u64Wdata)
{
    DWORD  dwStatus; 
    WDC_REG *pReg; 
    BYTE   bData = 0;
    WORD   wData = 0;
    UINT32 u32Data = 0;
    UINT64 u64Data = 0;
    
    if (!hDev)
    {
//        sprintf(m_errMsg, "WDC_DIAG_ReadWriteReg: Error - NULL WDC device handle");
        return FALSE;
    }

    if (!dwNumRegs || !pRegs)
    {
//        sprintf(m_errMsg, "WDC_DIAG_ReadWriteReg: %s",
//						!dwNumRegs ? "No registers to read/write (dwNumRegs == 0)" :
//						"Error - NULL registers pointer");
        return FALSE;
    }

    // Read/write register 
    if (dwReg > dwNumRegs)
    {
//        sprintf(m_errMsg, "Selection (%ld) is out of range (1 - %ld)", dwReg, dwNumRegs);
        return FALSE;
    }

    pReg = &pRegs[dwReg - 1];


    if ( ((WDC_READ == direction) && (WDC_WRITE == pReg->direction)) ||
        ((WDC_WRITE == direction) && (WDC_READ == pReg->direction)))
    {
//        sprintf(m_errMsg, "Error - you have selected to %s a %s-only register\n",
  //          (WDC_READ == direction) ? "read from" : "write to",
    //        (WDC_WRITE == pReg->direction) ? "write" : "read");
        return FALSE;
    }
/*
    if ((WDC_WRITE == direction) &&
        !WDC_DIAG_InputWriteData((WDC_SIZE_8 == pReg->dwSize) ? (PVOID)&bData :
        (WDC_SIZE_16 == pReg->dwSize) ? (PVOID)&wData :
        (WDC_SIZE_32 == pReg->dwSize) ? (PVOID)&u32Data : (PVOID)&u64Data,
        pReg->dwSize))
    {
        goto Exit;
    }
*/
    switch (pReg->dwSize)
    {
    case WDC_SIZE_8:
        if (WDC_READ == direction) {
            dwStatus = fPciCfg ? WDC_PciReadCfg8(hDev, pReg->dwOffset, &bData) :
                WDC_ReadAddr8(hDev, pReg->dwAddrSpace, pReg->dwOffset, &bData);
			*u64Rdata = bData;
        } else {
			bData = (BYTE)u64Wdata;
            dwStatus = fPciCfg ? WDC_PciWriteCfg8(hDev, pReg->dwOffset, bData) :
                WDC_WriteAddr8(hDev, pReg->dwAddrSpace, pReg->dwOffset, bData);
		}
        break;
    case WDC_SIZE_16:
        if (WDC_READ == direction) {
            dwStatus = fPciCfg ? WDC_PciReadCfg16(hDev, pReg->dwOffset, &wData) :
                WDC_ReadAddr16(hDev, pReg->dwAddrSpace, pReg->dwOffset, &wData);
			*u64Rdata = wData;
        } else {
			wData = (WORD)u64Wdata;
            dwStatus = fPciCfg ? WDC_PciWriteCfg16(hDev, pReg->dwOffset, wData) :
                WDC_WriteAddr16(hDev, pReg->dwAddrSpace, pReg->dwOffset, wData);
		}
        break;
    case WDC_SIZE_32:
        if (WDC_READ == direction) {
            dwStatus = fPciCfg ? WDC_PciReadCfg32(hDev, pReg->dwOffset, &u32Data) :
                WDC_ReadAddr32(hDev, pReg->dwAddrSpace, pReg->dwOffset, &u32Data);
			*u64Rdata = u32Data;
        } else {
			u32Data = (UINT32)u64Wdata;
            dwStatus = fPciCfg ? WDC_PciWriteCfg32(hDev, pReg->dwOffset, u32Data) :
                WDC_WriteAddr32(hDev, pReg->dwAddrSpace, pReg->dwOffset, u32Data);
		}
        break;
    case WDC_SIZE_64:
        if (WDC_READ == direction) {
            dwStatus = fPciCfg ? WDC_PciReadCfg64(hDev, pReg->dwOffset, &u64Data) :
                WDC_ReadAddr64(hDev, pReg->dwAddrSpace, pReg->dwOffset, &u64Data);
			*u64Rdata = u64Data;
        } else {
			u64Data = u64Wdata;
            dwStatus = fPciCfg ? WDC_PciWriteCfg64(hDev, pReg->dwOffset, u64Data) :
                WDC_WriteAddr64(hDev, pReg->dwAddrSpace, pReg->dwOffset, u64Data);
		}
        break;
    default:
//        sprintf(m_errMsg, "Invalid register size (%ld)\n", pReg->dwSize);
        return FALSE;
    }

    if (WD_STATUS_SUCCESS != dwStatus)
    {
//        sprintf(m_errMsg, "Failed %s %s. Error 0x%lx - %s\n",
  //          (WDC_READ == direction) ? "reading data from" : "writing data to",
    //        pReg->sName, dwStatus, Stat2Str(dwStatus));
		return FALSE;
    }

	return TRUE;
}


UINT32 CVirtex5BMD::VIRTEX5_ReadReg32(WDC_DEVICE_HANDLE hDev, DWORD offset)
{
    UINT32 data;

    WDC_ReadAddr32(hDev, VIRTEX5_SPACE, offset, &data);
    return data;
}


WORD CVirtex5BMD::code2size(BYTE bCode)
{
    if (bCode > 0x05)
        return 0;
    return (WORD)(128 << bCode);
}


WORD CVirtex5BMD::DMAGetMaxPacketSize(WDC_DEVICE_HANDLE hDev, BOOL fIsRead)
{
    UINT32 dltrsstat;
    WORD wByteCount;
	PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    //if (!IsValidDevice(hDev, "VIRTEX5_DMAGetMaxPacketSize"))
	if (!IsValidDevice(pDev, "VIRTEX5_DMAGetMaxPacketSize"))
        return 0;
    
    // Read encoded max payload sizes 
    dltrsstat = VIRTEX5_ReadReg32(hDev, VIRTEX5_DLTRSSTAT_OFFSET);
    
    // Convert encoded max payload sizes into byte count 
    if (fIsRead)
    {
        // bits 18:16 
        wByteCount = code2size((BYTE)((dltrsstat >> 16) & 0x7));
    }
    else
    {
        /* bits 2:0 */
        WORD wMaxCapPayload = code2size((BYTE)(dltrsstat & 0x7)); 
        /* bits 10:8 */
        WORD wMaxProgPayload = code2size((BYTE)((dltrsstat >> 8) & 0x7));

        wByteCount = MIN(wMaxCapPayload, wMaxProgPayload);
    }

    return wByteCount;
}


DWORD CVirtex5BMD::VIRTEX5_DMAOpen(WDC_DEVICE_HANDLE hDev, PVOID *ppBuf, DWORD dwOptions,
									DWORD dwBytes, VIRTEX5_DMA_HANDLE *ppDmaHandle)
{
    DWORD dwStatus;
    PVIRTEX5_DEV_CTX pDevCtx;
    VIRTEX5_DMA_HANDLE pVIRTEX5Dma = NULL;

    if (!IsValidDevice((PWDC_DEVICE)hDev, "VIRTEX5_DMAOpen"))
        return WD_INVALID_PARAMETER;
    
    if (!ppBuf)
    {
//        sprintf(m_errMsg, "VIRTEX5_DMAOpen: Error - NULL DMA buffer pointer");
        return WD_INVALID_PARAMETER;
    }

    pDevCtx = (PVIRTEX5_DEV_CTX)WDC_GetDevContext(hDev);
    if (pDevCtx->hDma)
    {
//        sprintf(m_errMsg, "VIRTEX5_DMAOpen: Error - DMA already open");
        return WD_TOO_MANY_HANDLES;
    }

    pVIRTEX5Dma = (VIRTEX5_DMA_STRUCT *)calloc(1, sizeof(VIRTEX5_DMA_STRUCT));
    if (!pVIRTEX5Dma)
    {
//        sprintf(m_errMsg, "VIRTEX5_DMAOpen: Failed allocating memory for VIRTEX5 DMA struct");
        return WD_INSUFFICIENT_RESOURCES;
    }
    pVIRTEX5Dma->hDev = hDev;
    
    // Allocate and lock a DMA buffer 
    dwStatus = WDC_DMAContigBufLock(hDev, ppBuf, dwOptions, dwBytes, &pVIRTEX5Dma->pDma);

    if (WD_STATUS_SUCCESS != dwStatus) 
    {
//        sprintf(m_errMsg, "VIRTEX5_DMAOpen: Failed locking a DMA buffer. Error 0x%lx", dwStatus);
        goto Error;
    }
    
    *ppDmaHandle = (VIRTEX5_DMA_HANDLE)pVIRTEX5Dma;

    // update the device context 
    pDevCtx->hDma = pVIRTEX5Dma;
    pDevCtx->pBuf = *ppBuf;
    
    return WD_STATUS_SUCCESS;

Error:
    if (pVIRTEX5Dma)
        VIRTEX5_DMAClose((VIRTEX5_DMA_HANDLE)pVIRTEX5Dma);
    
    return dwStatus;
}


void CVirtex5BMD::VIRTEX5_WriteReg32(WDC_DEVICE_HANDLE hDev, DWORD offset, UINT32 data)
{
    WDC_WriteAddr32(hDev, VIRTEX5_SPACE, offset, data);
}

void CVirtex5BMD::VIRTEX5_WriteReg16(WDC_DEVICE_HANDLE hDev, DWORD offset, WORD data)
{
    WDC_WriteAddr16(hDev, VIRTEX5_SPACE, offset, data);
}


// Enable DMA interrupts 
void CVirtex5BMD::VIRTEX5_DmaIntEnable(WDC_DEVICE_HANDLE hDev, BOOL fIsRead)
{
    UINT32 ddmacr = VIRTEX5_ReadReg32(hDev, VIRTEX5_DDMACR_OFFSET);

    if (!IsValidDevice((PWDC_DEVICE)hDev, "VIRTEX5_DmaIntEnable"))
        return;

    ddmacr &= fIsRead ? ~BIT23 : ~BIT7;
    VIRTEX5_WriteReg32(hDev, VIRTEX5_DDMACR_OFFSET, ddmacr);
}

DWORD CVirtex5BMD::VIRTEX5_IntEnable(WDC_DEVICE_HANDLE hDev, VIRTEX5_INT_HANDLER funcIntHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PVIRTEX5_DEV_CTX pDevCtx;

    //TraceLog("VIRTEX5_IntEnable entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "VIRTEX5_IntEnable"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PVIRTEX5_DEV_CTX)WDC_GetDevContext(pDev);

    /* Check if interrupts are already enabled */
    if (WDC_IntIsEnabled(hDev))
    {
//        sprintf(m_errMsg, "Interrupts are already enabled ...");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Message Signalled interrupts need not be acknowledged,
     * so no transfer commands are needed */
    #define NUM_TRANS_CMDS 0

    /* Store the diag interrupt handler routine, which will be executed by
       VIRTEX5_IntHandler() when an interrupt is received */
    pDevCtx->funcDiagIntHandler = funcIntHandler;
    
    /* Enable the interrupts */
    dwStatus = WDC_IntEnable(hDev, NULL, NUM_TRANS_CMDS, 0, VIRTEX5_IntHandler, (PVOID)pDev, WDC_IS_KP(hDev));

    if (WD_STATUS_SUCCESS != dwStatus)
    {
//        sprintf(m_errMsg, "Failed enabling interrupts. Error 0x%lx", dwStatus);
        return dwStatus;
    }

    /* TODO: You can add code here to write to the device in order
             to physically enable the hardware interrupts */

    //TraceLog("VIRTEX5_IntEnable: Interrupts enabled\n");

    return WD_STATUS_SUCCESS;
}

// Disable DMA interrupts 
void CVirtex5BMD::VIRTEX5_DmaIntDisable(WDC_DEVICE_HANDLE hDev, BOOL fIsRead)
{
    UINT32 ddmacr = VIRTEX5_ReadReg32(hDev, VIRTEX5_DDMACR_OFFSET);

    if (!IsValidDevice((PWDC_DEVICE)hDev, "VIRTEX5_DmaIntDisable"))
        return;

    ddmacr |= fIsRead ? BIT23 : BIT7;
    VIRTEX5_WriteReg32(hDev, VIRTEX5_DDMACR_OFFSET, ddmacr);
}


void CVirtex5BMD::VIRTEX5_DMADevicePrepare(VIRTEX5_DMA_HANDLE hDma, BOOL fIsRead, WORD wSize,
    WORD wCount, UINT32 u32Pattern, BOOL fEnable64bit, BYTE bTrafficClass)
{
    UINT32 tlps;
    UINT32 LowerAddr;
    BYTE UpperAddr;
    WDC_DEVICE_HANDLE hDev;
    PVIRTEX5_DEV_CTX pDevCtx;

    if (!IsValidDmaHandle(hDma, "VIRTEX5_DMADevicePrepare"))
        return;

    hDev = hDma->hDev;

    /* Assert Initiator Reset */
    VIRTEX5_WriteReg32(hDev, VIRTEX5_DSCR_OFFSET, 0x1);

    /* De-assert Initiator Reset */
    VIRTEX5_WriteReg32(hDev, VIRTEX5_DSCR_OFFSET, 0x0);

    LowerAddr = (UINT32)hDma->pDma->Page[0].pPhysicalAddr;
    UpperAddr = (BYTE)((hDma->pDma->Page[0].pPhysicalAddr >> 32) & 0xFF);

    tlps = (wSize & 0x1FFF) | /* tlps[0:12] - DMA TLP size */
        ((bTrafficClass & 0x7) << 16) | /* tlps[16:18] -
                                           DMA TLP Traffic Class */
        (fEnable64bit ? BIT19 : 0) | /* tlps[19] enable 64bit TLP */
        (UpperAddr << 24); /* tlps[24:31] - upper bits 33:39 of DMA addr*/

    if (fIsRead)
    {
        /* Set lower 32bit of DMA address */
        VIRTEX5_WriteReg32(hDev, VIRTEX5_RDMATLPA_OFFSET, LowerAddr);

        /* Set size, traffic class, 64bit enable, upper 8bit of DMA address */
        VIRTEX5_WriteReg32(hDev, VIRTEX5_RDMATLPS_OFFSET, tlps);

        /* Set TLP count */
        VIRTEX5_WriteReg16(hDev, VIRTEX5_RDMATLPC_OFFSET, wCount);

        /* Set Read DMA pattern */
        VIRTEX5_WriteReg32(hDev, VIRTEX5_RDMATLPP_OFFSET, u32Pattern);
    }
    else
    {
        /* Set lower 32bit of DMA address */
        VIRTEX5_WriteReg32(hDev, VIRTEX5_WDMATLPA_OFFSET, LowerAddr);

        /* Set size, traffic class, 64bit enable, upper 8bit of DMA address */
        VIRTEX5_WriteReg32(hDev, VIRTEX5_WDMATLPS_OFFSET, tlps);

        /* Set TLP count */
        VIRTEX5_WriteReg16(hDev, VIRTEX5_WDMATLPC_OFFSET, wCount);

        /* Set Read DMA pattern */
        VIRTEX5_WriteReg32(hDev, VIRTEX5_WDMATLPP_OFFSET, u32Pattern);
    }

    pDevCtx = (PVIRTEX5_DEV_CTX)WDC_GetDevContext(hDev);
    pDevCtx->fIsRead = fIsRead;
    pDevCtx->u32Pattern = u32Pattern;
    pDevCtx->dwTotalCount = (DWORD)wCount * (DWORD)wSize;
}

void CVirtex5BMD::AppInitStimulus(WDC_DEVICE_HANDLE hDev, VIDEO_INFO VideoInfo, int nx, int ny)
{
	// initialize video information for kernel plugin
	DWORD dwStatus, dwResult;

	dwStatus = WDC_CallKerPlug(hDev, KP_VRTX5_MSG_DATA, &VideoInfo, &dwResult);

	// write a temporary stimulus pattern with 7x7 pixels square
	UINT32 regData2;

	// turn on AOM signal phsically
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, BIT0+BIT1);
	//regData2 = ny;
	//regData2 = ny/2;
	//regData2 = regData2 << 24;
	//regData2 = regData2 + 0xffffffff;
//	regData2 = 0x3fffffff;
//	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER,  regData2);
	regData2 = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER);
	regData2 = (regData2 >> 30);
	regData2 = (regData2 << 30);
	regData2 = regData2 + 0x3fffffff;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER,  regData2);

	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_DATA, 0);
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND, BIT3+BIT11+BIT19+BIT27);

	regData2 = 0xff;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER2, regData2);
//	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2, BIT3+BIT11+BIT19+BIT27);
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2, BIT3+BIT11);
}

void CVirtex5BMD::AppAOM_On(WDC_DEVICE_HANDLE hDev)
{
	UINT32 regData;

	regData = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	regData = regData >> 1;
	regData = regData << 1;
	regData = regData + BIT0;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regData);
}

void CVirtex5BMD::AppAOM_Off(WDC_DEVICE_HANDLE hDev)
{
	UINT32 regData;

	regData = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	regData = regData >> 1;
	regData = regData << 1;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regData);
}

void CVirtex5BMD::AppUpdateSampleInfo(WDC_DEVICE_HANDLE hDev, VIDEO_INFO VideoInfo)
{
	UINT32   reg32, regTemp;

	// update run-time registers of ADC/DAC and the PCIe block
	// 1. update PCIe run-time register with on-board memory starting address and ending address
/*	regTemp  = VideoInfo.line_start_addr;
	reg32    = regTemp << 16;
	reg32    = reg32 | VideoInfo.line_end_addr;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, reg32);
*/
	// 2. update PCIe run-time register with line spacing, image width, and image height
	regTemp  = VideoInfo.line_spacing;
	reg32    = regTemp << 24;
	regTemp  = VideoInfo.img_width << 12;
	reg32    = reg32 | regTemp;
	reg32    = reg32 | VideoInfo.img_height;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_IMAGESIZE_OFFSET, reg32);

	// 3. update PCIe run-time register with pixel offset, line offset and end line index
//	regTemp  = VideoInfo.offset_pixel;
//	reg32    = regTemp << 24;
//	regTemp  = VideoInfo.offset_line;
//	regTemp  = regTemp << 16;
	regTemp  = VideoInfo.offset_line;
	reg32    = regTemp << 24;
	regTemp  = VideoInfo.offset_pixel;
	regTemp  = regTemp << 14;
	reg32    = reg32 | regTemp;
//	reg32    = reg32 | BIT14;			// force AOSLO to be connected to Red channel of the ADC chip
	reg32    = reg32 | BIT13 | BIT12;		// choose red channel
	reg32    = reg32 - BIT12;
	reg32    = reg32 | VideoInfo.end_line_ID;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_IMAGEOS_OFFSET, reg32);
}


//    4.1 Brightness of image
//    4.2 Constrast of image
//    4.3 H-sync triggering edge
//    4.4 V-sync triggering edge
//    4.5 Frequency of external pixel clock
int CVirtex5BMD::UpdateRuntimeRegisters(WDC_DEVICE_HANDLE hDev, unsigned char *m_adcVals)
{
	// Update ADC registers
	// ReadWriteI2CRegister(hDev, BOOL bRead, UINT32 slave_addr, BYTE m_regAddr, BYTE regValI, BYTE *regValO)
	BYTE relOut, relIn;
	int  ret;

	// PLL dividor, force pixel clock to 20MHz
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x01, m_adcVals[0x01], &relOut);
	Sleep(1);
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x02, m_adcVals[0x02], &relOut);
	Sleep(1);
	// VCO/CPMP
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x03, m_adcVals[0x03], &relOut);
	Sleep(1);
	// Phase adjustment
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x04, m_adcVals[0x04], &relOut);
	Sleep(1);
	// contrast
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x05, m_adcVals[0x05], &relOut);
	Sleep(1);
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x06, m_adcVals[0x06], &relOut);
	Sleep(1);
	// brightness
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x0B, m_adcVals[0x0B], &relOut);
	Sleep(1);
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x0C, m_adcVals[0x0C], &relOut);
	Sleep(1);
	// Hsync control
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x12, m_adcVals[0x12], &relOut);
	Sleep(1);
	//Vsync control
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x14, m_adcVals[0x14], &relOut);
	Sleep(1);
	// External clock control
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x18, m_adcVals[0x18], &relOut);
	Sleep(1);
	// External clamping control
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x4c000000, 0x1B, m_adcVals[0x1B], &relOut);
	Sleep(1);


	// Update DAC registers
	// 1. set power management register to enable all DAC's.
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x49, 0x00, &relOut);
	Sleep(1);
	// 2. set dac control register 21h[0] = '0'
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x21, 0x00, &relOut);
	Sleep(1);
	// 3. set sense bit to '1', connection detect
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x21, 0x01, &relOut);
	Sleep(1);
	// 4. reset sense bit to '0', connection detect
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x20, 0x00, &relOut);
	Sleep(1);
	// 5. set dac control register 21h[0] = '0'
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x21, 0x01, &relOut);
	// clock mode
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x1C, 0x00, &relOut);
	Sleep(1);
	// input clock
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x1D, 0x48, &relOut);
	Sleep(1);
	// input data format
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x1F, 0x80, &relOut);
	Sleep(1);


	// Update DAC registers
	// 1. set power management register to enable all DAC's.
	ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x49, 0x00, &relOut);
	Sleep(1);
	// 2. set dac control register 21h[0] = '0'
	ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x21, 0x00, &relOut);
	Sleep(1);
	// 3. set sense bit to '1', connection detect
	ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x20, 0x01, &relOut);
	Sleep(1);
	// 4. reset sense bit to '0', connection detect
	ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x20, 0x00, &relOut);
	Sleep(1);
	// 5. set dac control register 21h[0] = '0'
	ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x21, 0x01, &relOut);
	// 6. read sense bit
	ReadWriteI2CRegister(hDev, TRUE, 0x76000000, 0x20, 0x01, &relOut);
	Sleep(1);
	relIn = relOut | BIT0;
	// 7. set sense bit to '1'
	ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x20, relIn, &relOut);
	Sleep(1);
	// 8. reset sense bit to '0'
	relIn  = relIn - BIT0;
	ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x20, relIn, &relOut);
	Sleep(1);
	// clock mode
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x1C, 0x00, &relOut);
	Sleep(1);
	// input clock
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x1D, 0x48, &relOut);
	Sleep(1);
	// input data format
	ret = ReadWriteI2CRegister(hDev, FALSE, 0x76000000, 0x1F, 0x80, &relOut);
	Sleep(1);

	return 0;
}


int CVirtex5BMD::ReadWriteI2CRegister(WDC_DEVICE_HANDLE hDev, 
									  BOOL bRead, 
									  UINT32 slave_addr, 
									  BYTE m_regAddr, 
									  BYTE regValI, BYTE *regValO)
{
	UINT32  regData, regAddr, regVal;
	int     ret;
	BYTE    reg_val;
//	DWORD   regIdx;

	regData = VIRTEX5_ReadReg32(hDev, VIRTEX5_I2CINFO_OFFSET);

	// clear the high 30 bits
	regData = regData << 30;
	regData = regData >> 30;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_I2CINFO_OFFSET, regData);

	// move "register address" to bits 23-16
	regAddr = m_regAddr;
	regAddr = regAddr << 16;

	// move "register address" to bits 15-8
	regVal = regValI;
	regVal = regVal << 8;

	// add I2C slave address, I2C register address and I2C register value to the PCIe register data
	regData |= slave_addr;
	regData |= regAddr;
	regData |= regVal;

	if (bRead) {
		// Set bit 7 and clear bit 6 and bit 5 of the command
		regData |= BIT7;
		regData = regData&BIT6 ? regData-BIT6 : regData;
		regData = regData&BIT5 ? regData-BIT5 : regData;
	} else {
		// Set bit 4 and clear bit 3 and bit 2 of the command
		regData |= BIT4;
		regData = regData&BIT3 ? regData-BIT3 : regData;
		regData = regData&BIT2 ? regData-BIT2 : regData;
	}

	if (bRead)
		ret = I2CController(hDev, regData, &reg_val, TRUE);
	else
		ret = I2CController(hDev, regData, &reg_val, FALSE);

	if (ret == 0) 
		*regValO = reg_val;
	else
		*regValO = 0xff;

	return ret;

}

int CVirtex5BMD::I2CController(WDC_DEVICE_HANDLE hDev, UINT32 regDataW, BYTE *reg_val, BOOL bI2Cread)
{
	BOOL    bI2Cdone;
	int     timeout;
	UINT32  regDataR, regVal, doneBit, errBit;
//	CString msg;
//	DWORD   regIdx;

	if (bI2Cread) {
		doneBit = BIT5;
		errBit  = BIT6;
	} else {
		doneBit = BIT2;
		errBit  = BIT3;
	}

	VIRTEX5_WriteReg32(hDev, VIRTEX5_I2CINFO_OFFSET, regDataW);
	bI2Cdone = FALSE;
	timeout  = 0;
	do {
		Sleep(1);
		regDataR = VIRTEX5_ReadReg32(hDev, VIRTEX5_I2CINFO_OFFSET);
//		msg.Format("0x%8X", regDataR);

		// I2C bus controller has finished data access
		if (regDataR & doneBit) {
			bI2Cdone = TRUE;
			//if (regDataR & BIT6) {
			if (regDataR & errBit) {
				*reg_val = 0xff;
				return 2;
			} else {
				regVal = regDataR << 16;
				regVal = regVal >> 24;
//				msg.Format("0x%8X", regVal);
				*reg_val = (BYTE)regVal;
			}
//			msg.Format("0x%8X", regDataR);
		}
		timeout ++;
		if (timeout > 10) {
			bI2Cdone = TRUE;
			*reg_val = 0xff;
			return 3;
		}
	} while (bI2Cdone == FALSE);

	regDataR = regDataR << 30;
	regDataR = regDataR >> 30;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_I2CINFO_OFFSET, regDataR);
//	msg.Format("0x%8X", regDataR);

	return 0;

}

/*
void CVirtex5BMD::AppUpdateRedLaser(WDC_DEVICE_HANDLE hDev, BYTE power)
{
	UINT32  regR, regW;

	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER);
	regW = regR >> 24;
	regW = regW << 24; // reserve bits 24-31
	regR = regR << 16; 
	regR = regR >> 16; // reserve bits 0-15
	regW = regW + regR;
	regR = power;
	regR = regR << 16;
	regW = regW + regR;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER, regW);
}
*/
void CVirtex5BMD::AppUpdateIRLaser(WDC_DEVICE_HANDLE hDev, BYTE power)
{
	UINT32  regR, regW;

	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER2);
	regW = regR >> 8;
	regW = regW << 8;  // reserve bits 8-31
	regR = power;
	regW = regW + regR;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER2, regW);
}
/*
void CVirtex5BMD::AppShiftRedX(WDC_DEVICE_HANDLE hDev, int clocks)
{
	UINT32  regR, regW, high;

	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
	high = regR;
	high = high >> 25;	// reserve high 4 bits
	high = high << 25;
	regR = regR << 16;
	regR = regR >> 16; // reserve lower 16 bits
	if (clocks>=0) {
		regW = clocks;
	} else {
		regW = BIT8 - clocks;
	}
	regW = regW << 16; // move left 16 bits
	regW = regW + regR + high;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, regW);


//	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
//	char msg[80];
//	sprintf(msg, "%08x", regR);
}

void CVirtex5BMD::AppShiftGreenX(WDC_DEVICE_HANDLE hDev, int clocks)
{
	UINT32  regR, regW;

	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
	if (clocks>=0) {
		regR = regR | BIT25;
		regR = regR - BIT25;
		regW = clocks << 24;
	} else {
		regR = regR | BIT25;
		regW = (-clocks) << 24;
	}
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, regR);


	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER2);
	regR = regR << 8;
	regR = regR >> 8;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER2, regW+regR);

//	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
//	char msg[80];
//	sprintf(msg, "%08x", regR);
}
*/
void CVirtex5BMD::AppIRAOM_On(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	reg = reg | BIT1; 
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, reg);		// bit 15 controls stimulus pattern write/read
}

void CVirtex5BMD::AppIRAOM_Off(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	reg = reg | BIT1;
	reg = reg - BIT1;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, reg);		// bit 15 controls IR stimulus, bit 14 controls stimulus pattern write/read
}

void CVirtex5BMD::AppSamplePatch(WDC_DEVICE_HANDLE hDev, VIDEO_INFO VideoInfo, UINT32 lineID)
{
	UINT32 NextLineIndex;

	WDC_ReadAddr32(hDev, VIRTEX5_SPACE, VIRTEX5_IMAGEOS_OFFSET, &NextLineIndex);

	if (lineID == VideoInfo.img_height) {
		//StartAddrIndex = StartAddrIndex >> 16;
		//StartAddrIndex = StartAddrIndex << 16;
		NextLineIndex  = NextLineIndex >> 12;
		NextLineIndex  = NextLineIndex << 12;
		NextLineIndex += VideoInfo.line_spacing;
	} else {
		//StartAddrIndex += VideoInfo.addr_interval;
		NextLineIndex  += VideoInfo.line_spacing;
	}
	WDC_WriteAddr32(hDev, VIRTEX5_SPACE, VIRTEX5_IMAGEOS_OFFSET, NextLineIndex);
}


// this stimulus pattern is not stretched
// channelID: 0-both channels, 1-red, 2-ir
void CVirtex5BMD::AppLoadStimulus(WDC_DEVICE_HANDLE hDev, unsigned short *buffer, int nx, int ny, int channelID)
{
	UINT32 regData1, reg1, reg2, reg3, reg4, address, addr16;
	int    i, j, dword;
	short  word[2];
	BYTE   bytes[4];

	// write stimulus boundary
	if (channelID == 1) {
		// red channel
		regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND);
		regData1 = regData1 >> 16;
		regData1 = regData1 << 16;	// save high 16 bits for ir channel
		reg1 = nx/2;
		reg2 = reg1 << 8;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND,  regData1+reg1+reg2);
	} else if (channelID == 2) {
		// green channel
		regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND);
		regData1 = regData1 << 16;
		regData1 = regData1 >> 16;	// save low 16 bits for red channel
		reg1 = nx/2;
		reg3 = reg1 << 16;
		reg4 = reg1 << 24;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND,  regData1+reg3+reg4);
	} else if (channelID == 3) {
		// ir channel
		regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2);
		regData1 = regData1 >> 16;
		regData1 = regData1 << 16;	// reserve high 16 bits for mirror motion
		reg1 = nx/2;
		reg2 = reg1 << 8;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2, regData1+reg1+reg2);
	} else {
		// both channels
		reg1 = nx/2;
		reg2 = reg1 << 8;
		reg3 = reg1 << 16;
		reg4 = reg1 << 24;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND,  reg1+reg2+reg3+reg4);
		//VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2, reg1+reg2+reg3+reg4);
		regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2);
		regData1 = regData1 >> 16;
		regData1 = regData1 << 16;	// reserve high 16 bits for mirror motion
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2, regData1+reg1+reg2);
	}

	// set ram status to "read/write"
	switch (channelID) {
	case 0:
		address  = BIT19 | BIT18;
		break;
	case 1:
		address  = BIT18;
		break;
	case 2:
		address  = BIT19;
		break;
	case 3:
		address  = BIT14;
		break;
	default:
		address  = BIT19 | BIT18;
		break;
	}
/*
	if (channelID == 1 || channelID == 2 || channelID == 3) {
		// clear previous data on the block ram
		dword = 16384;
		for (i = 0; i < dword; i ++) {
			addr16 = address + i;
			// write address
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, addr16);
			// write 32-bit data
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_DATA, 0);
		}
	}
*/
	// upload stimulus pattern to FPGA
	if (channelID == 1 || channelID == 2 || channelID == 0) {
		dword = nx*ny/2 + nx*3;
		reg1 = reg2 = reg3 = 0;
		dword = (dword>=16384) ? 16384 : dword;
		for (i = 0; i < dword; i ++) {
			for (j = 0; j < 2; j ++) {
				if (2*i+j < nx*ny)
					word[j] = buffer[2*i+j];
				else 
					word[j] = 0;
			}
			reg1 = word[0];
			reg2 = word[1] << 16;

			regData1 = reg1 + reg2;// + reg3 + reg4;
			
			addr16 = address + i;
			// write address
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, addr16);
			// write 32-bit data
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_DATA, regData1);
		}
	} 
	
	if (channelID == 3 || channelID == 0) {
		address  = BIT14;

		dword = nx*ny/4+nx*5;
		reg1 = reg2 = reg3 = 0;
		dword = (dword>=8192) ? 8192 : dword;

		for (i = 0; i < dword; i ++) {
			for (j = 0; j < 4; j ++) {
				if (4*i+j < nx*ny)
					bytes[j] = (BYTE)(buffer[4*i+j]>>6);
				else 
					bytes[j] = 0;
			}
			reg1 = bytes[0];
			reg2 = bytes[1]; reg2 = reg2 << 8;
			reg3 = bytes[2]; reg3 = reg3 << 16;
			reg4 = bytes[3]; reg4 = reg4 << 24;
			regData1 = reg1 + reg2 + reg3 + reg4;
			
			addr16 = address + i;
			// write address
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, addr16);
			// write 32-bit data
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_DATA, regData1);
		}
	}

	// set ram status to "read only"
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);
}

void CVirtex5BMD::AppLoadStimulus8bits(WDC_DEVICE_HANDLE hDev, unsigned short *buffer, int nx, int ny)
{
	UINT32 regData1, reg1, reg2, reg3, reg4, address, addr16;
	int    i, j, dword;
	BYTE   bytes[4];

	// ir channel
	regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2);
	regData1 = regData1 >> 16;
	regData1 = regData1 << 16;	// reserve high 16 bits for mirror motion
	reg1 = nx/2;
	reg2 = reg1 << 8;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2, regData1+reg1+reg2);

	address  = BIT14;

	dword = nx*ny/4+nx*5;
	reg1 = reg2 = reg3 = 0;
	dword = (dword>=8192) ? 8192 : dword;

	for (i = 0; i < dword; i ++) {
		for (j = 0; j < 4; j ++) {
			if (4*i+j < nx*ny)
				bytes[j] = (BYTE)(buffer[4*i+j]>>6);
			else 
				bytes[j] = 0;
		}
		reg1 = bytes[0];
		reg2 = bytes[1]; reg2 = reg2 << 8;
		reg3 = bytes[2]; reg3 = reg3 << 16;
		reg4 = bytes[3]; reg4 = reg4 << 24;
		regData1 = reg1 + reg2 + reg3 + reg4;
		
		addr16 = address + i;
		// write address
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, addr16);
		// write 32-bit data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_DATA, regData1);
	}

	// set ram status to "read only"
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);
}


void CVirtex5BMD::AppLoadStimulus14bits(WDC_DEVICE_HANDLE hDev, unsigned short *buffer, int nx, int ny, int channelID)
{
	UINT32 regData1, reg1, reg2, reg3, reg4, address, addr16;
	int    i, j, dword;
	short  word[2];

	// write stimulus boundary
	if (channelID == 1) {
		// red channel
		regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND);
		regData1 = regData1 >> 16;
		regData1 = regData1 << 16;	// save high 16 bits for ir channel
		reg1 = nx/2;
		reg2 = reg1 << 8;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND,  regData1+reg1+reg2);
	} else if (channelID == 2) {
		// green channel
		regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND);
		regData1 = regData1 << 16;
		regData1 = regData1 >> 16;	// save low 16 bits for red channel
		reg1 = nx/2;
		reg3 = reg1 << 16;
		reg4 = reg1 << 24;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND,  regData1+reg3+reg4);
	} else {
		// both channels
		reg1 = nx/2;
		reg2 = reg1 << 8;
		reg3 = reg1 << 16;
		reg4 = reg1 << 24;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND,  reg1+reg2+reg3+reg4);
	}

	// set ram status to "read/write"
	switch (channelID) {
	case 0:
		address  = BIT19 | BIT18;
		break;
	case 1:
		address  = BIT18;
		break;
	case 2:
		address  = BIT19;
		break;
	default:
		address  = BIT19 | BIT18;
		break;
	}

	// upload stimulus pattern to FPGA
	dword = nx*ny/2 + nx*3;
	reg1 = reg2 = reg3 = 0;
	dword = (dword>=16384) ? 16384 : dword;
	for (i = 0; i < dword; i ++) {
		for (j = 0; j < 2; j ++) {
			if (2*i+j < nx*ny)
				word[j] = buffer[2*i+j];
			else 
				word[j] = 0;
		}
		reg1 = word[0];
		reg2 = word[1] << 16;

		regData1 = reg1 + reg2;
		
		addr16 = address + i;
		// write address
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, addr16);
		// write 32-bit data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_DATA, regData1);
	}

	// set ram status to "read only"
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);
}



void CVirtex5BMD::AppWriteWarpLUT(WDC_DEVICE_HANDLE hDev, int nSizeX, int nStretchedX, unsigned short *lut_buf, int nChannelID)
{
	UINT32 regData1, reg1, reg2, reg3, address, addr16;
	UINT32 i, j, dword;
	short  word[2];

	// set ram status to "read/write"
	switch (nChannelID) {
	case 0:
		address  = BIT20 | BIT21 | BIT22 | BIT23;
		break;
	case 1:
		address  = BIT20 | BIT22;		// write IR/RED channels together, LUT1
		break;
	case 2:
		address  = BIT20;
		break;
	case 3:
		address  = BIT22;
		break;
	case 4:
		address  = BIT23 | BIT21;		// write IR/RED channels together, LUT2
		break;
	case 5:
		address  = BIT21;
		break;
	case 6:
		address  = BIT23;
		break;
	case 7:
		address  = BIT15;
		break;
	default:
		address  = BIT20 | BIT21 | BIT22 | BIT23;
		break;
	}

	// upload LUT to FPGA
	dword = nStretchedX/2 + 1;
	reg1 = reg2 = reg3 = 0;
	for (i = 0; i < dword; i ++) {
		for (j = 0; j < 2; j ++) {
			if (2*i+j < nStretchedX) {
				word[j] = lut_buf[2*i+j];
			} else {
				word[j] = 0;
			}
		}
		reg1 = word[0]; 
		reg2 = word[1] << 16;
		regData1 = reg1 + reg2;
		
		addr16 = address + i;
		// write address
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, addr16);
		// write 32-bit data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_DATA, regData1);
	}

	// set ram status to "read only"
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);
}
//

void CVirtex5BMD::AppWriteWarpWeights(WDC_DEVICE_HANDLE hDev, int nSizeX, UINT32 *stim_buf, int channelID)
{
	UINT32 i, address, addr16;

	if (channelID == 0)
		address  = BIT24;		// red channel
	else
		address  = BIT25;		// ir channel --> why ir and not green? in ICANDIDoc it is green! 

	// upload LUT to FPGA
	for (i = 0; i < nSizeX; i ++) {
		addr16  = address + i;
		// write address
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, addr16);
		// write 32-bit data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_DATA, stim_buf[i]);
	}

	// set ram status to "read only"
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);
}
//

// for GPU-based FFT
void CVirtex5BMD::AppWriteStimLUT(WDC_DEVICE_HANDLE hDev, bool latency, int x0, int x1, int xLp, int xRp, 
								  int y1, int y2, int y3, int sinusoid, int channelID)
{
	UINT32  regLocY, regLocX, ctrlBits, regData1, regData2, wBIT;
	float   deltax;
	int     i, latency_x=0;

	if (y2 <= y1) return;

	if (channelID == STIM_CHANNEL_IR) {
		//latency_x = latency?SYSTEM_LATENCY_DAC8+AOM_LATENCYX_IR:0;
		if (g_ICANDIParams.Imaging840)	// easy switch between imaging wavelengths
			latency_x = latency?SYSTEM_LATENCY_DAC8+AOM_LATENCYX_840:0;

		else
			latency_x = latency?SYSTEM_LATENCY_DAC8+AOM_LATENCYX_788:0;
		
		wBIT = BIT16;
	}
	else if (channelID == STIM_CHANNEL_GR) {
		wBIT = BIT27;
		latency_x = latency?SYSTEM_LATENCY_DAC14+AOM_LATENCYX_GR:0;
	}
	else if (channelID == STIM_CHANNEL_RD) {
		wBIT = BIT26;
		latency_x = latency?SYSTEM_LATENCY_DAC14+AOM_LATENCYX_RED:0;
	}
	else return;

	ctrlBits = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	ctrlBits = ctrlBits << 22;
	ctrlBits = ctrlBits >> 22;

	// set "we" bit for LUT RAM, so that it is in write status
	// CLEAN registers
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);	
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, ctrlBits);
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, wBIT);

	deltax = (float)(1.0*(x1-x0)/(y2-y1));

	for (i = y1; i < y2; i ++) {
		regLocY = (i << 21);							// address
		regLocX = (UINT32)(x0 + (i-y1)*deltax + 0.5);
		regLocX = (regLocX-latency_x) << 10;	// data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
		// ATLTRACE("VIRTEX5_WriteReg32: hDev:%d VIRTEX5_STIMULUS_LOC:%d regLocY:%d regLocX:%d ctrlBits:%d y3:%d\n", hDev, VIRTEX5_STIMULUS_LOC, regLocY, regLocX, ctrlBits, y3);
	}

	// with extended stimulus pattern, add additional four lines at the end
	// of each stripe to prevent accidental data loss.
	// these lines will be overwritten by the next stripe if there is no data loss
	// y3 = -1 for small stimuli
	if (y3 == 0) {
		for (i = y2; i < y2+4; i ++) {
			regLocY = (i << 21);							// address
			regLocX = (x1-latency_x) << 10;	// data
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
		}
	}

	for (i = y2; i < y3; i ++) {	
		regLocY = (i << 21);							// address
		regLocX = (x1-latency_x) << 10;	// data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	// clear "we" bit for LUT RAM so that it is in read status
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);

	if (sinusoid == 1) {
		// IR channel
		if (channelID == STIM_CHANNEL_IR) {
			regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2);
			regData1 = regData1 >> 16;
			regData1 = regData1 << 16;	// reserve high 16 bits for mirror motion
			regData2 = xLp + (xRp<<8);
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2, regData1+regData2);
			// ATLTRACE("IR boundaries: hDev:%d VIRTEX5_STIMULUS_X_BOUND2:%d regData1:%d regData2:%d\n", hDev, VIRTEX5_STIMULUS_X_BOUND2, regData1, regData2);

		// green channel
		} else if (channelID == STIM_CHANNEL_GR) {
			regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND);
			regData1 = regData1<<16;	// swapped order (compared to IR and RD)
			regData1 = regData1>>16;	// swapped order (compared to IRand RD)
			regData2 = (xLp<<16) + (xRp<<24);
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND, regData1+regData2);
			// ATLTRACE("GR boundaries: hDev:%d VIRTEX5_STIMULUS_X_BOUND:%d regData1:%d regData2:%d\n", hDev, VIRTEX5_STIMULUS_X_BOUND, regData1, regData2);

		// red channel
		} else if (channelID == STIM_CHANNEL_RD) {
			regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND);
			regData1 = regData1>>16;
			regData1 = regData1<<16;
			regData2 = xLp + (xRp<<8);	// why same coding like IR? should be equal to Green - the other digital channel!
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND, regData1+regData2);
			// ATLTRACE("RD boundaries: hDev:%d VIRTEX5_STIMULUS_X_BOUND:%d regData1:%d regData2:%d\n", hDev, VIRTEX5_STIMULUS_X_BOUND, regData1, regData2);

		}
	}
}

// write LUT for real-time stimulus locations
// two stimuli have the same size
void CVirtex5BMD::AppWriteStimLUT(WDC_DEVICE_HANDLE hDev, bool latency, int x1ir, int x2ir, int x1gr, int x2gr, int x1rd, int x2rd, 
								  int y1, int y2, int y3, int ysRD, int ysGR, int sinusoid, int xLir, int xRir, int xLgr, int xRgr, int xLrd, int xRrd)
{
	UINT32  regLocY, regLocX, ctrlBits, regData1, regData2;
	float   deltax;
	int     i, latency_x = 0;

	if (y2 <= y1) return;

	ctrlBits = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	ctrlBits = ctrlBits << 22;
	ctrlBits = ctrlBits >> 22;

	// set "we" bit for LUT RAM, so that it is in write status
	//weaBIT = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIM_ADDRESS);	
	//weaBIT = weaBIT | BIT27;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT27);//weaBIT);

	deltax = (float)(1.0*(x2gr-x1gr)/(y2-y1));
	latency_x = latency?SYSTEM_LATENCY_DAC14+AOM_LATENCYX_GR:0;
	for (i = y1+ysGR; i < y2+ysGR; i ++) {
		regLocY = (i << 21);							// address
		regLocX = (UINT32)(x1gr + (i-y1-ysGR)*deltax + 0.5);
		regLocX = (regLocX-latency_x) << 10;	// data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	if (y3 > y2) {
		for (i = y2+ysGR; i < y3+ysGR; i ++) {
			regLocY = (i << 21);							// address
			regLocX = (x2gr-latency_x) << 10;	// data
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
		}
	}

	// clear "we" bit for LUT RAM so that it is in read status
	//weaBIT = weaBIT - BIT27 + BIT26;
	// CLEAN registers and prepare Red writing
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);	
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, ctrlBits);

	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT26);//weaBIT);	

	deltax = (float)(1.0*(x2rd-x1rd)/(y2-y1));
	latency_x = latency?SYSTEM_LATENCY_DAC14+AOM_LATENCYX_RED:0;
	for (i = y1+ysRD; i < y2+ysRD; i ++) {
		regLocY = (i << 21);							// address
		regLocX = (UINT32)(x1rd + (i-y1-ysRD)*deltax + 0.5);
		regLocX = (regLocX-latency_x) << 10;	// data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	if (y3 > y2) {
		for (i = y2+ysRD; i < y3+ysRD; i ++) {
			regLocY = (i << 21);							// address
			regLocX = (x2rd-latency_x) << 10;	// data
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
		}
	}

	// ------------ add IR channel
	// write IR channel, locations of x along vertical direction
	// CLEAN registers and prepare IR writing
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);	
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, ctrlBits);

	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT16);//weaBIT);	

	deltax = (float)(1.0*(x2ir-x1ir)/(y2-y1));
	//latency_x = latency?SYSTEM_LATENCY_DAC8+AOM_LATENCYX_IR:0;
	if (g_ICANDIParams.Imaging840)	// easy switch between imaging wavelengths
		latency_x = latency?SYSTEM_LATENCY_DAC8+AOM_LATENCYX_840:0;

	else
		latency_x = latency?SYSTEM_LATENCY_DAC8+AOM_LATENCYX_788:0;
	
	for (i = y1; i < y2; i ++) {
		regLocY = (i << 21);							// address
		regLocX = (UINT32)(x1ir + (i-y1)*deltax + 0.5);
		regLocX = (regLocX-latency_x) << 10;	// data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	if (y3 > y2) {
		for (i = y2; i < y3; i ++) {
			regLocY = (i << 21);							// address
			regLocX = (x2ir-latency_x) << 10;	// data
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
		}
	}

	// clear "we" bit for LUT RAM so that it is in read status
	//weaBIT = weaBIT - BIT26;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);//weaBIT);	

	if (sinusoid == 1) {
		regData1 = xLrd + (xRrd<<8);
		regData2 = (xLgr<<16) + (xRgr<<24);
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND, regData1+regData2);

		regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2);
		regData1 = regData1 >> 16;
		regData1 = regData1 << 16;	// reserve high 16 bits for mirror motion
		regData2 = xLir + (xRir<<8);
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2, regData1+regData2);
	}
}

// two stimuli have different sizes
void CVirtex5BMD::AppWriteStimLUT(WDC_DEVICE_HANDLE hDev, bool latency, int xcIR, int ycIR, int xcGR, int ycGR, int xcRD, int ycRD, int iry, int rdy, int gry, int sinusoid, int xLir, int xRir, int xLrd, int xRrd, int xLgr, int xRgr)
{
	UINT32  regLocY, regLocX, ctrlBits, regData1, regData2;
	int     i, y1, y2, latency_x = 0;

	ctrlBits = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	ctrlBits = ctrlBits << 22;
	ctrlBits = ctrlBits >> 22;

	// write Red LUT
	// set "we" bit for LUT RAM, so that it is in write status
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT26);	

	latency_x = latency?SYSTEM_LATENCY_DAC14+AOM_LATENCYX_RED:0;
	y1 = ycRD - floor(rdy/2.f);					// conserve stimulus heigth (ND, 20191017)
	y2 = ycRD + ceil(rdy/2.f);					// conserve stimulus heigth (ND, 20191017)

	for (i = y1; i < y2; i ++) {
		regLocY = (i << 21);							// address
		regLocX = (UINT32)(xcRD-latency_x);
		regLocX = regLocX << 10;	// data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	// CLEAN registers and prepare IR writing
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);	
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, ctrlBits);

	// write IR LUT
	// clear "we" bit for LUT RAM so that it is in read status
	// set "we" bit for LUT RAM, so that it is in write status
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT27);	

	latency_x = latency?SYSTEM_LATENCY_DAC14+AOM_LATENCYX_GR:0;
	y1 = ycGR - ceil(gry/2.f);					// conserve stimulus heigth (ND, 20191017)
	y2 = ycGR + floor(gry/2.f);					// conserve stimulus heigth (ND, 20191017)

	for (i = y1; i < y2; i ++) {
		regLocY = (i << 21);							// address
		regLocX = (UINT32)(xcGR-latency_x);
		regLocX = regLocX << 10;	// data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);	
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, ctrlBits);

	// -------------- add IR channel
	// write IR channel
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT16);	
	//latency_x = latency?SYSTEM_LATENCY_DAC8+AOM_LATENCYX_IR:0;
	if (g_ICANDIParams.Imaging840)	// easy switch between imaging wavelengths
		latency_x = latency?SYSTEM_LATENCY_DAC8+AOM_LATENCYX_840:0;

	else
		latency_x = latency?SYSTEM_LATENCY_DAC8+AOM_LATENCYX_788:0;

	y1 = ycIR - ceil(iry/2.f);					// conserve stimulus heigth (ND, 20191017)
	y2 = ycIR + floor(iry/2.f);					// conserve stimulus heigth (ND, 20191017)

	for (i = y1; i < y2; i ++) {
		regLocY = (i << 21);							// address
		regLocX = (UINT32)(xcIR-latency_x);
		regLocX = regLocX << 10;	// data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	// clear "we" bit for LUT RAM so that it is in read status
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);	

	if (sinusoid == 1) {
		regData1 = xLrd + (xRrd<<8);
		regData2 = (xLgr<<16) + (xRgr<<24);
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND, regData1+regData2);

		regData1 = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2);
		regData1 = regData1 >> 16;
		regData1 = regData1 << 16;	// reserve high 16 bits for mirror motion
		regData2 = xLir + (xRir<<8);
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2,regData1+regData2);
	}
}

// for GPU-based FFT algorithm
void CVirtex5BMD::AppWriteStimAddrShift(WDC_DEVICE_HANDLE hDev, int y1, int y2, int y3, int stimWidth, int stripeH, int channelID)
{
	UINT32  regLocY, regLocX, ctrlBits, wBIT;
	int     i, delta;

	if (y2 <= y1) return;

	if (channelID == STIM_CHANNEL_IR)      wBIT = BIT17;		// IR data
	else if (channelID == STIM_CHANNEL_GR) wBIT = BIT29;		// green data
	else if (channelID == STIM_CHANNEL_RD) wBIT = BIT28;		// red data
	else if (channelID == STIM_CHANNEL_NU) wBIT = BIT31;		// null operation
	else return;

	ctrlBits = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	ctrlBits = ctrlBits << 22;
	ctrlBits = ctrlBits >> 22;

	// setup writing status
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, wBIT);	

	for (i = y1; i < y2; i ++) {
		regLocY = i << 21;							// address
		regLocX = stimWidth << 10;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	// image is compressed in y direction
	if (y2-y1<stripeH && y3==0) {
		delta = stripeH - (y2-y1) + 1;
		regLocY = y1 << 21;							// address
		regLocX = (delta*stimWidth) << 10;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	// image is stretched in y direction
	if (y2-y1>stripeH && y3==0) {
		for (i = y1+stripeH; i < y2; i ++) {
			regLocY = i << 21;							// address
			regLocX = 0;
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
		}
	}

	// process last stripe on the extended stimulus 
	for (i = y2; i < y3; i ++) {
		regLocY = i << 21;							// address
		regLocX = stimWidth << 10;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	// set control bit to read status
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);	
}


// two stimuli have the same size
void CVirtex5BMD::AppWriteStimAddrShift(WDC_DEVICE_HANDLE hDev, int y1, int y2, int y3, int y4, int dx_rd, int stripeH, int stripeID)
{
	UINT32  regLocY, regLocX, ctrlBits;
	int     i, delta;

	if (y2 <= y1) return;

	ctrlBits = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	ctrlBits = ctrlBits << 22;
	ctrlBits = ctrlBits >> 22;

	if (stripeID == -100) {		// simple stimulus pattern
		// write red channel
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT28);	
		for (i = y1+y3; i < y2+y3; i ++) {
			regLocY = i << 21;							// address
			regLocX = dx_rd << 10;
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
		}
		// write green channel
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT29);	
		for (i = y1+y4; i < y2+y4; i ++) {
			regLocY = i << 21;							// address
			regLocX = dx_rd << 10;
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
		}

		// write IR channel
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT17);	
		for (i = y1; i < y2; i ++) {
			regLocY = i << 21;							// address
			regLocX = dx_rd << 10;
			VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
		}

		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);

	} else {					// extended stimulus pattern

		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 31);

		if (stripeID == 1) {
			for (i = y1; i < y3; i ++) {
				regLocY = i << 21;							// address
				regLocX = dx_rd << 10;
				VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
			}
		} else {
			for (i = y1; i < y2; i ++) {
				regLocY = i << 21;							// address
				regLocX = dx_rd << 10;
				VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
			}
			// image is stretched in y direction
			for (i = y2; i < y3; i ++) {
				regLocY = i << 21;							// address
				regLocX = 0;
				VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
			}
			// image is compressed in y direction
			if (y2-y1 < stripeH) {
				delta = stripeH - (y2-y1) + 1;
				regLocY = y1 << 21;							// address
				regLocX = (delta*dx_rd) << 10;
				VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
			}
		}

		// setup writing status
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT29+BIT28+BIT17);	

		if (stripeID == 1) {
			for (i = y1; i < y3; i ++) {
				regLocY = i << 21;							// address
				regLocX = dx_rd << 10;
				VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
			}
		} else {
			for (i = y1; i < y2; i ++) {
				regLocY = i << 21;							// address
				regLocX = dx_rd << 10;
				VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
			}
			// image is stretched in y direction
			for (i = y2; i < y3; i ++) {
				regLocY = i << 21;							// address
				regLocX = 0;
				VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
			}
			// image is compressed in y direction
			if (y2-y1 < stripeH) {
				delta = stripeH - (y2-y1) + 1;
				regLocY = y1 << 21;							// address
				regLocX = (delta*dx_rd) << 10;
				VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
			}
		}

	}

	// set control bit to read status
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);	
}

// two stimuli have different sizes
void CVirtex5BMD::AppWriteStimAddrShift(WDC_DEVICE_HANDLE hDev, int yc, int ys_red, int ys_gr, int dx_ir, int dy_ir, int dx_gr, int dy_gr, int dx_rd, int dy_rd)
{
	UINT32  regLocY, regLocX, ctrlBits;
	int     i, y1, y2, HalfH1_rd, HalfH2_rd, HalfH1_gr, HalfH2_gr;

	ctrlBits = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	ctrlBits = ctrlBits << 22;
	ctrlBits = ctrlBits >> 22;

	// set "we" bit for LUT RAM, so that it is in write status
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT28);	
	
	HalfH1_rd = floor(dy_rd/2.f);					// conserve stimulus heigth (ND, 20191017)
	HalfH2_rd = ceil(dy_rd/2.f);					// conserve stimulus heigth (ND, 20191017)
	y1 = yc - HalfH1_rd + ys_red;
	y2 = yc + HalfH2_rd + ys_red;

	for (i = y1; i < y2; i ++) {
		regLocY = i << 21;							// address
		regLocX = dx_rd << 10;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	// CLEAN registers and prepare IR writing
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);	
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, ctrlBits);

	// clear "we" bit for LUT RAM so that it is in read status
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT29);	
	
	HalfH1_gr = ceil(dy_gr/2.f);					// conserve stimulus heigth (ND, 20191017)
	HalfH2_gr = floor(dy_gr/2.f);					// conserve stimulus heigth (ND, 20191017)
	y1 = yc - HalfH1_gr + ys_gr;
	y2 = yc + HalfH2_gr + ys_gr;

	for (i = y1; i < y2; i ++) {
		regLocY = i << 21;							// address
		regLocX = dx_gr << 10;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	// write IR channel
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, BIT17);	
	y1 = yc - ceil(dy_ir/2.f);						// conserve stimulus heigth (ND, 20191017)
	y2 = yc + floor(dy_ir/2.f);						// conserve stimulus heigth (ND, 20191017)

	for (i = y1; i < y2; i ++) {
		regLocY = i << 21;							// address
		regLocX = dx_ir << 10;
		VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regLocY+regLocX+ctrlBits);
	}

	// clear "we" bit for LUT RAM so that it is in read status
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIM_ADDRESS, 0);	
}

/*
void CVirtex5BMD::AppWriteCriticalInfo(WDC_DEVICE_HANDLE hDev, int targetY, float predictionTime)
{
	int     lineID;
	UINT32  regHi, regLo, regI, regO;

	lineID = (int)(targetY-predictionTime*16);
	if (lineID <= 0) return;
	regI   = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINECTRL_OFFSET);
	regHi  = regI  >> 16;
	regHi  = regHi << 16;
	regLo  = regI  << 27;
	regLo  = regLo >> 27;
	regO   = lineID << 6;
	regO   = regO + regHi + regLo + BIT5;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINECTRL_OFFSET, regO);
}

void CVirtex5BMD::AppClearCriticalInfo(WDC_DEVICE_HANDLE hDev)
{
	UINT32  regI, regO;

	regI   = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINECTRL_OFFSET);
	regO   = (regI|BIT5) - BIT5;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINECTRL_OFFSET, regO);
}

BOOL CVirtex5BMD::AppReadCriticalInfo(WDC_DEVICE_HANDLE hDev)
{
	UINT32  regI;

	regI = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINECTRL_OFFSET);
	if (regI&BIT5 > 0) return TRUE;
	else return FALSE;
}
*/

void CVirtex5BMD::PupilMaskOn(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
	reg = reg | BIT28; 
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, reg);		// bit 15 controls stimulus pattern write/read
}

void CVirtex5BMD::PupilMaskOff(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
	reg = reg | BIT28;
	reg = reg - BIT28;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, reg);		// bit 15 controls IR stimulus, bit 14 controls stimulus pattern write/read
}

void CVirtex5BMD::RedCalibrationOn(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
	reg = reg | BIT29; 
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, reg);		// bit 15 controls stimulus pattern write/read
}

void CVirtex5BMD::RedCalibrationOff(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
	reg = reg | BIT29;
	reg = reg - BIT29;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, reg);		// bit 15 controls IR stimulus, bit 14 controls stimulus pattern write/read
}

void CVirtex5BMD::StimLinesOn(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
	reg = reg | BIT30; 
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, reg);		// bit 15 controls stimulus pattern write/read
}

void CVirtex5BMD::StimLinesOff(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
	reg = reg | BIT30;
	reg = reg - BIT30;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, reg);		// bit 15 controls IR stimulus, bit 14 controls stimulus pattern write/read
}

void CVirtex5BMD::WritePupilMask(WDC_DEVICE_HANDLE hDev,UINT32 sx, UINT32 sy, UINT32 nx, UINT32 ny)
{
	UINT32 regW, regR, regH, regL;

//	regW = (sx << 25) + (sy << 18) + (nx << 9) + ny;
//	VIRTEX5_WriteReg32(hDev, VIRTEX5_PUPILMASK_OFFSET, regW);


	regW = (sx << 20) + (nx << 10) + ny;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_PUPILMASK_OFFSET, regW);


	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	// reserve high 22 bits
	regH = regR >> 10;
	regH = regH << 10;
	// reserve low 2 bits
	regL = regR << 30;
	regL = regL >> 30;

	UINT32 nTemp = sy;
	nTemp = (nTemp << 2);
	regW = regH + regL + nTemp;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, regW);
	
}

void CVirtex5BMD::ReadPupilMask(WDC_DEVICE_HANDLE hDev, UINT32 *sx, UINT32 *sy, UINT32 *nx, UINT32 *ny)
{
	UINT32 regR, reg;

	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_PUPILMASK_OFFSET);
	reg = regR >> 25;
	*sx = reg  << 25;
	reg = regR >> 18;
	reg = reg  << 25;
	*sy = reg  >> 25;
	reg = regR >> 9;
	reg = reg  << 23;
	*nx = reg  >> 23;
	reg = regR << 23;
	*ny = reg  >> 23;
}




void CVirtex5BMD::AppStartADC(WDC_DEVICE_HANDLE hDev)
{
	UINT32 ddmacr;

	ddmacr = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINECTRL_OFFSET);
	ddmacr |= BIT0;
    VIRTEX5_WriteReg32(hDev, VIRTEX5_LINECTRL_OFFSET, ddmacr);
}

void CVirtex5BMD::AppStopADC(WDC_DEVICE_HANDLE hDev)
{
	UINT32 regR;

	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINECTRL_OFFSET);
	regR |= BIT0;
	regR -= BIT0;
    VIRTEX5_WriteReg32(hDev, VIRTEX5_LINECTRL_OFFSET, regR);	
}


void CVirtex5BMD::AppUpdate14BITsLaserRed(WDC_DEVICE_HANDLE hDev, int power)
{
	UINT32  regR, regW, regH;

	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER);
	regW = regR << 16;
	regW = regW >> 16;  // reserve bits 0-15
	regH = regR >> 30;
	regH = regH << 30;	// reserve bits 31-30
	regR = power << 16;
	regW = regW + regR + regH;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER, regW);
}

void CVirtex5BMD::AppUpdate14BITsLaserGR(WDC_DEVICE_HANDLE hDev, int power)
{
	UINT32  regR, regW;

	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER);
	regW = regR >> 14;
	regW = regW << 14;  // reserve bits 16-31
	//regR = power;
	regR = power>=BIT14 ? BIT13-1 : power;
	regW = regW + regR;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER, regW);
}

BOOL CVirtex5BMD::DetectSyncSignals(WDC_DEVICE_HANDLE hDev)
{
	BYTE    regVal, hsync, vsync;

	if (ReadWriteI2CRegister(hDev, TRUE, 0x4c000000, 0x24, 0, &regVal) == 0) {
		hsync = regVal & BIT7;
		vsync = regVal & BIT5;
		if (hsync == 0 || vsync == 0) 
			return FALSE;
		else 
			return TRUE;
	} else {
		return TRUE;
	}
}

void CVirtex5BMD::AppWriteStimLoc(WDC_DEVICE_HANDLE hDev, int x, int y, int latency)
{
	UINT32 ctrlBits, reg;

	if (x < 0 || y < 0) return;

	ctrlBits = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_LOC);
	ctrlBits = ctrlBits << 22;
	ctrlBits = ctrlBits >> 22;
	
	reg = (y << 21) + ((x-latency) << 10) + ctrlBits;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, reg);
}

/*
void CVirtex5BMD::AppAddTCAbox(WDC_DEVICE_HANDLE hDev,  int sxIR, int syIR, int dxIR, int dyIR,
														int sxRd, int syRd, int dxRd, int dyRd,
														int sxGr, int syGr, int dxGr, int dyGr) {
	UINT32  regData, regCtrl;
	int     i, n;

	n = 32;

	// write IR tca information
	regCtrl = BIT27;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, regCtrl);

	for (i = 0; i < n; i ++) {
		regData = (sxIR<<23) + (syIR<<14) + (dxIR<<5) + i;
		// write 32-bit data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_TCABOX_OFFSET, regData);
	}

	// write Red tca information
	regCtrl = BIT26;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, regCtrl);

	for (i = 0; i < n; i ++) {
		regData = (sxRd<<23) + (syRd<<14) + (dxRd<<5) + i;
		// write 32-bit data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_TCABOX_OFFSET, regData);
	}

	// write Green tca information
	regCtrl = BIT25;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, regCtrl);

	for (i = 0; i < n; i ++) {
		regData = (sxGr<<23) + (syGr<<14) + (dxGr<<5) + i;
		// write 32-bit data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_TCABOX_OFFSET, regData);
	}

	// write tca height information
	regCtrl = BIT24;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, regCtrl);

	for (i = 0; i < n; i ++) {
		regData = (dyIR<<23) + (dyRd<<14) + (dyGr<<5) + i;
		// write 32-bit data
		VIRTEX5_WriteReg32(hDev, VIRTEX5_TCABOX_OFFSET, regData);
	}

	// clear control bits
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, 0);
}

void CVirtex5BMD::AppRemoveTCAbox(WDC_DEVICE_HANDLE hDev)
{
	VIRTEX5_WriteReg32(hDev, VIRTEX5_TCABOX_OFFSET, 0);
}

void CVirtex5BMD::AppShiftRedY(WDC_DEVICE_HANDLE hDev, int lines)
{
	UINT32  regR, regW, high, low;

	if (abs(lines) > 127) return;

	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
	high = regR;
	high = high >> 16;	// reserve high 16 bits
	high = high << 16;
	low  = regR << 24;
	low  = low  >> 24;

	if (lines>=0) {
		regW = lines;
	} else {
		regW = BIT7 - lines;
	}

	regW = regW << 8; // move left 8 bits
	regW = high + regW + low;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, regW);
}

void CVirtex5BMD::AppShiftGreenY(WDC_DEVICE_HANDLE hDev, int lines)
{
	UINT32  regR, regW, high;

	if (abs(lines) > 127) return;

	regR = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINEINFO_OFFSET);
	high = regR;
	high = high >> 8;	// reserve high 16 bits
	high = high << 8;

	if (lines>=0) {
		regW = lines;
	} else {
		regW = BIT7 - lines;
	}

	regW = high + regW;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINEINFO_OFFSET, regW);
}
*/
void CVirtex5BMD::AppRedAOM_On(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER);
	reg = reg | BIT15; 
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER, reg);
}


void CVirtex5BMD::AppRedAOM_Off(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER);
	reg = reg | BIT15;
	reg = reg - BIT15;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, reg);
}

void CVirtex5BMD::AppGreenAOM_On(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER);
	reg = reg | BIT14; 
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER, reg);
}

void CVirtex5BMD::AppGreenAOM_Off(WDC_DEVICE_HANDLE hDev)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER);
	reg = reg | BIT14;
	reg = reg - BIT14;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_LOC, reg);
}


void CVirtex5BMD::AppMotionTraces(WDC_DEVICE_HANDLE hDev, float deltaX, float deltaY, float scalerX, float scalerY, FILE* fp)
{
	UINT32  regA, regB;
	int     x, y, xt, yt;

/*	regA = VIRTEX5_ReadReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2);
	regA = regA << 16;
	regA = regA >> 16; // reserve low 16 bits for IR stimulus boundary

	x = (roundf)(scalerX*deltaX) + 128;  // add offset 128 to motion trace
	y = (roundf)(scalerY*deltaY) + 128;  // add offset 128 to motion trace
	x = (x < 0) ?   0   : x;
	x = (x > 255) ? 255 : x;
	y = (y < 0) ?   0   : y;
	y = (y > 255) ? 255 : y;

	regA = (x << 24) + (y << 16) + regA;
	// write 32-bit data
	VIRTEX5_WriteReg32(hDev, VIRTEX5_STIMULUS_X_BOUND2, regA);
*/
	// scale dac output, from 8 bits to 14 bits
	//	dac_scaler = 64;
	// write motion trace to the 14-bit dac
	x = (roundf)(64.*scalerX*deltaX) + 8192;  // add offset 8192 to motion trace
	y = (roundf)(64.*scalerY*deltaY) + 8192;  // add offset 8192 to motion trace
	x = (x < 0) ?       0   : x;
	x = (x > 16383) ? 16383 : x;
	y = (y < 0) ?   0       : y;
	y = (y > 16383) ? 16383 : y; 
	regA = (x << 16) + y;

	VIRTEX5_WriteReg32(hDev, VIRTEX5_PUPILMASK_OFFSET, regA);

/*	if (fp!= NULL)
		fprintf(fp, "%2.4f\t%d\t%2.4f\t%d\n",deltaX, x, deltaY, y);

	regB = VIRTEX5_ReadReg32(hDev, VIRTEX5_PUPILMASK_OFFSET);
	xt = (regB>>16);
	yt= (regB << 16);
	yt = (yt >> 16);
	// now write x, y to the log file
	if (fp!= NULL)
		fprintf(fp, "%2.4f\t%d\t%d\t%d\t%d\n",deltaY, x, xt, y, yt);*/
}


void CVirtex5BMD::AppSetFrameCounter(WDC_DEVICE_HANDLE hDev, BOOL bByLines)
{
	UINT32 reg, high, low;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LINECTRL_OFFSET);
	high = reg  >> 6;
	high = high << 6;
	low  = reg  << 27;
	low  = low  >> 27;
	
	if (bByLines) 
		reg = high + BIT5 + low;
	else
		reg = high + low;

	VIRTEX5_WriteReg32(hDev, VIRTEX5_LINECTRL_OFFSET, reg);
}


void CVirtex5BMD::AppVsyncTrigEdge(WDC_DEVICE_HANDLE hDev, BOOL bStatus)
{
	UINT32  reg;

	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_LASER_POWER);
	reg = reg | BIT30;
	if (!bStatus) 
		reg = reg - BIT30;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_LASER_POWER, reg);
}

void CVirtex5BMD::AppSetADCchannel(WDC_DEVICE_HANDLE hDev, BYTE chID)
{
	UINT32  reg, regCh;

	if (chID > 3) return;

	regCh = (chID << 12);
	reg = VIRTEX5_ReadReg32(hDev, VIRTEX5_IMAGEOS_OFFSET);
	reg = reg | BIT12 | BIT13;
	reg = reg - BIT12 - BIT13;
	reg = reg + regCh;
	VIRTEX5_WriteReg32(hDev, VIRTEX5_IMAGEOS_OFFSET, reg);
}
