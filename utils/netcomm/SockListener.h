// SockListener.h: interface for the CSockListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOCKLISTENER_H__CD3719C0_D197_4235_BCF8_B3EC909E0F23__INCLUDED_)
#define AFX_SOCKLISTENER_H__CD3719C0_D197_4235_BCF8_B3EC909E0F23__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CSockListener;

#include "WinSock2Async.h"
#include "SockClient.h"

class CSockListener : public CWinSock2Async
{
public:
	CSockListener(CString*, HANDLE*);
	virtual ~CSockListener();
	bool InitPort(LPCTSTR sAddress = "127.0.0.1", int PortID = 0);
	CString* netbuff;
	HANDLE* eID;
	
//Event handlers
private:
    void OnAccept( int nError );

//Attributes
private:
    CSockClient* m_psockClient;						//Client connection

};

#endif // !defined(AFX_SOCKLISTENER_H__CD3719C0_D197_4235_BCF8_B3EC909E0F23__INCLUDED_)
