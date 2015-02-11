/*****************************************************************************
 Octopus microscope control software
 
 Copyright (C) 2004-2015 Jan Liphardt (jan.liphardt@stanford.edu)
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 ******************************************************************************/

/***************************Basically ready for Mindy*************************/
/*****************************************************************************/

#include "stdafx.h"
#include "Octopus.h"
#include "OctopusMPC200.h"
#include "OctopusGlobals.h"
#include "OctopusClock.h"
#include "OctopusCameraDlg.h"
#include "roe200.h"
#include "OctopusLog.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <direct.h>

extern COctopusGlobals	    B;
extern COctopusGoodClock*   glob_m_pGoodClock;
extern COctopusCamera*      glob_m_pCamera;
extern COctopusLog*         glob_m_pLog;

using namespace std;

DWORD WINAPI MPC200CommandBuffer( LPVOID lpParameter )
{
	COctopusMPC200* this_ = (COctopusMPC200*)lpParameter; 

	while ( this_ -> run_the_wait_thread )
	{
		this_ -> GetPosition();
		
		Sleep(300);
		
		if ( this_ -> run_the_wait_thread ) 
		{ 
			if( this_->m_SeqList.GetCount() > 0 )
			{
				this_->ExecuteNextCommand();
			}
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// COctopusMPC200 dialog

COctopusMPC200::COctopusMPC200(CWnd* pParent)
	: CDialog(COctopusMPC200::IDD, pParent)
{

	actuator = NULL;
	actuator = new ROE200;
	
	moving_x = false;
	m_AXIS   = 1; // 0 = X

	m_nNewX   = 0;
	m_nNewY   = 0;
	m_nNewZ   = 0;

	m_nID     = 1;
	m_nSpeed  = 3;
	
	m_nXCurr_Micron = 0;
	m_nYCurr_Micron = 0;
	m_nZCurr_Micron = 0;

	   XCurr_Step   = 0;
	   YCurr_Step   = 0;
	   ZCurr_Step   = 0;

	m_nStepX  = 0;
	m_nStepY  = 0;
	m_nStepZ  = 0;
	
	VERIFY(m_bmp_no.LoadBitmap(IDB_NO));
	VERIFY(m_bmp_yes.LoadBitmap(IDB_YES));

	B.MPC200_loaded     = false;
	moving              = false;
	run_the_wait_thread = false;

	systemtime = CTime::GetCurrentTime();
    CString t  = systemtime.Format(_T("%H_%M_%S"));
	CString d  = systemtime.Format(_T("%m_%d_%y/"));
	std::string dd=(CT2CA(d));
	std::string tt=(CT2CA(t));
	string directory = "C:/Users/Lab/Documents/data/forceprobe/";
	directory.append(dd);
	const char * dir = directory.c_str();
	mkdir(dir);

	directory.append("logs/");
	const char * dir2 = directory.c_str();
	mkdir(dir2);

	std::string logout = directory;
	logout.append("logfile_");
	logout.append(tt);
	logout.append(".txt");
	const char *c = logout.c_str();
	pFile = fopen(c, "wt");

	if( Create(COctopusMPC200::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

	SetTimer( TIMER_MPC, 100, NULL ); 
	
}

BOOL COctopusMPC200::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if( actuator->Initialize() == 0 )
	{
		B.MPC200_loaded = true;
		wait_thread_cmd = NULL;
		run_the_wait_thread = true;
		DWORD wait_thread_cmd_id = NULL;
		m_SeqList.ResetContent();
		wait_thread_cmd = CreateThread( 0, 4096, MPC200CommandBuffer, (LPVOID)this, 0, &wait_thread_cmd_id );
	};

	return TRUE;
}

void COctopusMPC200::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control( pDX, IDC_POS_X, m_X_Now ); 
	DDX_Control( pDX, IDC_POS_Y, m_Y_Now ); 
	DDX_Control( pDX, IDC_POS_Z, m_Z_Now ); 

	DDX_Text(pDX, IDC_MPC_RE_SPEED, m_nSpeed);
	DDV_MinMaxInt(pDX, m_nSpeed, 0, 15);

	DDX_Text(pDX, IDC_MPC_RE_dX, m_nStepX);
	DDV_MinMaxLong(pDX, m_nStepX, -2000, 2000);

	DDX_Text(pDX, IDC_MPC_RE_dY, m_nStepY);
	DDV_MinMaxLong(pDX, m_nStepY, -2000, 2000);

	DDX_Text(pDX, IDC_MPC_RE_dZ, m_nStepZ);
	DDV_MinMaxLong(pDX, m_nStepZ, -2000, 2000);
	
	DDX_Radio( pDX, IDC_MPC_X, m_AXIS );
	DDX_Control(pDX, IDC_MPC200_LIST_SEQUENCE, m_SeqList);

}

BEGIN_MESSAGE_MAP(COctopusMPC200, CDialog)
	//{{AFX_MSG_MAP(COctopusMPC200)
	ON_BN_CLICKED(	IDC_MPC_STOP,       OnStop)
	ON_BN_CLICKED(	IDC_MPC_GO_CENTER,  OnGotoCenter) 
	ON_BN_CLICKED(  IDC_MPC_GO_XYZ,     OnStepXYZ)
	ON_EN_KILLFOCUS(IDC_MPC_RE_SPEED,   OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_RE_dX,      OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_RE_dY,      OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_RE_dZ,      OnKillfocusGeneral)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void COctopusMPC200::OnTimer( UINT_PTR nIDEvent ) 
{
	if( nIDEvent == TIMER_MPC ) 
	{
		CString str;
		
        str.Format(_T("X: %0.3f"), m_nXCurr_Micron); m_X_Now.SetWindowText( str );
		str.Format(_T("Y: %0.3f"), m_nYCurr_Micron); m_Y_Now.SetWindowText( str );
		str.Format(_T("Z: %0.3f"), m_nZCurr_Micron); m_Z_Now.SetWindowText( str );

		systemtime     = CTime::GetCurrentTime();
		CString stime  = systemtime.Format(_T(" %m/%d/%y %H:%M:%S "));
		
        CString out;
        out.Format(_T("T:%.3f "), (double)glob_m_pGoodClock->End());
        out.Append( stime );
        out.AppendFormat( _T(" MPC200 POS: %f %f %f\n"), m_nXCurr_Micron, m_nYCurr_Micron, m_nZCurr_Micron);
        fprintf( pFile, out );
        fflush( pFile );
    }
    
	CDialog::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////

BOOL COctopusMPC200::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int id = LOWORD(wParam); // Notification code
	if( id == 2 ) return FALSE; // Trap ESC key
	if( id == 1 ) return FALSE; // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}

void COctopusMPC200::OnKillfocusGeneral()
{
    UpdateData( true );
}

//***********************************************************************
//***********************************************************************
//***********************************************************************
//***********************************************************************
//***********************************************************************

void COctopusMPC200::ExecuteNextCommand( void )
{

	if (run_the_wait_thread == false) return;

	CString command;
    
	m_SeqList.GetText(0, command);
	
	if (command.Left(6) == "CENTER")
	{
		actuator->GotoCenter();
		m_SeqList.DeleteString(0);
	}
	else if ( command.Left(7) == "STEPXYZ")
	{
		actuator->StepXYZ( m_nStepX, m_nStepY, m_nStepZ, m_nSpeed );
		m_SeqList.DeleteString(0);
	}
	else if ( command.Left(7) == "STOP")
	{
		actuator->StopMovingNow(); 
		m_SeqList.DeleteString(0);
	}
}

//**********************************************************************************

void COctopusMPC200::StopEverything( void ) 
{
	KillTimer( TIMER_MPC );
	run_the_wait_thread = false;
	Sleep(100);
	WaitForSingleObject(wait_thread_cmd, 2000);
	Sleep(100);
}

COctopusMPC200::~COctopusMPC200() 
{
	if( actuator != NULL )
	{
		actuator->Close();
		delete actuator;
		actuator = NULL;
	}

	B.MPC200_loaded = false;
}

//**********************************************************************************

void COctopusMPC200::GetPosition( void ) 
{
	if ( run_the_wait_thread == false ) return;

	actuator->TryUpdatePosition();
	
	m_nXCurr_Micron = actuator->GetX();
	m_nYCurr_Micron = actuator->GetY();
	m_nZCurr_Micron = actuator->GetZ();

	B.MPC_m_nXCurr_Micron = m_nXCurr_Micron;
	B.MPC_m_nYCurr_Micron = m_nYCurr_Micron;
	B.MPC_m_nZCurr_Micron = m_nZCurr_Micron;
}

void COctopusMPC200::OnGotoCenter( void )
{
    m_SeqList.AddString("CENTER");
}

void COctopusMPC200::OnStepXYZ( void )
{
    m_SeqList.AddString("STEPXYZ");
}

void COctopusMPC200::OnStop( void )
{
    m_SeqList.AddString("STOP");
}






