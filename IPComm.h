#ifndef INTERPROCESSCOMMUNICATION_H__
#define INTERPROCESSCOMMUNICATION_H__

#define COMM_START             0
#define COMM_END              -1
#define REFERENCE_FRAME      101
#define TARGET_FRAME         102
#define INVALID_HEADER       103
#define HEADER_RECEIVED      104

//#define SLICE_HEIGHT          16	// define slice height of an image patch which will 
									// be sent to MSC server in each communication cycle.
//#define SLICE_COUNTS          32	// total number of slices an image frame which will 
									// be sent to MSC server.
									/* these two parameters will be built into GUI in a later version */
#define WITHOUT_DELTA         -1
#define WITH_DELTA             0
#define FLAG_SACCADE          -1
#define FLAG_DRIFT             0

#define DELTAS_STIMULUS        100
#define DELTAS_PATCH           101

#define FILTER_NONE            0
#define FILTER_GAUSSIAN_3BY3   1
#define FILTER_GAUSSIAN_5BY5   2
#define FILTER_MEDIAN_3BY3     4
#define FILTER_MEDIAN_5BY5     5

#define ERROR_PATCH_CNT       -1
#define ERROR_patchIDX        -2
#define ERROR_patchWidth      -3
#define ERROR_patchHeight     -4
#define ERROR_fraID           -5
#define ERROR_F_XDIM          -6
#define ERROR_F_YDIM          -7
#define ERROR_L_XDIM_fra      -8
#define ERROR_L_YDIM_fra      -9
#define ERROR_X_OFFSET_fra   -10
#define ERROR_Y_OFFSET_fra   -11
#define ERROR_TOT_L_CT       -12
#define ERROR_L_CT           -13
#define ERROR_X_SHIFT_fra    -14
#define ERROR_Y_SHIFT_fra    -15
#define ERROR_ITERFINAL      -16
#define ERROR_K1             -17
#define ERROR_GTHRESH        -18
#define ERROR_SRCTHRESH      -19

#define ERROR_patchSize      -20

#include <windows.h>
#include <tchar.h>

//---------------------------------------------------------------------
// Structure used by the class to manage its internal variables
// This struct is mapped into the common memory area between the two
// communicating processes
//
typedef struct
{
	DWORD ServerPID; // the PID of the process that last wrote
	DWORD ClientPID; // the first process
} tInterProcessCommunicationData;

//----------------------------------------------------------------------
// Class interface
//
class CIPComm
{
public:
	DWORD  GetLastError() const;
	DWORD  GetOtherProcessId() const;
	tInterProcessCommunicationData * GetIpcInternalData() const;
	bool   InterConnect();
	CIPComm(LPCTSTR SharedName, DWORD Size, BOOL bHandlesInheritable = FALSE);
   ~CIPComm();
	void   SendBuffer(LPVOID Buffer, DWORD Size);
	void   ReceiveBuffer(LPVOID Buffer, DWORD Size);
private:
	void   EndRaceProtection();
	bool   BeginRaceProtection();

	DWORD  LastError;
	HANDLE FetchEvent(LPCTSTR szName);
	bool   m_bIsServer;
	tInterProcessCommunicationData *m_ipcd;
	void   MakeObjectsNames(LPCTSTR szSharedName);
	DWORD  m_nSize;
	DWORD  m_PID;
	HANDLE m_hFileMap;
	HANDLE m_hDataMutex; // memory protection mutex
	HANDLE m_hServerEvent; 
	HANDLE m_hClientEvent;
	HANDLE m_hRaceMutex;
	LPVOID m_lpMem;
	LPVOID m_lpMappedViewOfFile;
	TCHAR  m_szMutexName[80];
	TCHAR  m_szFileMapName[80];
	TCHAR  m_szSharedName[80];
	TCHAR  m_szRaceMutex[80];
	BOOL   m_bHandlesInheritable;
};

#endif