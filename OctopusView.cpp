/**********************************************************************************
***************************   OctopusView           *******************************
**********************************************************************************/

#include "stdafx.h"
#include "MainFrm.h"
#include "Octopus.h"
#include "OctopusView.h"
#include "OctopusClock.h"
#include "OctopusCameraDlg.h"
#include "OctopusMotor.h"
#include "OctopusGlobals.h"
#include "OctopusMPC200.h"
//#include "OctopusScope.h"
//#include "OctopusStage545.h"
//#include "OctopusStage686.h"
//#include "OctopusAOTF.h"
//#include "OctopusLED.h"
#include "OctopusLog.h"
//#include "OctopusObjectivePiezo.h"
//#include "OctopusLasers.h"

COctopusCamera*            glob_m_pCamera          = NULL;
COctopusMPC200*            glob_m_pMPC200          = NULL;
//COctopusShutterAndWheel*   glob_m_pShutterAndWheel = NULL;
//COctopusScript*            glob_m_pScript          = NULL;
//COctopusLED*               glob_m_pLED             = NULL;
//COctopusAOTF*              glob_m_pAOTF            = NULL;
//scope 1;
//COctopusStage545*          glob_m_pStage545        = NULL;
//scope 2;
COctopusMotor*          glob_m_pMotor		   = NULL;
//COctopusObjectivePiezo*    glob_m_pObjPiezo        = NULL;
//COctopusLasers*            glob_m_pLasers          = NULL;

//COctopusScope*             glob_m_pScope = NULL;

COctopusGoodClock          GoodClock;
COctopusGoodClock*         glob_m_pGoodClock = &GoodClock;

COctopusLog                Log;
COctopusLog*               glob_m_pLog = &Log;


COctopusGlobals            B;

IMPLEMENT_DYNCREATE(COctopusView, CFormView)

BEGIN_MESSAGE_MAP(COctopusView, CFormView)
	ON_BN_CLICKED(IDC_STATUS_OPEN_ANDOR,    OnOpenAndor)
	ON_BN_CLICKED(IDC_STATUS_OPEN_MPC200,   OnOpenMPC200)
	ON_BN_CLICKED(IDC_STATUS_OPEN_MOTOR,    OnOpenMotor)

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
	//toyscope

	scope = 2;
	
	if ( scope == 1 ) //Phil's toyscope
	{
		//set this to true if you have a iXon Plus.
		B.Andor_new = true;

		//GetDlgItem( IDC_STATUS_OPEN_PIEZO )->EnableWindow( false );
		//GetDlgItem( IDC_STATUS_OPEN_PIEZO )->ShowWindow(   false );

		//GetDlgItem( IDC_STATUS_OPEN_WHEEL )->EnableWindow( false );
		//GetDlgItem( IDC_STATUS_OPEN_WHEEL )->ShowWindow(   false );
	}
	else if ( scope == 2 )
	{
		B.Andor_new = false;

		//OnOpenAndor(); 
		//OnOpenMPC200(); 
		//OnOpenScope(); 
		//GetDlgItem( IDC_STATUS_OPEN_LASERS )->EnableWindow( false );
		//GetDlgItem( IDC_STATUS_OPEN_LASERS )->ShowWindow(   false );
	}
	else
	{
		B.Andor_new = false;
	}

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

	if( glob_m_pMotor != NULL ) 
	{
		glob_m_pMotor->DestroyWindow();
		delete glob_m_pMotor;
		glob_m_pMotor = NULL;
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
/*
void COctopusView::OnOpenScript( void ) 
{	
	GetDlgItem( IDC_STATUS_OPEN_SCRIPT )->EnableWindow( false );

	if( glob_m_pScript == NULL ) 
	{
		glob_m_pScript = new COctopusScript( this );
	} 
	else 
	{
		if( glob_m_pScript->IsWindowVisible() ) 
		{
			glob_m_pScript->DestroyWindow();
			delete glob_m_pScript;
			glob_m_pScript = NULL;
		} 
		else 
		{
			glob_m_pScript->ShowWindow( SW_RESTORE );
		}
	}
	GetDlgItem( IDC_STATUS_OPEN_SCRIPT )->EnableWindow( true );
}
*/
void COctopusView::OnOpenMotor( void ) 
{	

	GetDlgItem( IDC_STATUS_OPEN_MOTOR )->EnableWindow( false );

	if( glob_m_pMotor == NULL ) 
	{
		
		glob_m_pMotor = new COctopusMotor( this );
		
		if ( B.load_wheel_failed ) 
		{ // close it down
			glob_m_pMotor->DestroyWindow();
			delete glob_m_pMotor;
			glob_m_pMotor = NULL;
		}
	} 
	else 
	{
		if( glob_m_pMotor->IsWindowVisible() ) 
		{
			glob_m_pMotor->DestroyWindow();
			delete glob_m_pMotor;
			glob_m_pMotor = NULL;
		} 
		else 
		{
			glob_m_pMotor->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_MOTOR )->EnableWindow( true );

}
/*
void COctopusView::OnOpenLED( void ) 
{	
	
	GetDlgItem( IDC_STATUS_OPEN_LED )->EnableWindow( false );

	if( glob_m_pLED == NULL ) 
	{
		glob_m_pLED = new COctopusLED( this );
		
		if ( !B.LED_loaded ) 
		{ // close it down
			glob_m_pLED->DestroyWindow();
			delete glob_m_pLED;
			glob_m_pLED = NULL;
		}
	} 
	else 
	{
		if( glob_m_pLED->IsWindowVisible() ) 
		{
			glob_m_pLED->DestroyWindow();
			delete glob_m_pLED;
			glob_m_pLED = NULL;
		} 
		else 
		{
			glob_m_pLED->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_LED )->EnableWindow( true );
	
}


void COctopusView::OnOpenStage545( void ) 
{	

	GetDlgItem( IDC_STATUS_OPEN_STAGE )->EnableWindow( false );

	if( glob_m_pStage545 == NULL ) 
	{
		glob_m_pStage545 = new COctopusStage545( this );

		debug.Format(_T(" PI 545 stage class opened "));
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	} 
	else {
		if( glob_m_pStage545->IsWindowVisible() ) {
			glob_m_pStage545->Close();
			glob_m_pStage545->DestroyWindow();
			delete glob_m_pStage545;
			glob_m_pStage545 = NULL;
			debug.Format(_T(" PI 545 stage class closed "));
			if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
		} else {
			glob_m_pStage545->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_STAGE )->EnableWindow( true );
}


void COctopusView::OnOpenStage686( void ) 
{	

	GetDlgItem( IDC_STATUS_OPEN_STAGE )->EnableWindow( false );

	if( glob_m_pStage686 == NULL ) 
	{
		
		AfxMessageBox(_T("Warning Warning Warning\nThe stage is about to initialize.\nYou may damage the stage, objective or the piezo positioner if you have not backed off the objective.\n"));
		glob_m_pStage686 = new COctopusStage686( this );

		debug.Format(_T(" PI 686 stage class opened "));
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	} 
	else {
		if( glob_m_pStage686->IsWindowVisible() ) {
			glob_m_pStage686->Close();
			glob_m_pStage686->DestroyWindow();
			delete glob_m_pStage686;
			glob_m_pStage686 = NULL;
			debug.Format(_T(" PI 686 stage class closed "));
			if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
		} else {
			glob_m_pStage686->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_STAGE )->EnableWindow( true );
}

void COctopusView::OnOpenPiezo( void ) 
{	

	GetDlgItem( IDC_STATUS_OPEN_PIEZO )->EnableWindow( false );

	if( glob_m_pObjPiezo == NULL ) {
		glob_m_pObjPiezo = new COctopusObjectivePiezo( this );
		debug.Format(_T(" Piezo class opened "));
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	} 
	else {
		if( glob_m_pObjPiezo->IsWindowVisible() ) 
		{
			glob_m_pObjPiezo->Close();
			glob_m_pObjPiezo->DestroyWindow();
			delete glob_m_pObjPiezo;
			glob_m_pObjPiezo = NULL;
			debug.Format(_T(" Piezo class closed "));
			if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
		} else {
			glob_m_pObjPiezo->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_PIEZO )->EnableWindow( true );
}

void COctopusView::OnOpenLasers( void ) 
{	
	
	GetDlgItem( IDC_STATUS_OPEN_LASERS )->EnableWindow( false );

	if( glob_m_pLasers == NULL ) 
	{
		glob_m_pLasers = new COctopusLasers( this );
		
		if ( !B.Lasers_loaded ) { // close it down
			glob_m_pLasers->DestroyWindow();
			delete glob_m_pLasers;
			glob_m_pLasers = NULL;
		}
	} else {
		if( glob_m_pLasers->IsWindowVisible() ) {
			glob_m_pLasers->DestroyWindow();
			delete glob_m_pLasers;
			glob_m_pLasers = NULL;
		} 
		else {
			glob_m_pLasers->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_LASERS )->EnableWindow( true );
	
}

void COctopusView::OnOpenAOTF( void ) 
{	
	
	GetDlgItem( IDC_STATUS_OPEN_AOTF )->EnableWindow( false );

	if( glob_m_pAOTF == NULL ) 
	{
		glob_m_pAOTF = new COctopusAOTF( this );
		
		if ( !B.AOTF_loaded ) { // close it down
			glob_m_pAOTF->DestroyWindow();
			delete glob_m_pAOTF;
			glob_m_pAOTF = NULL;
		}
	} else {
		if( glob_m_pAOTF->IsWindowVisible() ) {
			glob_m_pAOTF->DestroyWindow();
			delete glob_m_pAOTF;
			glob_m_pAOTF = NULL;
		} 
		else {
			glob_m_pAOTF->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_AOTF )->EnableWindow( true );
	
}
*/

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

