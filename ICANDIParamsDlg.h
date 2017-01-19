#include "afxwin.h"
#if !defined(AFX_ICANDIPARAMSDLG_H__F44E0ED7_DFB4_4194_BF45_4DDE31BF90DC__INCLUDED_)
#define AFX_ICANDIPARAMSDLG_H__F44E0ED7_DFB4_4194_BF45_4DDE31BF90DC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ICANDIParamsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CICANDIParamsDlg dialog

class CICANDIParamsDlg : public CDialog
{
// Construction
public:
	CICANDIParamsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CICANDIParamsDlg)
	enum { IDD = IDD_DLG_PARAMS };
	CListCtrl	m_lstPatchInfo;
	UINT	m_F_XDIM;
	UINT	m_F_YDIM;
	UINT	m_GTHRESH;
	UINT	m_ITERNUM;
	UINT	m_K1;
	UINT	m_L_XDIM;
	UINT	m_L_YDIM;
	UINT	m_LAYER_CNT;
	UINT	m_PATCH_WIDTH;
	UINT	m_PATCH_HEIGHT;
	UINT	m_PATCH_CNT;
	UINT	m_SRCTHRESH;
	UINT	m_X_OFFSET;
	UINT	m_X_SHIFT;
	UINT	m_Y_OFFSET;
	UINT	m_Y_SHIFT;
	UINT	m_L_YDIM0;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CICANDIParamsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CICANDIParamsDlg)
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	afx_msg void OnCheckDesinusoid();
	afx_msg void OnInvertMotionTraces();
	afx_msg void OnWriteDigitalMarks();
	afx_msg void OnWriteMarkIRG1();
	afx_msg void OnWriteMarkREDG1();
	afx_msg void OnWriteMarkGRG1();
	afx_msg void OnWriteMarkBLUEG1();
	afx_msg void OnWriteMarkIRG2();
	afx_msg void OnWriteMarkREDG2();
	afx_msg void OnWriteMarkGRG2();
	afx_msg void OnWriteMarkBLUEG2();
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkListParams(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK();
	afx_msg void OnButtonUpdate();
	afx_msg void OnButtonLut();
	afx_msg void OnCbnSelchangeComboFilter();
	afx_msg void OnCbnSelchangeGpuDevList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	BOOL ValidateLists();
	BOOL ValidateAddLine(int *vals);
	BOOL ValidateInputs();
	int  m_intDesinusoid;
	CString m_LUTname;

	int  m_intGPUIDold;
	int  m_intGPUID;
	int  m_intFilterID;
	int  m_gpu_counts;
	CString *m_gpu_dev_list;
	int  m_intInvertTraces;
	BOOL m_boolWriteMarkFlags[9];

private:
	int   m_nColumns;	
	CButton m_chkDesinusoid;
	CButton m_chkInvertMotionTraces;
	CButton m_chkDigitalMarks;
	CButton m_chkWriteMarkIRG1;
	CButton m_chkWriteMarkREDG1;
	CButton m_chkWriteMarkGRG1;
	CButton m_chkWriteMarkBLUEG1;
	CButton m_chkWriteMarkIRG2;
	CButton m_chkWriteMarkREDG2;
	CButton m_chkWriteMarkGRG2;
	CButton m_chkWriteMarkBLUEG2;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICANDIPARAMSDLG_H__F44E0ED7_DFB4_4194_BF45_4DDE31BF90DC__INCLUDED_)
