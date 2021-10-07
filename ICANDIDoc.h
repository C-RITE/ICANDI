// ICANDIDoc.h : interface of the CICANDIDoc class
//
/////////////////////////////////////////////////////////////////////////////
#include "virtex5bmd.h"
#include "utils/netcomm/SockListener.h"
#include <fstream> 

#include <algorithm>
#include "vfw.h"
#include "MatALL.h"
#include "opencv2/videoio/videoio_c.h"
#include "opencv2/core.hpp"
#include "opencv2/highgui/highgui.hpp"
using namespace cv;

#if !defined(AFX_ICANDIDOC_H__6ABEA42C_1404_481D_888B_C3BE7E912E99__INCLUDED_)
#define AFX_ICANDIDOC_H__6ABEA42C_1404_481D_888B_C3BE7E912E99__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

#define  RET_CODE_SUCCESS  0
#define  RET_CODE_SACCADE  1
#define  RET_CODE_EST_ERR  2
#define  RET_MSC_INIT_ERR  3
#define  TOTAL_SEQUENCE	   1000
#define  STIMOFFSETLIMITX 32
#define  STIMOFFSETLIMITY 16

#define  BLOCK_COUNT   32

const int  FFT_WIDTH      = 256;
const int  FFT_HEIGHT256  = 256;
const int  FFT_HEIGHT128  = 128;
const int  FFT_HEIGHT064  = 32;

void _stdcall Out32(short PortAddress, short data); //12/15/2011
short _stdcall Inp32(short PortAddress);

struct stimulus_data
{
	int F_XDIM;
	int F_YDIM;
	int BlockID;
	int slice_height; 
	int StimulusX;
	int StimulusY;
	int iternum;
	int k1;
	int thre_g;
	int thre_s;
	int qxCenOld;
	int qyCenOld;
	int qyCenOldR;
	int qyCenOldT;
	float prediction_time;
	BOOL bCritical;
	int X_OFFSET_fine;
	int Y_OFFSET_fine;
	int L_YDIM_fine;
	int fraID;
	BOOL projection;
};

struct stabilization_data
{
	int    PATCH_CNT;	// patch counts
	int    F_XDIM;		// image width
	int    F_YDIM;		// image height
	int   *L_XDIM;
	int   *L_YDIM;
	int   *L_YDIM0;		// start position of L_YDIM
	int   *X_OFFSET;
	int   *Y_OFFSET;

	int    TOT_L_CT;
	int    L_CT;
	int    ITERFINAL;
	int    K1;
	int    GTHRESH;
	int    SRCTHRESH;

	int    LYDIMCen;
	int    XoffsetCen;
	int    PatchHeightCen;
};

class CICANDIDoc : public CDocument
{
protected: // create from serialization only
	CICANDIDoc();
	DECLARE_DYNCREATE(CICANDIDoc)

// Attributes
public:	
	BOOL    m_bDumpingVideo;
	BOOL	m_bUpdateDutyCycle;

	//MIL_ID   m_MilImage;		// Image buffer identifier.
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CICANDIDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	CString *m_strStimVideoName;
	BOOL  m_bSymbol;
	void  LoadSymbol(CString filename="", unsigned short* stim_data=NULL, int width=0, int height=0);
	BOOL  LoadStimVideo(CString *filename, int i);
	bool  SendNetMessage(CString message);

	virtual ~CICANDIDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    BOOL m_bServerCreated;

// Generated message map functions
protected:
	//{{AFX_MSG(CICANDIDoc)
	afx_msg void OnEditParams();
	afx_msg void OnUpdateEditParams(CCmdUI* pCmdUI);
	afx_msg void OnSetupDesinusoid();
	afx_msg void OnUpdateSetupDesinusoid(CCmdUI* pCmdUI);
	afx_msg void OnCameraConnect();
	afx_msg void OnCameraDisconnect();
	afx_msg void OnUpdateCameraConnect(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCameraDisconnect(CCmdUI* pCmdUI);
	afx_msg void OnStablizeGo();
	afx_msg void OnUpdateStablizeGo(CCmdUI* pCmdUI);
	afx_msg void OnStablizeSuspend();
	afx_msg void OnUpdateStablizeSuspend(CCmdUI* pCmdUI);
	afx_msg void OnSaveReference();
	afx_msg void OnUpdateSaveReference(CCmdUI* pCmdUI);
	afx_msg void OnMovieNormalize();
	afx_msg void OnUpdateMovieNormalize(CCmdUI* pCmdUI);
	afx_msg void OnLoadExtRef();
	afx_msg void OnUpdateLoadExtRef(CCmdUI* pCmdUI);
	
	DECLARE_MESSAGE_MAP()


private:
	MMRESULT m_idTimerEvent;
	UINT    m_nTimerRes;
	ULONG   m_uResolution;

	int     m_nStimSizeX;
	int     m_nStimSizeY;
	long    m_bufferAttributes;
	BOOL    m_bMilImageValid;
	CSockListener	*m_ncListener_AO;
	CSockListener	*m_ncListener_Matlab;
	CSockListener	*m_ncListener_IGUIDE;

	DWORD   thdid_handle[10];
	HANDLE  thd_handle[10];
	static  DWORD WINAPI ThreadStablizationFFT(LPVOID pParam);
	static  DWORD WINAPI ThreadSaveVideoHDD(LPVOID pParam);
	static  DWORD WINAPI ThreadVideoSampling(LPVOID pParam);
	static  DWORD WINAPI ThreadLoadData2FPGA(LPVOID pParam);
	static  DWORD WINAPI ThreadReadStimVideo(LPVOID pParam);
	static	DWORD WINAPI ThreadNetMsgProcess(LPVOID pParam);
	static	DWORD WINAPI ThreadSLRProcess(LPVOID pParam);
	static	DWORD WINAPI ThreadSyncOCTVideo(LPVOID pParam);
	static	DWORD WINAPI ThreadSendViewMsg(LPVOID pParam);
	static  DWORD WINAPI ThreadPlayStimPresentFlag(LPVOID pParam);
	void	Initialize_LUT(); //lasers power calibration values

public:
	BOOL    m_bCameraConnected;

	void DrawStimulus();
	BOOL LoadMaskTraces(CString filename);
	BOOL Load14BITbuffer(CString filename, unsigned short **stim_buffer, int *width, int *height);
	BOOL Load8BITbmp(CString filename, unsigned short **stim_buffer, int *width, int *height, BOOL checksize);
	void LoadMultiStimuli();	
	BOOL LoadMultiStimuli_Matlab(int clear, CString folder, CString prefix, short startind, short endind, CString ext);
	void PlaybackMovie(LPCTSTR filename);
	void StopPlayback();
	int	 saveBitmap();
	void Load_Default_Stimulus(bool inv);
	void GetSysTime(CString &buf);
//	int UpdateRuntimeRegisters(unsigned char *);
//	int I2CController(UINT32 regDataW, BYTE *reg_val, BOOL bI2Cread);
//	int ReadWriteI2CRegister(BOOL bRead, UINT32 slave_addr, BYTE regAddr, BYTE regValI, BYTE *regValO);
	void UpdateImageGrabber();
	CString  m_aviFileNameA;			// file name for raw video
	CString  m_aviFileNameB;			// file name for stabilized video
	CString  m_txtFileName;				// file name for stimulus position
	CString  m_videoFileName;
	UINT     m_iSavedFramesA;			// frame number for raw video
	UINT     m_iSavedFramesB;			// frame number for stabilized video
	VideoWriter m_aviFileA;				// AVI hander for raw video
	VideoWriter m_aviFileB;				// AVI hander for stabilized video
	cv::Size	 m_frameSizeA;
	cv::Size	 m_frameSizeB;
	FILE    *m_fpStimPos;				// txt file pointer for stimulus positions
	BOOL     m_bValidAviHandleA;		// flag for raw video
	BOOL     m_bValidAviHandleB;		// flag for stabilized video
	HANDLE	*m_eNetMsg;
	CString *m_strNetRecBuff;
	BOOL	 m_bPlayback;
	BOOL	 m_bExtCtrl;
	short	m_nVideoNum;
	CString m_VideoTimeStamp;
	CString m_VideoFolder;
	CString m_VideoFolderSuffix;
	CString m_waveLength;

	//Stimulus location recovery 
	CLSID	pBmpEncoder;	
	BOOL	m_bMatlab;
	CPoint	m_cpOldLoc;
	CPoint  m_cpOldLoc_bk;
	unsigned char* m_pOldRef;

	UINT     m_nMovieLength;			// define length of movie that will be saved to HDD
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICANDIDOC_H__6ABEA42C_1404_481D_888B_C3BE7E912E99__INCLUDED_)
