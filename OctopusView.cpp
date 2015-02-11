/**********************************************************************************
***************************   OctopusView           *******************************
**********************************************************************************/

#include "stdafx.h"
#include "MainFrm.h"
#include "Octopus.h"
#include "OctopusView.h"
#include "OctopusLog.h"
#include "OctopusClock.h"
#include "OctopusCameraDlg.h"
#include "OctopusGlobals.h"
#include "OctopusMPC200.h"

COctopusCamera*            glob_m_pCamera          = NULL;
COctopusMPC200*            glob_m_pMPC200          = NULL;
COctopusGoodClock          GoodClock;
COctopusGoodClock*         glob_m_pGoodClock = &GoodClock;
COctopusLog                Log;
COctopusLog*               glob_m_pLog = &Log;
COctopusGlobals            B;

IMPLEMENT_DYNCREATE(COctopusView, CFormView)

BEGIN_MESSAGE_MAP(COctopusView, CFormView)
	ON_BN_CLICKED(IDC_STATUS_OPEN_ANDOR,    OnOpenAndor)
	ON_BN_CLICKED(IDC_STATUS_OPEN_MPC200,   OnOpenMPC200)
END_MESSAGE_MAP()

void COctopusView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange( pDX );
}  

COctopusView::COctopusView():CFormView(COctopusView::IDD) 
{
}

void COctopusView::OnInitialUpdate() 
{
	B.Andor_new = true;
	CFormView::OnInitialUpdate();
}

COctopusView::~COctopusView() 
{
	if( glob_m_pCamera != NULL ) 
	{
		if ( B.Camera_Thread_running ) 
			glob_m_pCamera->StopCameraThread();
		glob_m_pCamera->DestroyWindow();
		delete glob_m_pCamera;
		glob_m_pCamera = NULL;
	}
	if( glob_m_pMPC200 != NULL ) 
	{
		glob_m_pMPC200->DestroyWindow();
		delete glob_m_pMPC200;
		glob_m_pMPC200 = NULL;
	}
}

void COctopusView::OnOpenAndor( void ) 
{	
	GetDlgItem( IDC_STATUS_OPEN_ANDOR )->EnableWindow( false );

	if( glob_m_pCamera == NULL ) 
	{
		glob_m_pCamera = new COctopusCamera( this );
	} 
	else 
	{
		if ( B.Camera_Thread_running )
		{
			AfxMessageBox(_T("Yipes - the camera is running in movie mode,\nand you just tried to close it!\nPlease turn it off first!"));
		}
		else if( glob_m_pCamera->IsWindowVisible() ) 
		{
			glob_m_pCamera->DestroyWindow();
			delete glob_m_pCamera;
			glob_m_pCamera = NULL;
		} 
		else 
		{
			glob_m_pCamera->ShowWindow( SW_RESTORE );
		}
	}
	GetDlgItem( IDC_STATUS_OPEN_ANDOR )->EnableWindow( true );
}

void COctopusView::OnOpenMPC200( void ) 
{	
	
	GetDlgItem( IDC_STATUS_OPEN_MPC200 )->EnableWindow( false );

	if( glob_m_pMPC200 == NULL ) 
	{
		glob_m_pMPC200 = new COctopusMPC200( this );

		if ( !B.MPC200_loaded ) 
		{
			glob_m_pMPC200->DestroyWindow();
			delete glob_m_pMPC200;
			glob_m_pMPC200 = NULL;
		}
	} 
	else 
	{
		if( glob_m_pMPC200->IsWindowVisible() ) 
		{
			glob_m_pMPC200->StopEverything();
			glob_m_pMPC200->DestroyWindow();
			delete glob_m_pMPC200;
			glob_m_pMPC200 = NULL;
		} 
		else {
			glob_m_pMPC200->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_MPC200 )->EnableWindow( true );
	
}

