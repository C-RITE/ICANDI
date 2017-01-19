// ICANDIParamsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ICANDI.h"
#include "ICANDIParamsDlg.h"

#include "ICANDIParams.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CICANDIParamsDlg dialog

extern ICANDIParams    g_ICANDIParams;
extern BOOL         g_bNoGPUdevice;

CICANDIParamsDlg::CICANDIParamsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CICANDIParamsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CICANDIParamsDlg)
	m_F_XDIM = 0;
	m_F_YDIM = 0;
	m_GTHRESH = 0;
	m_ITERNUM = 0;
	m_K1 = 0;
	m_L_XDIM = 0;
	m_L_YDIM = 0;
	m_LAYER_CNT = 0;
//	m_PATCH_WIDTH = 0;
//	m_PATCH_HEIGHT = 0;
	m_PATCH_CNT = 0;
	m_SRCTHRESH = 0;
	m_X_OFFSET = 0;
	m_X_SHIFT = 0;
	m_Y_OFFSET = 0;
	m_Y_SHIFT = 0;
	m_L_YDIM0 = 0;
	//}}AFX_DATA_INIT

	m_nColumns = 8;
	m_LUTname =  "";

	m_intGPUID = 0;
	m_intGPUIDold = 0;
	m_gpu_dev_list = NULL;
	m_gpu_counts   = 0;
}


void CICANDIParamsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CICANDIParamsDlg)
	DDX_Control(pDX, IDC_LIST_PARAMS, m_lstPatchInfo);
	DDX_Text(pDX, IDC_EDIT_F_XDIM, m_F_XDIM);
	DDV_MinMaxUInt(pDX, m_F_XDIM, 1, 2048);
	DDX_Text(pDX, IDC_EDIT_F_YDIM, m_F_YDIM);
	DDV_MinMaxUInt(pDX, m_F_YDIM, 1, 2048);
	/*	DDX_Text(pDX, IDC_EDIT_GTHRESH, m_GTHRESH);
	DDV_MinMaxUInt(pDX, m_GTHRESH, 0, 255);
	DDX_Text(pDX, IDC_EDIT_ITER_NUM, m_ITERNUM);
	DDV_MinMaxUInt(pDX, m_ITERNUM, 2, 15);
	DDX_Text(pDX, IDC_EDIT_K1, m_K1);
	DDV_MinMaxUInt(pDX, m_K1, 0, 65536);
	DDX_Text(pDX, IDC_EDIT_L_XDIM, m_L_XDIM);
	DDX_Text(pDX, IDC_EDIT_L_YDIM, m_L_YDIM);
	DDX_Text(pDX, IDC_EDIT_LAYER_CNT, m_LAYER_CNT);
	DDV_MinMaxUInt(pDX, m_LAYER_CNT, 2, 10);
	DDX_Text(pDX, IDC_EDIT_PATCH_CNT, m_PATCH_CNT);
	DDV_MinMaxUInt(pDX, m_PATCH_CNT, 1, 128);
	DDX_Text(pDX, IDC_EDIT_SRCTHRESH, m_SRCTHRESH);
	DDV_MinMaxUInt(pDX, m_SRCTHRESH, 0, 255);
	DDX_Text(pDX, IDC_EDIT_X_OFFSET, m_X_OFFSET);
	DDX_Text(pDX, IDC_EDIT_X_SHIFT, m_X_SHIFT);
	DDX_Text(pDX, IDC_EDIT_Y_OFFSET, m_Y_OFFSET);
	DDX_Text(pDX, IDC_EDIT_Y_SHIFT, m_Y_SHIFT);
	DDX_Text(pDX, IDC_EDIT_L_YDIM_0, m_L_YDIM0);
	DDV_MinMaxUInt(pDX, m_L_YDIM0, 0, 2047);*/	
	DDX_Control(pDX, IDC_CHECK_DESINUSOID, m_chkDesinusoid);
	DDX_Control(pDX, IDC_CHECK_INVERTTRACES, m_chkInvertMotionTraces);
	DDX_Control(pDX, IDC_TURN_ON_OFF, m_chkDigitalMarks);
	DDX_Control(pDX, IDC_IR_GAIN1, m_chkWriteMarkIRG1);
	DDX_Control(pDX, IDC_RED_GAIN1, m_chkWriteMarkREDG1);
	DDX_Control(pDX, IDC_GR_GAIN1, m_chkWriteMarkGRG1);
	DDX_Control(pDX, IDC_BLUE_GAIN1, m_chkWriteMarkBLUEG1);
	DDX_Control(pDX, IDC_IR_GAIN2, m_chkWriteMarkIRG2);
	DDX_Control(pDX, IDC_RED_GAIN2, m_chkWriteMarkREDG2);
	DDX_Control(pDX, IDC_GR_GAIN2, m_chkWriteMarkGRG2);
	DDX_Control(pDX, IDC_BLUE_GAIN2, m_chkWriteMarkBLUEG2);

	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CICANDIParamsDlg, CDialog)
	//{{AFX_MSG_MAP(CICANDIParamsDlg)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)
	ON_BN_CLICKED(IDC_CHECK_DESINUSOID, OnCheckDesinusoid)	
	ON_BN_CLICKED(IDC_CHECK_INVERTTRACES, OnInvertMotionTraces)
	ON_BN_CLICKED(IDC_TURN_ON_OFF, OnWriteDigitalMarks)
	ON_BN_CLICKED(IDC_IR_GAIN1, OnWriteMarkIRG1)
	ON_BN_CLICKED(IDC_RED_GAIN1, OnWriteMarkREDG1)
	ON_BN_CLICKED(IDC_GR_GAIN1, OnWriteMarkGRG1)
	ON_BN_CLICKED(IDC_BLUE_GAIN1, OnWriteMarkBLUEG1)	
	ON_BN_CLICKED(IDC_IR_GAIN2, OnWriteMarkIRG2)
	ON_BN_CLICKED(IDC_RED_GAIN2, OnWriteMarkREDG2)
	ON_BN_CLICKED(IDC_GR_GAIN2, OnWriteMarkGRG2)
	ON_BN_CLICKED(IDC_BLUE_GAIN2, OnWriteMarkBLUEG2)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_PARAMS, OnDblclkListParams)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, OnButtonUpdate)
	ON_BN_CLICKED(IDC_BUTTON_LUT, OnButtonLut)
	
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_COMBO_FILTER, &CICANDIParamsDlg::OnCbnSelchangeComboFilter)
	ON_CBN_SELCHANGE(IDC_GPU_DEV_LIST, &CICANDIParamsDlg::OnCbnSelchangeGpuDevList)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CICANDIParamsDlg message handlers

void CICANDIParamsDlg::OnButtonAdd() 
{
	int    *vals, rowIdx;
	LVITEM  lvi;
	CString strItem;

	vals = new int[11];

	// TODO: Add your control notification handler code here
	if (ValidateInputs() == FALSE) return;
	if (ValidateAddLine(vals) == FALSE) {
		delete [] vals;
		return;
	} else {
		rowIdx = m_lstPatchInfo.GetItemCount();

		// add new items into the Control List
		lvi.mask =  LVIF_TEXT;
		lvi.iItem = rowIdx;
		// Insert the first item
		strItem.Format(_T("%d"), rowIdx+1);
		lvi.iSubItem = 0;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.InsertItem(&lvi);
		for (int i = 1; i < m_nColumns; i++)
		{
			// Insert the first item
			strItem.Format(_T("%d"), vals[i]);
			lvi.iSubItem = i;
			lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
			m_lstPatchInfo.SetItem(&lvi);
		}

		delete [] vals;
	}

	strItem.Format("%d", rowIdx+1);
	SetDlgItemText(IDC_EDIT_PATCH_CNT, strItem);
}

void CICANDIParamsDlg::OnButtonRemove() 
{
	int     rowIdx, itemCnt;
	LVITEM  lvi;
	CString strItem;

	// retrieve the index of the item that will be deleted
	rowIdx = m_lstPatchInfo.GetSelectionMark();
	if (rowIdx >= 0) m_lstPatchInfo.DeleteItem(rowIdx);

	// reorder patch ID
	itemCnt = m_lstPatchInfo.GetItemCount();
	lvi.mask =  LVIF_TEXT;
	lvi.iSubItem = 0;
	for (int i = 0; i < itemCnt; i ++) {
		strItem.Format(_T("%d"), i+1);
		lvi.iItem = i;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.SetItem(&lvi);
	}

	strItem.Format("%d", itemCnt);
	SetDlgItemText(IDC_EDIT_PATCH_CNT, strItem);
}

void CICANDIParamsDlg::OnButtonUp() 
{
	int     i, rowIdx, itemCnt;
	LVITEM  lvi1, lvi2;
	CString strItem1, strItem2;

	rowIdx = m_lstPatchInfo.GetSelectionMark();
	if (rowIdx <= 0) return;

	// exchange contents in consecutive rows
	lvi1.mask =  LVIF_TEXT;
	lvi1.iItem = rowIdx;
	lvi2.mask =  LVIF_TEXT;
	lvi2.iItem = rowIdx-1;
	for (i = 1; i < m_nColumns; i ++) {
		strItem1 = m_lstPatchInfo.GetItemText(rowIdx, i);
		strItem2 = m_lstPatchInfo.GetItemText(rowIdx-1, i);
		lvi1.iSubItem = i;
		lvi2.iSubItem = i;
		lvi1.pszText = (LPTSTR)(LPCTSTR)(strItem2);
		lvi2.pszText = (LPTSTR)(LPCTSTR)(strItem1);
		m_lstPatchInfo.SetItem(&lvi1);
		m_lstPatchInfo.SetItem(&lvi2);
	}

	// reorder patch index
	itemCnt = m_lstPatchInfo.GetItemCount();
	lvi1.mask =  LVIF_TEXT;
	lvi1.iSubItem = 0;
	for (i = 0; i < itemCnt; i ++) {
		strItem1.Format(_T("%d"), i+1);
		lvi1.iItem = i;
		lvi1.pszText = (LPTSTR)(LPCTSTR)(strItem1);
		m_lstPatchInfo.SetItem(&lvi1);
	}

	m_lstPatchInfo.SetItemState(rowIdx-1, LVIS_SELECTED, LVIS_SELECTED);
	m_lstPatchInfo.SetSelectionMark(rowIdx-1);
}

void CICANDIParamsDlg::OnButtonDown() 
{
	int     i, rowIdx, itemCnt;
	LVITEM  lvi1, lvi2;
	CString strItem1, strItem2;
	
	rowIdx = m_lstPatchInfo.GetSelectionMark();
	itemCnt = m_lstPatchInfo.GetItemCount();
	if (rowIdx < 0 || rowIdx >= itemCnt-1) return;

	// exchange contents in consecutive rows
	lvi1.mask =  LVIF_TEXT;
	lvi1.iItem = rowIdx;
	lvi2.mask =  LVIF_TEXT;
	lvi2.iItem = rowIdx+1;
	for (i = 1; i < m_nColumns; i ++) {
		strItem1 = m_lstPatchInfo.GetItemText(rowIdx, i);
		strItem2 = m_lstPatchInfo.GetItemText(rowIdx+1, i);
		lvi1.iSubItem = i;
		lvi2.iSubItem = i;
		lvi1.pszText = (LPTSTR)(LPCTSTR)(strItem2);
		lvi2.pszText = (LPTSTR)(LPCTSTR)(strItem1);
		m_lstPatchInfo.SetItem(&lvi1);
		m_lstPatchInfo.SetItem(&lvi2);
	}

	// reorder patch index
	itemCnt = m_lstPatchInfo.GetItemCount();
	lvi1.mask =  LVIF_TEXT;
	lvi1.iSubItem = 0;
	for (i = 0; i < itemCnt; i ++) {
		strItem1.Format(_T("%d"), i+1);
		lvi1.iItem = i;
		lvi1.pszText = (LPTSTR)(LPCTSTR)(strItem1);
		m_lstPatchInfo.SetItem(&lvi1);
	}

	m_lstPatchInfo.SetItemState(rowIdx+1, LVIS_SELECTED, LVIS_SELECTED);
	m_lstPatchInfo.SetSelectionMark(rowIdx+1);
}

void CICANDIParamsDlg::OnCheckDesinusoid() 
{
	if (m_chkDesinusoid.GetCheck() == 0) {
		m_intDesinusoid = 0;
		((CButton*)GetDlgItem(IDC_BUTTON_LUT))->EnableWindow(FALSE);
	} else if (m_chkDesinusoid.GetCheck() == 1) {
		m_intDesinusoid = 1;
		((CButton*)GetDlgItem(IDC_BUTTON_LUT))->EnableWindow(TRUE);
	} else {
		m_chkDesinusoid.SetCheck(0);
		m_intDesinusoid = 0;
		((CButton*)GetDlgItem(IDC_BUTTON_LUT))->EnableWindow(FALSE);
		MessageBox("Undertermined Value for Desinusoid check box!!");
	}	
}

void CICANDIParamsDlg::OnInvertMotionTraces() 
{
	if (m_chkInvertMotionTraces.GetCheck() == 0) {
		m_intInvertTraces = 0;		
	} else if (m_chkInvertMotionTraces.GetCheck() == 1) {		
		m_intInvertTraces = 1;
	} else {
		m_chkInvertMotionTraces.SetCheck(0);
		m_intInvertTraces = 0;
		MessageBox("Undertermined Value for Invert Traces check box!!");
	}	
}

void CICANDIParamsDlg::OnWriteDigitalMarks() 
{
	if (m_chkDigitalMarks.GetCheck() == 0) {
		m_boolWriteMarkFlags[8] = 0;
	} else if (m_chkDigitalMarks.GetCheck() == 1) {		
		m_boolWriteMarkFlags[8] = 1;
	} else {
		m_chkDigitalMarks.SetCheck(0);
		m_boolWriteMarkFlags[8] = 0;
		MessageBox("Undertermined Value for IR Channel Write Mark Flag at Gain=1 check box!!");
	}

	m_chkWriteMarkIRG1.EnableWindow(m_boolWriteMarkFlags[8]);
	m_chkWriteMarkREDG1.EnableWindow(m_boolWriteMarkFlags[8]);
	m_chkWriteMarkGRG1.EnableWindow(m_boolWriteMarkFlags[8]);
	m_chkWriteMarkBLUEG1.EnableWindow(m_boolWriteMarkFlags[8]);
	m_chkWriteMarkIRG2.EnableWindow(m_boolWriteMarkFlags[8]);
	m_chkWriteMarkREDG2.EnableWindow(m_boolWriteMarkFlags[8]);
	m_chkWriteMarkGRG2.EnableWindow(m_boolWriteMarkFlags[8]);
	m_chkWriteMarkBLUEG2.EnableWindow(m_boolWriteMarkFlags[8]);
}

void CICANDIParamsDlg::OnWriteMarkIRG1() 
{
	if (m_chkWriteMarkIRG1.GetCheck() == 0) {
		m_boolWriteMarkFlags[7] = 0;		
	} else if (m_chkWriteMarkIRG1.GetCheck() == 1) {		
		m_boolWriteMarkFlags[7] = 1;
	} else {
		m_chkWriteMarkIRG1.SetCheck(0);
		m_boolWriteMarkFlags[7] = 0;
		MessageBox("Undertermined Value for IR Channel Write Mark Flag at Gain=1 check box!!");
	}	
}

void CICANDIParamsDlg::OnWriteMarkREDG1() 
{
	if (m_chkWriteMarkREDG1.GetCheck() == 0) {
		m_boolWriteMarkFlags[6] = 0;		
	} else if (m_chkWriteMarkREDG1.GetCheck() == 1) {		
		m_boolWriteMarkFlags[6] = 1;
	} else {
		m_chkWriteMarkREDG1.SetCheck(0);
		m_boolWriteMarkFlags[6] = 0;
		MessageBox("Undertermined Value for Red Channel Write Mark Flag at Gain=1 check box!!");
	}	
}

void CICANDIParamsDlg::OnWriteMarkGRG1() 
{
	if (m_chkWriteMarkGRG1.GetCheck() == 0) {
		m_boolWriteMarkFlags[5] = 0;		
	} else if (m_chkWriteMarkGRG1.GetCheck() == 1) {		
		m_boolWriteMarkFlags[5] = 1;
	} else {
		m_chkWriteMarkGRG1.SetCheck(0);
		m_boolWriteMarkFlags[5] = 0;
		MessageBox("Undertermined Value for Green Channel Write Mark Flag at Gain=1 check box!!");
	}	
}

void CICANDIParamsDlg::OnWriteMarkBLUEG1() 
{
	if (m_chkWriteMarkBLUEG1.GetCheck() == 0) {
		m_boolWriteMarkFlags[4] = 0;		
	} else if (m_chkWriteMarkBLUEG1.GetCheck() == 1) {		
		m_boolWriteMarkFlags[4] = 1;
	} else {
		m_chkWriteMarkBLUEG1.SetCheck(0);
		m_boolWriteMarkFlags[4] = 0;
		MessageBox("Undertermined Value for Blue Channel Write Mark Flag at Gain=1 check box!!");
	}	
}

void CICANDIParamsDlg::OnWriteMarkIRG2() 
{
	if (m_chkWriteMarkIRG2.GetCheck() == 0) {
		m_boolWriteMarkFlags[3] = 0;		
	} else if (m_chkWriteMarkIRG2.GetCheck() == 1) {		
		m_boolWriteMarkFlags[3] = 1;
	} else {
		m_chkWriteMarkIRG2.SetCheck(0);
		m_boolWriteMarkFlags[3] = 0;
		MessageBox("Undertermined Value for IR Channel Write Mark Flag at Gain!=1 check box!!");
	}	
}

void CICANDIParamsDlg::OnWriteMarkREDG2() 
{
	if (m_chkWriteMarkREDG2.GetCheck() == 0) {
		m_boolWriteMarkFlags[2] = 0;		
	} else if (m_chkWriteMarkREDG2.GetCheck() == 1) {		
		m_boolWriteMarkFlags[2] = 1;
	} else {
		m_chkWriteMarkREDG2.SetCheck(0);
		m_boolWriteMarkFlags[2] = 0;
		MessageBox("Undertermined Value for Red Channel Write Mark Flag at Gain!=1 check box!!");
	}	
}

void CICANDIParamsDlg::OnWriteMarkGRG2() 
{
	if (m_chkWriteMarkGRG2.GetCheck() == 0) {
		m_boolWriteMarkFlags[1] = 0;		
	} else if (m_chkWriteMarkGRG2.GetCheck() == 1) {		
		m_boolWriteMarkFlags[1] = 1;
	} else {
		m_chkWriteMarkGRG2.SetCheck(0);
		m_boolWriteMarkFlags[1] = 0;
		MessageBox("Undertermined Value for Green Channel Write Mark Flag at Gain!=1 check box!!");
	}	
}

void CICANDIParamsDlg::OnWriteMarkBLUEG2() 
{
	if (m_chkWriteMarkBLUEG2.GetCheck() == 0) {
		m_boolWriteMarkFlags[0] = 0;		
	} else if (m_chkWriteMarkBLUEG2.GetCheck() == 1) {		
		m_boolWriteMarkFlags[0] = 1;
	} else {
		m_chkWriteMarkBLUEG2.SetCheck(0);
		m_boolWriteMarkFlags[0] = 0;
		MessageBox("Undertermined Value for Blue Channel Write Mark Flag at Gain!=1 check box!!");
	}	
}

BOOL CICANDIParamsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CRect rect;
	m_lstPatchInfo.GetClientRect(&rect);
	int i;
	int nColInterval = rect.Width()/m_nColumns;
/*
	// add item labels
	m_lstPatchInfo.InsertColumn(0, _T("Pat ID"), LVCFMT_LEFT, nColInterval);
	m_lstPatchInfo.InsertColumn(1, _T("L_XDIM"), LVCFMT_LEFT, nColInterval);
	m_lstPatchInfo.InsertColumn(2, _T("L_YDIM"), LVCFMT_LEFT, nColInterval);
	m_lstPatchInfo.InsertColumn(3, _T("L_YDIM0"), LVCFMT_LEFT, nColInterval);
	m_lstPatchInfo.InsertColumn(4, _T("Xoffset"), LVCFMT_LEFT, nColInterval);
	m_lstPatchInfo.InsertColumn(5, _T("Yoffset"), LVCFMT_LEFT, nColInterval);
	m_lstPatchInfo.InsertColumn(6, _T("fra_Xoffset"), LVCFMT_LEFT, nColInterval);
	m_lstPatchInfo.InsertColumn(7, _T("fra_Yoffset"), LVCFMT_LEFT, nColInterval);

	// initialize control list by reading the values from the global structure g_ICANDIParams
	LVITEM lvi;
	CString strItem;
	for (int i = 0; i < g_ICANDIParams.PATCH_CNT; i++)
	{
		// Insert the first item
		lvi.mask =  LVIF_TEXT;
		strItem.Format(_T("%d"), i+1);
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.InsertItem(&lvi);


		// Set subitem 3
		strItem.Format(_T("%d"), g_ICANDIParams.L_XDIM[i]);
		lvi.iSubItem = 1;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.SetItem(&lvi);
		// Set subitem 4
		strItem.Format(_T("%d"), g_ICANDIParams.L_YDIM[i]);
		lvi.iSubItem = 2;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.SetItem(&lvi);
		// Set subitem 5
		strItem.Format(_T("%d"), g_ICANDIParams.L_YDIM0[i]);
		lvi.iSubItem = 3;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.SetItem(&lvi);
		// Set subitem 6
		strItem.Format(_T("%d"), g_ICANDIParams.X_OFFSET[i]);
		lvi.iSubItem = 4;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.SetItem(&lvi);
		// Set subitem 7
		strItem.Format(_T("%d"), g_ICANDIParams.Y_OFFSET[i]);
		lvi.iSubItem = 5;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.SetItem(&lvi);
		// Set subitem 8
		strItem.Format(_T("%d"), g_ICANDIParams.FRA_X_OFFSET[i]);
		lvi.iSubItem = 6;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.SetItem(&lvi);
		// Set subitem 9
		strItem.Format(_T("%d"), g_ICANDIParams.FRA_Y_OFFSET[i]);
		lvi.iSubItem = 7;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.SetItem(&lvi);
	}

	// set text color and style of the control list
	m_lstPatchInfo.SetTextColor(RGB(0,127,0));
	m_lstPatchInfo.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | 
									LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT);
*/
	if (m_intDesinusoid == 0) {  // without layer Desinusoid
		m_chkDesinusoid.SetCheck(0);
		((CButton*)GetDlgItem(IDC_BUTTON_LUT))->EnableWindow(FALSE);
	} else {					// with layer Desinusoid
		m_chkDesinusoid.SetCheck(1);
		((CButton*)GetDlgItem(IDC_BUTTON_LUT))->EnableWindow(TRUE);
	}

	m_intInvertTraces?m_chkInvertMotionTraces.SetCheck(1):m_chkInvertMotionTraces.SetCheck(0);
	m_boolWriteMarkFlags[8]?m_chkDigitalMarks.SetCheck(1):m_chkDigitalMarks.SetCheck(0);
	m_boolWriteMarkFlags[7]?m_chkWriteMarkIRG1.SetCheck(1):m_chkWriteMarkIRG1.SetCheck(0);
	m_boolWriteMarkFlags[6]?m_chkWriteMarkREDG1.SetCheck(1):m_chkWriteMarkREDG1.SetCheck(0);
	m_boolWriteMarkFlags[5]?m_chkWriteMarkGRG1.SetCheck(1):m_chkWriteMarkGRG1.SetCheck(0);
	m_boolWriteMarkFlags[4]?m_chkWriteMarkBLUEG1.SetCheck(1):m_chkWriteMarkBLUEG1.SetCheck(0);
	m_boolWriteMarkFlags[3]?m_chkWriteMarkIRG2.SetCheck(1):m_chkWriteMarkIRG2.SetCheck(0);
	m_boolWriteMarkFlags[2]?m_chkWriteMarkREDG2.SetCheck(1):m_chkWriteMarkREDG2.SetCheck(0);
	m_boolWriteMarkFlags[1]?m_chkWriteMarkGRG2.SetCheck(1):m_chkWriteMarkGRG2.SetCheck(0);
	m_boolWriteMarkFlags[0]?m_chkWriteMarkBLUEG2.SetCheck(1):m_chkWriteMarkBLUEG2.SetCheck(0);


	((CComboBox*)GetDlgItem(IDC_COMBO_FILTER))->ResetContent();
	((CComboBox*)GetDlgItem(IDC_COMBO_FILTER))->InsertString(0, _T("3x3 Gauss"));
	((CComboBox*)GetDlgItem(IDC_COMBO_FILTER))->InsertString(1, _T("5x5 Gauss"));
	((CComboBox*)GetDlgItem(IDC_COMBO_FILTER))->InsertString(2, _T("No Filter"));
	((CComboBox*)GetDlgItem(IDC_COMBO_FILTER))->SetCurSel(m_intFilterID);

	// do additional job for listing CPU devices
	if (m_gpu_dev_list == NULL || m_gpu_counts == 0) {
		((CButton*)GetDlgItem(IDC_GPU_DEV_LIST))->EnableWindow(FALSE);
	} else {
		((CComboBox*)GetDlgItem(IDC_GPU_DEV_LIST))->ResetContent();
		for (i = 0; i < m_gpu_counts; i ++) {
			((CComboBox*)GetDlgItem(IDC_GPU_DEV_LIST))->InsertString(i, m_gpu_dev_list[i]);
		}
	}

	// use GPU-based FFT for stabilization
	((CButton*)GetDlgItem(IDC_GPU_DEV_LIST))->EnableWindow(TRUE);

	if (m_intGPUID < m_gpu_counts) {
		((CComboBox*)GetDlgItem(IDC_GPU_DEV_LIST))->SetCurSel(m_intGPUID);
	} else {
		((CComboBox*)GetDlgItem(IDC_GPU_DEV_LIST))->SetCurSel(0);
		m_intGPUID = 0;
	}
//		m_GPUchosen = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CICANDIParamsDlg::ValidateInputs()
{
	int    F_XDIM, F_YDIM;
	char  *text;

	// check validity of the inputs
	text = new char [8];
	GetDlgItemText(IDC_EDIT_F_XDIM, text, 8);
	F_XDIM = atoi(text);
	GetDlgItemText(IDC_EDIT_F_YDIM, text, 8);
	F_YDIM = atoi(text);
/*	GetDlgItemText(IDC_EDIT_PATCH_CNT, text, 8);
	PATCH_CNT = atoi(text);
	GetDlgItemText(IDC_EDIT_LAYER_CNT, text, 8);
	LAYER_CNT = atoi(text);
	GetDlgItemText(IDC_EDIT_ITER_NUM, text, 8);
	ITERNUM = atoi(text);
	GetDlgItemText(IDC_EDIT_K1, text, 8);
	K1 = atoi(text);
	GetDlgItemText(IDC_EDIT_GTHRESH, text, 8);
	GTHRESH = atoi(text);
	GetDlgItemText(IDC_EDIT_SRCTHRESH, text, 8);
	SRCTHRESH = atoi(text);*/

	delete [] text;

	if (F_XDIM < 1 || F_XDIM > 2048) {
		MessageBox("Invalid frame width (Enter 1~2048)", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (F_YDIM < 1 || F_YDIM > 2048) {
		MessageBox("Invalid frame height (Enter 1~2048)", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
/*	if (PATCH_CNT < 1 || PATCH_CNT > 128) {
		MessageBox("Invalid patch counts (Enter 1~64))", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (LAYER_CNT < 2 || LAYER_CNT > 10) {
		MessageBox("Invalid layer counts (Enter 1~10))", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (ITERNUM < 2 || ITERNUM > 15) {
		MessageBox("Invalid iteration number (Enter 2~15))", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (K1 < 0 || K1 > 65536) {
		MessageBox("Invalid K1 (Enter 0~65536))", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (GTHRESH < 0 || GTHRESH > 255) {
		MessageBox("Invalid GTHRESH (Enter 0~255)", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (SRCTHRESH < 0 || SRCTHRESH > 255) {
		MessageBox("Invalid SRCTHRESH (Enter 0~255)", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
*/
	return TRUE;
}

BOOL CICANDIParamsDlg::ValidateAddLine(int *vals)
{
	int    F_XDIM, F_YDIM;
	char  *text;

	// check validity of inputs
	text = new char [8];
	GetDlgItemText(IDC_EDIT_F_XDIM, text, 8);
	F_XDIM = atoi(text);
	GetDlgItemText(IDC_EDIT_F_YDIM, text, 8);
	F_YDIM = atoi(text);
//	GetDlgItemText(IDC_EDIT_PAT_X, text, 8);
//	PATCH_WIDTH = atoi(text);
//	GetDlgItemText(IDC_EDIT_PAT_Y, text, 8);
//	PATCH_HEIGHT = atoi(text);
/*	GetDlgItemText(IDC_EDIT_L_XDIM, text, 8);
	L_XDIM = atoi(text);
	GetDlgItemText(IDC_EDIT_L_YDIM, text, 8);
	L_YDIM = atoi(text);
	GetDlgItemText(IDC_EDIT_L_YDIM_0, text, 8);
	L_YDIM0 = atoi(text);
	GetDlgItemText(IDC_EDIT_X_OFFSET, text, 8);
	X_OFFSET = atoi(text);
	GetDlgItemText(IDC_EDIT_Y_OFFSET, text, 8);
	Y_OFFSET = atoi(text);
	GetDlgItemText(IDC_EDIT_X_SHIFT, text, 8);
	X_SHIFT = atoi(text);
	GetDlgItemText(IDC_EDIT_Y_SHIFT, text, 8);
	Y_SHIFT = atoi(text);
*/
	/*
	if (PATCH_WIDTH < 1 || PATCH_WIDTH > F_XDIM) {
		MessageBox("Invalid patch width", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (PATCH_HEIGHT < 1 || PATCH_HEIGHT > F_YDIM) {
		MessageBox("Invalid patch height", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}*/
/*	if (L_XDIM < 1 || L_XDIM > F_XDIM) {
		MessageBox("Invalid L_XDIM", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (L_YDIM < 1 || L_YDIM > F_YDIM) {
		MessageBox("Invalid L_YDIM", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (L_YDIM < 0 || L_YDIM >= F_YDIM) {
		MessageBox("Invalid L_YDIM0", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (X_OFFSET < 1 || X_OFFSET > L_XDIM) {
		MessageBox("Invalid X offset", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (Y_OFFSET < 1 || Y_OFFSET > L_YDIM) {
		MessageBox("Invalid Y offset", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (X_SHIFT < 0 || X_SHIFT > 255) {
		MessageBox("Invalid frame X offset (Enter 0~255)", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}
	if (Y_SHIFT < 0 || Y_SHIFT > 255) {
		MessageBox("Invalid frame Y offset (Enter 0~255)", "Invalid input", MB_ICONWARNING);
		return FALSE;
	}*/
/*
	vals[1] = PATCH_WIDTH;
	vals[2] = PATCH_HEIGHT;
	vals[3] = L_XDIM;
	vals[4] = L_YDIM;
	vals[5] = L_YDIM0;
	vals[6] = X_OFFSET;
	vals[7] = Y_OFFSET;
	vals[8] = X_SHIFT;
	vals[9] = Y_SHIFT;
*/
/*	vals[1] = L_XDIM;
	vals[2] = L_YDIM;
	vals[3] = L_YDIM0;
	vals[4] = X_OFFSET;
	vals[5] = Y_OFFSET;
	vals[6] = X_SHIFT;
	vals[7] = Y_SHIFT;

*/	return TRUE;
}

void CICANDIParamsDlg::OnDblclkListParams(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int     rowIdx;
	CString strItem;

	rowIdx = m_lstPatchInfo.GetSelectionMark();

	// after double click, copy the selected row on the control list
	// to the input line
/*	strItem = m_lstPatchInfo.GetItemText(rowIdx, 1);
	SetDlgItemText(IDC_EDIT_PAT_X, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 2);
	SetDlgItemText(IDC_EDIT_PAT_Y, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 3);
	SetDlgItemText(IDC_EDIT_L_XDIM, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 4);
	SetDlgItemText(IDC_EDIT_L_YDIM, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 5);
	SetDlgItemText(IDC_EDIT_L_YDIM_0, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 6);
	SetDlgItemText(IDC_EDIT_X_OFFSET, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 7);
	SetDlgItemText(IDC_EDIT_Y_OFFSET, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 8);
	SetDlgItemText(IDC_EDIT_X_SHIFT, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 9);
	SetDlgItemText(IDC_EDIT_Y_SHIFT, strItem);
*/
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 1);
	SetDlgItemText(IDC_EDIT_L_XDIM, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 2);
	SetDlgItemText(IDC_EDIT_L_YDIM, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 3);
	SetDlgItemText(IDC_EDIT_L_YDIM_0, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 4);
	SetDlgItemText(IDC_EDIT_X_OFFSET, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 5);
	SetDlgItemText(IDC_EDIT_Y_OFFSET, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 6);
	SetDlgItemText(IDC_EDIT_X_SHIFT, strItem);
	strItem = m_lstPatchInfo.GetItemText(rowIdx, 7);
	SetDlgItemText(IDC_EDIT_Y_SHIFT, strItem);

	*pResult = 0;
}

BOOL CICANDIParamsDlg::ValidateLists()
{
	int     i, itemCnt, pat_heights, pat_start_old = 0;
	int    *L_XDIM, *L_YDIM, *L_YDIM0;
	int    *X_OFFSET, *Y_OFFSET, *X_SHIFT, *Y_SHIFT, F_XDIM, F_YDIM;
	char   *strItem;
	CString msg;

	itemCnt = m_lstPatchInfo.GetItemCount();
	strItem = new char [8];

//	PATCH_WIDTH = new int [itemCnt];
//	PATCH_HEIGHT = new int [itemCnt];
	L_XDIM = new int [itemCnt];
	L_YDIM = new int [itemCnt];
	L_YDIM0 = new int [itemCnt];
	X_OFFSET = new int [itemCnt];
	Y_OFFSET = new int [itemCnt];
	X_SHIFT = new int [itemCnt];
	Y_SHIFT = new int [itemCnt];

	delete [] g_ICANDIParams.L_XDIM;
	delete [] g_ICANDIParams.L_YDIM;
	delete [] g_ICANDIParams.L_YDIM0;
	delete [] g_ICANDIParams.X_OFFSET;
	delete [] g_ICANDIParams.Y_OFFSET;
	delete [] g_ICANDIParams.FRA_X_OFFSET;
	delete [] g_ICANDIParams.FRA_Y_OFFSET;

	g_ICANDIParams.PATCH_CNT = itemCnt;
	g_ICANDIParams.L_XDIM = new int [itemCnt];
	g_ICANDIParams.L_YDIM = new int [itemCnt];
	g_ICANDIParams.L_YDIM0 = new int [itemCnt];
	g_ICANDIParams.X_OFFSET = new int [itemCnt];
	g_ICANDIParams.Y_OFFSET = new int [itemCnt];
	g_ICANDIParams.FRA_X_OFFSET = new int [itemCnt];
	g_ICANDIParams.FRA_Y_OFFSET = new int [itemCnt];

	GetDlgItemText(IDC_EDIT_F_XDIM, strItem, 8);
	F_XDIM = atoi(strItem);
	GetDlgItemText(IDC_EDIT_F_YDIM, strItem, 8);
	F_YDIM = atoi(strItem);

	// read in all data on the lists
	pat_heights = 0;
	for (i = 0; i < itemCnt; i ++) {
/*		m_lstPatchInfo.GetItemText(i, 1, strItem, 8);
		PATCH_WIDTH[i] = atoi(strItem);
		if (PATCH_WIDTH[i] > F_XDIM) {
			msg.Format("Invalid patch width on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 2, strItem, 8);
		PATCH_HEIGHT[i] = atoi(strItem);
		pat_heights += PATCH_HEIGHT[i];
		if (PATCH_HEIGHT[i] > F_YDIM) {
			msg.Format("Invalid patch height on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 3, strItem, 8);
		L_XDIM[i] = atoi(strItem);
		if (L_XDIM[i] > F_XDIM) {
			msg.Format("Invalid L_XDIM on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 4, strItem, 8);
		L_YDIM[i] = atoi(strItem);
		if (L_YDIM[i] > F_YDIM) {
			msg.Format("Invalid L_XDIM on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 5, strItem, 8);
		L_YDIM0[i] = atoi(strItem);
		if (L_YDIM0[i] < 0 || L_YDIM0[i] >= F_YDIM) {
			msg.Format("Invalid L_XDIM0 on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		if (pat_start_old > L_YDIM0[i]) {
			msg.Format("Invalid patch order on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		pat_start_old = L_YDIM0[i];

		m_lstPatchInfo.GetItemText(i, 6, strItem, 8);
		X_OFFSET[i] = atoi(strItem);
		if (X_OFFSET[i] > L_XDIM[i]) {
			msg.Format("Invalid X_OFFSET (larger than L_XDIM) on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 7, strItem, 8);
		Y_OFFSET[i] = atoi(strItem);
		if (Y_OFFSET[i] > F_YDIM) {
			msg.Format("Invalid Y_OFFSET (larger than L_YDIM) on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 8, strItem, 8);
		X_SHIFT[i] = atoi(strItem);
		if (X_SHIFT[i] < 0 || X_SHIFT[i] > 255) {
			msg.Format("Invalid frame X_OFFSET (valid: 0~255) on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 9, strItem, 8);
		Y_SHIFT[i] = atoi(strItem);
		if (Y_SHIFT[i] < 0 || Y_SHIFT[i] > 255) {
			msg.Format("Invalid frame Y_OFFSET (valid: 0~255) on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}*/
		m_lstPatchInfo.GetItemText(i, 1, strItem, 8);
		L_XDIM[i] = atoi(strItem);
		if (L_XDIM[i] > F_XDIM) {
			msg.Format("Invalid L_XDIM on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 2, strItem, 8);
		L_YDIM[i] = atoi(strItem);
		if (L_YDIM[i] > F_YDIM) {
			msg.Format("Invalid L_XDIM on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 3, strItem, 8);
		L_YDIM0[i] = atoi(strItem);
		if (L_YDIM0[i] < 0 || L_YDIM0[i] >= F_YDIM) {
			msg.Format("Invalid L_XDIM0 on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		if (pat_start_old > L_YDIM0[i]) {
			msg.Format("Invalid patch order on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		pat_start_old = L_YDIM0[i];

		m_lstPatchInfo.GetItemText(i, 4, strItem, 8);
		X_OFFSET[i] = atoi(strItem);
		if (X_OFFSET[i] > L_XDIM[i]) {
			msg.Format("Invalid X_OFFSET (larger than L_XDIM) on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 5, strItem, 8);
		Y_OFFSET[i] = atoi(strItem);
		if (Y_OFFSET[i] > F_YDIM) {
			msg.Format("Invalid Y_OFFSET (larger than L_YDIM) on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 6, strItem, 8);
		X_SHIFT[i] = atoi(strItem);
		if (X_SHIFT[i] < 0 || X_SHIFT[i] > 255) {
			msg.Format("Invalid frame X_OFFSET (valid: 0~255) on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
		m_lstPatchInfo.GetItemText(i, 7, strItem, 8);
		Y_SHIFT[i] = atoi(strItem);
		if (Y_SHIFT[i] < 0 || Y_SHIFT[i] > 255) {
			msg.Format("Invalid frame Y_OFFSET (valid: 0~255) on row %d", i+1);
			MessageBox(msg, "Invalid data", MB_ICONWARNING);
			return FALSE;
		}
	}
/*
	if (pat_heights != F_YDIM) {
		msg.Format("Invalid summation of patch heights (= %d). It should be equal to frame height (= %d)", 
					pat_heights, F_YDIM);
		MessageBox(msg, "Invalid data", MB_ICONWARNING);
		return FALSE;
	}
*/
	for (i = 0; i < itemCnt; i ++) {
//		g_ICANDIParams.patchWidth[i] = PATCH_WIDTH[i];
//		g_ICANDIParams.patchHeight[i] = PATCH_HEIGHT[i];
		g_ICANDIParams.L_XDIM[i] = L_XDIM[i];
		g_ICANDIParams.L_YDIM[i] = L_YDIM[i];
		g_ICANDIParams.L_YDIM0[i] = L_YDIM0[i];
		g_ICANDIParams.X_OFFSET[i] = X_OFFSET[i];
		g_ICANDIParams.Y_OFFSET[i] = Y_OFFSET[i];
		g_ICANDIParams.FRA_X_OFFSET[i] = X_SHIFT[i];
		g_ICANDIParams.FRA_Y_OFFSET[i] = Y_SHIFT[i];
	}

	delete [] strItem;
//	delete [] PATCH_WIDTH;
//	delete [] PATCH_HEIGHT;
	delete [] L_XDIM;
	delete [] L_YDIM;
	delete [] L_YDIM0;
	delete [] X_OFFSET;
	delete [] Y_OFFSET;
	delete [] X_SHIFT;
	delete [] Y_SHIFT;

	return TRUE;
}

void CICANDIParamsDlg::OnOK() 
{
	// TODO: Add extra validation here
	if (ValidateInputs() == FALSE) return;
//	if (ValidateLists() == FALSE) return;

	CString   msg;
	msg = m_LUTname;
	msg.TrimLeft(' ');
	msg.TrimRight(' ');
	if (msg.GetLength() == 0 && m_intDesinusoid == 1) {
		MessageBox("Error: no lookup table file name is loaded.", "Invalid Input", MB_ICONEXCLAMATION);
		m_intDesinusoid = 0;
	}

	CDialog::OnOK();
}


void CICANDIParamsDlg::OnButtonUpdate() 
{
	int    *vals, rowIdx;
	LVITEM  lvi;
	CString strItem;

	vals = new int[m_nColumns];
	if (ValidateAddLine(vals) == FALSE) {
		delete [] vals;
		return;
	}

	rowIdx = m_lstPatchInfo.GetSelectionMark();

	// add new items into the Control List
	lvi.mask =  LVIF_TEXT;
	lvi.iItem = rowIdx;
	for (int i = 1; i < m_nColumns; i++)
	{
		// Insert the first item
		strItem.Format(_T("%d"), vals[i]);
		lvi.iSubItem = i;
		lvi.pszText = (LPTSTR)(LPCTSTR)(strItem);
		m_lstPatchInfo.SetItem(&lvi);
	}

	delete [] vals;
}

void CICANDIParamsDlg::OnButtonLut() 
{
	// open a .lut file (look up table)
	char     BASED_CODE szFilter[] = "Look up table files (*.lut)|*.lut|";
	CString  lutFileName;

	CFileDialog fd(TRUE,"lut",NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOREADONLYRETURN, "Look up table files (*.lut)|*.lut||");
	
	if (g_ICANDIParams.fnameLUT.GetLength() != 0)
		fd.m_ofn.lpstrInitialDir = g_ICANDIParams.fnameLUT.Left(g_ICANDIParams.fnameLUT.ReverseFind('\\'));
	else
		fd.m_ofn.lpstrInitialDir = g_ICANDIParams.VideoFolderPath;

	if (fd.DoModal() != IDOK) {
		m_LUTname =  "";
	} else {
		m_LUTname = fd.GetPathName();	
	}
}

void CICANDIParamsDlg::OnCbnSelchangeComboFilter()
{
	m_intFilterID = ((CComboBox*)GetDlgItem(IDC_COMBO_FILTER))->GetCurSel();
}

void CICANDIParamsDlg::OnCbnSelchangeGpuDevList()
{
	m_intGPUID = ((CComboBox*)GetDlgItem(IDC_GPU_DEV_LIST))->GetCurSel();
	m_intGPUIDold = m_intGPUID;
}