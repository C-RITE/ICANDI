// SockListener.cpp: implementation of the CSockListener class.
//
//////////////////////////////////////////////////////////////////////

#include "../../stdafx.h"
#include "SockListener.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CSockListener::CSockListener(CString* buffer, HANDLE* eventid) :
	m_psockClient( NULL )
{
		netbuff = buffer;
		eID = eventid;
}

CSockListener::~CSockListener()
{
	if( m_psockClient )
		delete m_psockClient;
}



// ***************************************************************************
//DESCRIPTION:
//      Called by the framework to notify a listening socket that it can 
//      accept pending connection requests by calling the Accept member 
//      function.
//PARAMS:
//      The most recent error on a socket. The following error codes 
//      applies to the OnAccept member function: 
//       0            The function executed successfully.
//       WSAENETDOWN  The Windows Sockets implementation detected 
//                    that the network subsystem failed. 
// ***************************************************************************
void CSockListener::OnAccept( int nError )
{
    printf( _T("CSockListener::OnAccept(%d)\n"), nError );
    //Close of the old connection and create a new one
	if( m_psockClient )
		delete m_psockClient;
	Sleep( 1 );

	//Create the new connection
    m_psockClient = new CSockClient(netbuff, eID);
    if( Accept( m_psockClient ) == 0 )
    {
		char szErrMessage[WSA_ERROR_LEN+1];
		WSAGetLastErrorMessage( szErrMessage );
	    printf( _T("ERROR on Accept %s\n"), szErrMessage );
		return;
    }

	//Dsiplay connection details
	char szAddress[17];
	int nPort;
	if( m_psockClient->GetPeerName( szAddress, &nPort ) == 0 )
	{
	   printf( _T("Connected to %s on port %d\n"), szAddress, nPort );
	}
	netbuff->Format("Accepted##");
	SetEvent(*eID);
}

bool CSockListener::InitPort( LPCTSTR sAddress, int PortID )
{
	return(CWinSock2Async::Create(sAddress, PortID));
}
