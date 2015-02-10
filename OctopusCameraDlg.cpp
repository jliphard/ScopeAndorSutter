/**********************************************************************************
******************** COctopusCamera.cpp : implementation file
**********************************************************************************/

#include "stdafx.h"
#include "atmcd32d.h"
#include "Octopus.h"
#include "OctopusClock.h"
#include "OctopusCameraDlg.h"
#include "OctopusCameraDisplay.h"
#include "OctopusGlobals.h"
#include "OctopusLog.h"

DWORD WINAPI FocusThread(LPVOID lpParameter);

extern COctopusGlobals B;
extern COctopusLog* glob_m_pLog;
COctopusPictureDisplay* glob_m_pPictureDisplay = NULL;
extern COctopusGoodClock* glob_m_pGoodClock;

COctopusCamera::COctopusCamera(CWnd* pParent)
	: CDialog(COctopusCamera::IDD, pParent)
{    
	B.files_written			= 0;
	B.savetofile			= false;
	B.W						= 0;
	B.H						= 0;
	B.automatic_gain		= true;
	B.manual_gain			= 0.05;

	B.pics_per_file			= 400;
	B.pictures_to_skip      = 0;
	
	B.Camera_Thread_running = false;
	B.bin                   = 1;
	B.memory				= NULL;
	B.savetime				= 0;
	B.nd_setting            = 0;
	B.expt_frame            = 0;
	B.expt_time             = 0;
	B.ROI_changed           = true;
	B.Ampl_Conv             = true;
	B.SetFocus              = false;
	FrameTransfer           = true;
	TTL                     = false;
	

	//allocate lots of memory and be done with it...
	//this is way more than we should ever need.

	B.memory = new u16 [201 * 512 * 512];

	u32_Exposure_Time_Single_ms = 10;
	u32_Exposure_Time_Movie_ms  = 10;
	B.CameraExpTime_ms          = u32_Exposure_Time_Single_ms;
	u16_Gain_Mult_Factor_Single = 0;
	u16_Gain_Mult_Factor_Movie  = 0;
	m_IDC_MANUAL_GAIN           = B.manual_gain;
	m_DISPLAY_GAIN_CHOICE       = 1;
	m_DISPLAY_VSSPEED           = 4;
	m_DISPLAY_HSSPEED           = 0;
	m_DISPLAY_BIN_CHOICE        = B.bin - 1;

	m_IDC_PICTURES_PER_FILE     = B.pics_per_file;
	m_IDC_PICTURES_SKIP         = B.pictures_to_skip;

	m_CCD_target_temp		    = 10;
	m_CCD_current_temp          = 0;
	
	VERIFY(m_bmp_no.LoadBitmap(IDB_NO));
	VERIFY(m_bmp_yes.LoadBitmap(IDB_YES));
	
    CString currentDir;
    GetCurrentDirectory( MAX_PATH, currentDir.GetBufferSetLength(MAX_PATH) );
    currentDir.ReleaseBuffer();
	currentDir.Append(_T("\\fallback"));

	pathname_display = currentDir;
	B.pathname       = pathname_display;

	if( Create(COctopusCamera::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera(CWnd* pParent) "));

    if ( B.Andor_new )
		em_limit = 950; //we are dealing with a new camera
	else
		em_limit = 255; //we are dealing with an old camera
}

void COctopusCamera::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    
	DDX_Control( pDX, IDC_CAS_TEMP_CURRENT, m_Temperature_Now);

	DDX_Control( pDX, IDC_CAS_PICTURES_SKIP_IND, m_Skip_Now);

    DDX_Text( pDX, IDC_CAS_EXPOSURE_TIME_SINGLE, u32_Exposure_Time_Single_ms);
	DDV_MinMaxLong(pDX, u32_Exposure_Time_Single_ms, 1, 10000);

	DDX_Text( pDX, IDC_CAS_EXPOSURE_TIME_MOVIE, u32_Exposure_Time_Movie_ms);
	DDV_MinMaxLong(pDX, u32_Exposure_Time_Movie_ms, 1, 10000);

	DDX_Text( pDX, IDC_CAS_GAIN_SINGLE, u16_Gain_Mult_Factor_Single);

	if ( B.Andor_new )
		DDV_MinMaxInt(pDX, u16_Gain_Mult_Factor_Single, 0, 950);
	else
		DDV_MinMaxInt(pDX, u16_Gain_Mult_Factor_Single, 0, 255);

	DDX_Text( pDX, IDC_CAS_GAIN_MOVIE, u16_Gain_Mult_Factor_Movie);

	if ( B.Andor_new )
		DDV_MinMaxInt(pDX, u16_Gain_Mult_Factor_Movie, 0, 950);
	else
		DDV_MinMaxInt(pDX, u16_Gain_Mult_Factor_Movie, 0, 255);

	DDX_Text( pDX, IDC_CAS_MANUAL_GAIN,	m_IDC_MANUAL_GAIN);
    DDV_MinMaxDouble(pDX, m_IDC_MANUAL_GAIN, 0.0001, 10.0);
	
	DDX_Text( pDX, IDC_CAS_PICTURES_PER_FILE, m_IDC_PICTURES_PER_FILE);
	DDV_MinMaxInt(pDX, m_IDC_PICTURES_PER_FILE, 1, 10000);

	DDX_Text( pDX, IDC_CAS_PICTURES_SKIP, m_IDC_PICTURES_SKIP);
	DDV_MinMaxInt(pDX, m_IDC_PICTURES_SKIP, 0, 10000);
    
	DDX_Text( pDX, IDC_CAS_TEMP_TARGET, m_CCD_target_temp);
	DDV_MinMaxInt(pDX, m_CCD_target_temp, -80, 25);

	DDX_Radio( pDX, IDC_CAS_DISPLAY_GAIN_MANUAL, m_DISPLAY_GAIN_CHOICE);

	DDX_Radio( pDX, IDC_CAS_DISPLAY_BIN_1, m_DISPLAY_BIN_CHOICE);

	DDX_Radio( pDX, IDC_CAS_VSSPEED_0, m_DISPLAY_VSSPEED);
	
	DDX_Radio( pDX, IDC_CAS_HSSPEED_0, m_DISPLAY_HSSPEED);

	DDX_Text(pDX, IDC_CAS_PATHNAME,   pathname_display);

	DDX_Control(pDX, IDC_CAS_SAVEONOFF_BMP,  m_status_save);

	DDX_Control(pDX, IDC_CAS_FT,  m_ctlFTCheckBox);
	DDX_Control(pDX, IDC_CAS_TTL, m_ctlTTLCheckBox);
	
}  

BEGIN_MESSAGE_MAP(COctopusCamera, CDialog)
	ON_BN_CLICKED(IDC_CAS_STARTSTOP_PICTURE,  TakePicture)
	ON_BN_CLICKED(IDC_CAS_START_MOVIE,	      StartMovie)
	ON_BN_CLICKED(IDC_CAS_STOP_MOVIE,	      StopCameraThread)

	ON_BN_CLICKED(IDC_CAS_SETPATH,			  OnSetPath)

	ON_EN_KILLFOCUS(IDC_CAS_EXPOSURE_TIME_SINGLE,   OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_CAS_EXPOSURE_TIME_MOVIE,    OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_CAS_GAIN_SINGLE,			OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_CAS_GAIN_MOVIE,				OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_CAS_TEMP_TARGET,			OnKillfocusTempTarget)
	ON_EN_KILLFOCUS(IDC_CAS_MANUAL_GAIN,			OnKillfocusManualGain)
	
	ON_EN_KILLFOCUS(IDC_CAS_PICTURES_PER_FILE,		OnKillfocusPicturesPerFile)
	ON_EN_KILLFOCUS(IDC_CAS_PICTURES_SKIP,		    OnKillfocusPicturesSkip)
	
	ON_BN_CLICKED(IDC_CAS_DISPLAY_GAIN_MANUAL,		OnDisplayGainManual)
	ON_BN_CLICKED(IDC_CAS_DISPLAY_GAIN_AUTOMATIC,	OnDisplayGainAutomatic)

	ON_BN_CLICKED(IDC_CAS_DISPLAY_BIN_1,			OnBinning1x1)
	ON_BN_CLICKED(IDC_CAS_DISPLAY_BIN_2,			OnBinning2x2)
	ON_BN_CLICKED(IDC_CAS_DISPLAY_BIN_4,			OnBinning4x4)
	ON_BN_CLICKED(IDC_CAS_DISPLAY_BIN_8,			OnBinning8x8)

	ON_BN_CLICKED(IDC_CAS_VSSPEED_0, OnVSSPEED_0)
	ON_BN_CLICKED(IDC_CAS_VSSPEED_1, OnVSSPEED_1)
	ON_BN_CLICKED(IDC_CAS_VSSPEED_2, OnVSSPEED_2)
	ON_BN_CLICKED(IDC_CAS_VSSPEED_3, OnVSSPEED_3)
	ON_BN_CLICKED(IDC_CAS_VSSPEED_4, OnVSSPEED_4)

	ON_BN_CLICKED(IDC_CAS_HSSPEED_0, OnHSSPEED_0)
	ON_BN_CLICKED(IDC_CAS_HSSPEED_1, OnHSSPEED_1)
	ON_BN_CLICKED(IDC_CAS_HSSPEED_2, OnHSSPEED_2)
	ON_BN_CLICKED(IDC_CAS_HSSPEED_3, OnHSSPEED_3)
	ON_BN_CLICKED(IDC_CAS_HSSPEED_4, OnHSSPEED_4)
	ON_BN_CLICKED(IDC_CAS_HSSPEED_5, OnHSSPEED_5)
	
	ON_BN_CLICKED(IDC_CAS_SAVEONOFF,  OnFileChange)
	ON_BN_CLICKED(IDC_CAS_FOCUSONOFF, OnFocusChange)

	ON_BN_CLICKED(IDC_CAS_FT,  OnBnClickedCasFt)
	ON_BN_CLICKED(IDC_CAS_TTL, OnBnClickedCasTTL)
	ON_WM_TIMER()

END_MESSAGE_MAP()

BOOL COctopusCamera::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if ( FrameTransfer )
		CheckDlgButton(IDC_CAS_FT, BST_CHECKED);
	else
		CheckDlgButton(IDC_CAS_FT, BST_UNCHECKED);

	if ( TTL )
		CheckDlgButton(IDC_CAS_TTL, BST_CHECKED);
	else
		CheckDlgButton(IDC_CAS_TTL, BST_UNCHECKED);
	
	char aBuffer[256];

	GetCurrentDirectory(256, (LPSTR)aBuffer); // Look in current working directory
	                                           // for driver files

	if( andor::Initialize(aBuffer) != DRV_SUCCESS ) 
		AfxMessageBox(_T("Failure Opening Andor Camera"));
	
	Opencam(); 

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::OnInitDialog() "));

	return TRUE;
}

COctopusCamera::~COctopusCamera() 
{
	if( B.Camera_Thread_running ) 
		StopCameraThread();

	if(CoolerOFF() != DRV_SUCCESS ) 
		AfxMessageBox(_T("Failure turning Cooler Off"));

	do{GetTemperature(&m_CCD_current_temp); } while ( m_CCD_current_temp < 0 );
	
	if ( B.memory != NULL ) 
	{
		delete [] B.memory;
		B.memory = NULL;
	}

    ShutDown();

	if( glob_m_pPictureDisplay != NULL ) {
		glob_m_pPictureDisplay->DestroyWindow();
		delete glob_m_pPictureDisplay;
		glob_m_pPictureDisplay = NULL;
	}

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" ~COctopusCamera() "));
}

/*********************************************************************************
* Open the Camera
*********************************************************************************/

void COctopusCamera::Opencam() 
{
	SetTemperature( m_CCD_target_temp );
	
	if( CoolerON()!= DRV_SUCCESS ) 
		AfxMessageBox(_T("Failure turning Cooler On"));
	
	int xpixels;
	int ypixels;

	GetDetector( &xpixels, &ypixels );
	
	B.CCD_x_phys_e = xpixels;
	B.CCD_y_phys_e = ypixels;

	B.ROI_target.x1 = 1;
	B.ROI_target.y1 = 1;
	B.ROI_target.x2 = B.CCD_x_phys_e;
	B.ROI_target.y2 = B.CCD_y_phys_e;

	SetROI_To_Default();

	//keep track of temperature
	SetTimer( TIMER_TEMP, 2000, NULL ); 

	//allow access to gains > 300
	if ( B.Andor_new )
		SetEMAdvanced(1); 
	
	SetEMGainMode(3);

	//andor::SetShutter( 0, 0, 0, 0 ); // Full auto - this works for the old camera
	SetShutter( 1, 1, 0, 0 ); // Always open - this works for the old camera
	
	OnVSSPEED_4();

	OnHSSPEED_1(); //5MHz 14 bit	

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::OpenCam() "));

}

/*********************************************************************************
*********************************************************************************/

void COctopusCamera::TakePicture() 
{
	if( B.Camera_Thread_running ) 
		return;

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::TakePicture() "));

	SetReadMode(4);             //set read out mode to image
	SetAcquisitionMode(1);      //set mode for single scan
	SetExposureTime( float(u32_Exposure_Time_Single_ms) * 0.001 );
	
	B.CameraExpTime_ms = u32_Exposure_Time_Single_ms;

	if ( B.Ampl_Conv )
		SetEMCCDGain( 0 );
	else
        SetEMCCDGain( int(u16_Gain_Mult_Factor_Single) );
    	
	if ( B.ROI_changed ) 
		SetROI();

	SetImage( B.ROI_actual.bin, B.ROI_actual.bin, \
		      B.ROI_actual.x1,  B.ROI_actual.x2,  \
			  B.ROI_actual.y1,  B.ROI_actual.y2 );
	
	SetShutter( 1, 1, 0, 0 ); // Always open - this works for the old camera

	if ( glob_m_pPictureDisplay == NULL || B.ROI_changed ) 
		OpenWindow();

	StartAcquisition();
	
	int status = 0;
	do{ GetStatus( &status ); } while ( status != DRV_IDLE );
    
	GetAcquiredData16( B.memory, B.H * B.W );

	if ( glob_m_pPictureDisplay != NULL ) 
		glob_m_pPictureDisplay->Update_Bitmap( B.memory, 1 );
}

/*********************************************************************************
* Start/stop continuous data aquisition
*********************************************************************************/

void COctopusCamera::StartMovie( void )
{	
	
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::StartMovie( void ) "));
	
	if( B.Camera_Thread_running ) 
		return;

	DisableDlgMovie();

	SetAcquisitionMode( 5 );		// set to run till abort
	SetReadMode( 4 );				// image mode
	SetExposureTime( float(u32_Exposure_Time_Movie_ms) * 0.001 );
	
	B.CameraExpTime_ms = u32_Exposure_Time_Movie_ms;

	SetKineticCycleTime( 0 );
	
	if ( B.Ampl_Conv )
		SetEMCCDGain( 0 );
	else
       SetEMCCDGain( int(u16_Gain_Mult_Factor_Movie) );

	if( TTL )
	{	
		SetTriggerMode( 1 );		// fast external trigger
		SetFastExtTrigger( 1 ); 
	}
	else
		SetTriggerMode( 0 );		// internal trigger

	if ( FrameTransfer )
		SetFrameTransferMode( 1 );	// FT mode
	else
		SetFrameTransferMode( 0 );	// non FT mode

	if ( B.ROI_changed ) 
		SetROI();
	
	SetImage( B.ROI_actual.bin, B.ROI_actual.bin, \
		      B.ROI_actual.x1,  B.ROI_actual.x2,  \
			  B.ROI_actual.y1,  B.ROI_actual.y2 );

	SetShutter( 1, 1, 0, 0 ); // Always open - this works for the old camera

	if ( glob_m_pPictureDisplay == NULL || B.ROI_changed ) 
		OpenWindow();

	StartAcquisition();
	StartWaitThread();
}

void COctopusCamera::StopCameraThread() 
{
	KillWaitThread();
	AbortAcquisition();

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::StopCameraThread() "));
}

/*********************************************************************************
********************	Utility functions ****************************************
*********************************************************************************/

void COctopusCamera::SetROI_To_Default( void )  // set ROI to default
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::SetROI_To_Default( void ) "));

	B.ROI_target.bin = 1;
	B.ROI_target.x1  = 1;
	B.ROI_target.y1  = 1;
	B.ROI_target.x2  = B.CCD_x_phys_e;
	B.ROI_target.y2  = B.CCD_y_phys_e;

	SetROI();
}

void COctopusCamera::SetROI( void ) 
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::SetROI( void ) "));

	// ROInew setting quality control
	if ((B.ROI_target.x2 > B.CCD_x_phys_e ) || (B.ROI_target.x2 <= B.ROI_target.x1 )) B.ROI_target.x2 = B.CCD_x_phys_e;
	if ((B.ROI_target.y2 > B.CCD_y_phys_e ) || (B.ROI_target.y2 <= B.ROI_target.y1 )) B.ROI_target.y2 = B.CCD_y_phys_e;
	if ((B.ROI_target.x1 > B.ROI_target.x2) || (B.ROI_target.x1 < 1)) B.ROI_target.x1 = 1;
	if ((B.ROI_target.y1 > B.ROI_target.y2) || (B.ROI_target.y1 < 1)) B.ROI_target.y1 = 1;

	//write the new
	B.ROI_actual.x1 = B.ROI_target.x1;	// serial start
	B.ROI_actual.x2 = B.ROI_target.x2;	// serial end
	B.ROI_actual.y1 = B.ROI_target.y1;	// parallel start
	B.ROI_actual.y2 = B.ROI_target.y2;	// parallel end

	BinChange();
}

void COctopusCamera::BinChange( void )
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::BinChange( void ) "));

	B.ROI_actual.bin = B.bin;
	
	while (((B.ROI_actual.x2 - B.ROI_actual.x1 + 1) % B.ROI_actual.bin) != 0) B.ROI_actual.x2--;
	while (((B.ROI_actual.y2 - B.ROI_actual.y1 + 1) % B.ROI_actual.bin) != 0) B.ROI_actual.y2--;
	
	B.W = (B.ROI_actual.x2 - B.ROI_actual.x1 + 1) / B.ROI_actual.bin;
	B.H = (B.ROI_actual.y2 - B.ROI_actual.y1 + 1) / B.ROI_actual.bin;

	//should be true anyway, but just in case.....
	B.ROI_changed = true;
}

void COctopusCamera::OpenWindow( void ) 
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::OpenWindow( void ) "));

	if( glob_m_pPictureDisplay != NULL ) 
	{
		glob_m_pPictureDisplay->DestroyWindow();
		delete glob_m_pPictureDisplay;
		glob_m_pPictureDisplay = NULL;
		if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" glob_m_pPictureDisplay != NULL "));
	}
	if( glob_m_pPictureDisplay == NULL ) 
	{
		B.ROI_changed = false;
		glob_m_pPictureDisplay = new COctopusPictureDisplay( this, B.W, B.H );
		glob_m_pPictureDisplay->Create( IDC_CAS_DISP );
		glob_m_pPictureDisplay->ShowWindow( SW_SHOW );
		glob_m_pPictureDisplay->Create_Bitmap(); 
		if ( glob_m_pLog != NULL ) 
			glob_m_pLog->Write(_T(" glob_m_pPictureDisplay == NULL "));
	}
}

void COctopusCamera::OnKillfocusGeneral() { UpdateData( true ); }

void COctopusCamera::OnKillfocusTempTarget() {
	UpdateData( true );
	SetTemperature ( m_CCD_target_temp );
}

void COctopusCamera::OnKillfocusManualGain() {
	UpdateData( true );
	B.manual_gain = m_IDC_MANUAL_GAIN;
}

void COctopusCamera::OnKillfocusPicturesPerFile() {
	UpdateData( true );
	B.pics_per_file = m_IDC_PICTURES_PER_FILE;
}

void COctopusCamera::OnKillfocusPicturesSkip() {
	UpdateData( true );
	B.pictures_to_skip = m_IDC_PICTURES_SKIP;
	CString str;
	str.Format(_T("Skip: %d"), B.pictures_to_skip);
	if( IsWindowVisible() ) m_Skip_Now.SetWindowText( str );
}

void COctopusCamera::OnBinning1x1() { m_DISPLAY_BIN_CHOICE = 0; B.bin = 1; BinChange(); }
void COctopusCamera::OnBinning2x2() { m_DISPLAY_BIN_CHOICE = 1; B.bin = 2; BinChange(); }
void COctopusCamera::OnBinning4x4() { m_DISPLAY_BIN_CHOICE = 2; B.bin = 4; BinChange(); }
void COctopusCamera::OnBinning8x8() { m_DISPLAY_BIN_CHOICE = 3; B.bin = 8; BinChange(); }

void COctopusCamera::OnVSSPEED_0() { m_DISPLAY_VSSPEED = 0; OnVSSPEED_Report(); }
void COctopusCamera::OnVSSPEED_1() { m_DISPLAY_VSSPEED = 1; OnVSSPEED_Report(); }
void COctopusCamera::OnVSSPEED_2() { m_DISPLAY_VSSPEED = 2; OnVSSPEED_Report(); }
void COctopusCamera::OnVSSPEED_3() { m_DISPLAY_VSSPEED = 3; OnVSSPEED_Report(); }
void COctopusCamera::OnVSSPEED_4() { m_DISPLAY_VSSPEED = 4; OnVSSPEED_Report(); }

void COctopusCamera::OnHSSPEED_0() { m_DISPLAY_HSSPEED = 0; OnHSSPEED_Report(); }
void COctopusCamera::OnHSSPEED_1() { m_DISPLAY_HSSPEED = 1; OnHSSPEED_Report(); }
void COctopusCamera::OnHSSPEED_2() { m_DISPLAY_HSSPEED = 2; OnHSSPEED_Report(); }
void COctopusCamera::OnHSSPEED_3() { m_DISPLAY_HSSPEED = 3; OnHSSPEED_Report(); }
void COctopusCamera::OnHSSPEED_4() { m_DISPLAY_HSSPEED = 4; OnHSSPEED_Report(); }
void COctopusCamera::OnHSSPEED_5() { m_DISPLAY_HSSPEED = 5; OnHSSPEED_Report(); }

void COctopusCamera::OnVSSPEED_Report()
{
	SetVSSpeed( m_DISPLAY_VSSPEED ); 
}

void COctopusCamera::OnHSSPEED_Report()
{
	if (        m_DISPLAY_HSSPEED == 0 )
	{
		SetOutputAmplifier( 0 );  // EMCCD
		SetADChannel( 0 );		 // 14 bit
		SetHSSpeed( 0, 0 );       // 10Mhz
		SetPreAmpGain( 2 );		 // 5x
	} 
	else if ( m_DISPLAY_HSSPEED == 1 )
	{
		SetOutputAmplifier( 0 );  // EMCCD
		SetADChannel( 0 );		 // 14 bit
		SetHSSpeed( 0, 1 );       // 5 Mhz
		SetPreAmpGain( 2 );		 // 5x
	} 
	else if ( m_DISPLAY_HSSPEED == 2 )
	{
		SetOutputAmplifier( 0 );  // EMCCD
		SetADChannel( 0 );		 // 14 bit
		SetHSSpeed( 0, 2 );       // 3 Mhz
		SetPreAmpGain( 2 );		 // 5x
	} 
	else if ( m_DISPLAY_HSSPEED == 3 )
	{
		SetOutputAmplifier( 0 );  // EMCCD
		SetADChannel( 1 );		 // 16 bit
		SetHSSpeed( 0, 3 );       // 1MHz
		SetPreAmpGain( 2 );		 // 5x
	} 
	else if ( m_DISPLAY_HSSPEED == 4 )
	{
		SetOutputAmplifier( 1 ); // Conv
		SetADChannel( 1 );       // 16 bit
		SetHSSpeed( 1, 1 );      // 1 Mhz
		SetPreAmpGain( 0 );		// 1.0
	} 
	else if ( m_DISPLAY_HSSPEED == 5 )
	{
		SetOutputAmplifier( 1 ); // Conv 
		SetADChannel( 0 );       // 14 bit
		SetHSSpeed( 1, 0 );      // 3 Mhz
		SetPreAmpGain( 0 );		// 1.0
	}

	B.Ampl_Setting = m_DISPLAY_HSSPEED;

	if ( B.Ampl_Setting > 3 )
		B.Ampl_Conv = true;
	else
		B.Ampl_Conv = false;

	UpdateData( false );

}

void COctopusCamera::OnDisplayGainManual() 
{ 
	m_DISPLAY_GAIN_CHOICE = 0;
	UpdateData( false );
	if( m_DISPLAY_GAIN_CHOICE == 0)
		B.automatic_gain = false;
	else
		B.automatic_gain = true;
}

void COctopusCamera::OnDisplayGainAutomatic() 
{ 
	m_DISPLAY_GAIN_CHOICE = 1;
	UpdateData( false );
	if( m_DISPLAY_GAIN_CHOICE == 0)
		B.automatic_gain = false;
	else
		B.automatic_gain = true;
}

void COctopusCamera::OnFileChange( void ) 
{
	if ( !B.savetofile ) 
		StartSaving();
	else 
		StopSaving();
}

void COctopusCamera::OnFocusChange( void ) 
{
	if ( B.Camera_Thread_running == false ) 
		B.SetFocus = true;
}

void COctopusCamera::StartSaving() 
{

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::StartSaving() "));

	if ( !B.savetofile ) 
	{	
		B.savetofile = true;
		m_status_save.SetBitmap( m_bmp_yes );
		if ( glob_m_pGoodClock == NULL ) return;
		B.savetime = (double)glob_m_pGoodClock->End();
		B.expt_frame = 0; //counter for saved frames
	}
}

void COctopusCamera::StopSaving()
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::StopSaving() "));

	if ( B.savetofile ) 
	{ 
		B.savetofile = false;
		m_status_save.SetBitmap( m_bmp_no );
	    if (glob_m_pPictureDisplay != NULL) 
			glob_m_pPictureDisplay->Close_The_File();
	}
}

void COctopusCamera::OnSetPath( void ) 
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::OnSetPath( void ) "));

	CFileDialog FileDlg( FALSE, _T("dth"), NULL, OFN_HIDEREADONLY, _T("Octopus Header (*.dth)|*.dth|"), this);
    
	CString pathname;
	
	if( FileDlg.DoModal() == IDOK ) 
	{
		pathname   = FileDlg.GetPathName();
		pathname   = pathname.Left(pathname.Find(_T(".dth")));
		pathname_display = pathname;
		B.pathname       = pathname;
		UpdateData( false );
	}
}

void COctopusCamera::OnTimer( UINT_PTR nIDEvent ) 
{

	if( nIDEvent == TIMER_TEMP ) 
	{
		int status = 0;

		GetStatus( &status );
		
		if( status == DRV_IDLE ) 
		{
			GetTemperature( &m_CCD_current_temp );
			CString str;
			str.Format(_T("Current T: %d C"), m_CCD_current_temp);
			if( IsWindowVisible() ) m_Temperature_Now.SetWindowText( str );
		}
	}

	CDialog::OnTimer(nIDEvent);
}

/************************************
 Read info from camera
 ***********************************/

void COctopusCamera::StartWaitThread( void )
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::StartWaitThread( void ) "));
	
	DWORD WaitThreadID;
	
	B.Camera_Thread_running = true;
	
	WaitThread = CreateThread( 0, 4096, FocusThread, NULL, 0, &WaitThreadID );
}

void COctopusCamera::KillWaitThread( void )
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::KillWaitThread( void ) "));

	if( !B.Camera_Thread_running ) return;

	B.Camera_Thread_running = false;

	WaitForSingleObject( WaitThread, 500 );
	
	Sleep( 500 );

	EnableDlg();
	
	if ( !CloseHandle( WaitThread ) ) 
		AfxMessageBox(_T("Error: could not close camera thread."));
}

DWORD WINAPI FocusThread(LPVOID lpParameter) 
{
    Sleep(30); //this thread should run at around 30 Hz

	long first      = 0;
	long last       = 0;
	long validfirst = 0;
	long validlast  = 0;
	long retval     = 0;

	u16 NumberOfNewImages = 0;

	while( B.Camera_Thread_running ) 
	{		
		retval = GetNumberNewImages( &first, &last );
		
		// GetNumberNewImages is a wierd function 
		// when there is one image to transfer, 
		// last == first, and thus number of images = 0;
		// likewise, transfer two images = ... 1, 1+1 and so forth

		NumberOfNewImages = last - first + 1;

		if ( retval == DRV_NO_NEW_DATA )
		{
			//do nothing
		} 
		else if ( retval == DRV_SUCCESS && NumberOfNewImages >= 1 )
		{
			//let's get cracking...

			retval = GetImages16( first, last,        \
					B.memory, B.W * B.H * NumberOfNewImages, \
					&validfirst, &validlast);

			if ( retval == DRV_SUCCESS )
			{
				if ( glob_m_pPictureDisplay != NULL )
					glob_m_pPictureDisplay->Update_Bitmap( B.memory, NumberOfNewImages );
			} 
		}
	}
	return 0;	
}

/************************************
 User interface etc
 ***********************************/

void COctopusCamera::OnBnClickedCasFt()
{
    int l_ChkBox = m_ctlFTCheckBox.GetCheck();
	
	if (l_ChkBox == 0) 
		FrameTransfer = false;
	else if (l_ChkBox == 1) 
		FrameTransfer = true;
}

void COctopusCamera::OnBnClickedCasTTL()
{
    int l_ChkBox = m_ctlTTLCheckBox.GetCheck();
	
	if (l_ChkBox == 0) 
		TTL = false;
	else if (l_ChkBox == 1) 
		TTL = true;
}

BOOL COctopusCamera::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int id    = LOWORD(wParam); // Notification code
	if( id == 2 ) return FALSE; // Trap ESC key
	if( id == 1 ) return FALSE; // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}

void COctopusCamera::EnableDlg( void ) 
{
	GetDlgItem( IDC_CAS_STARTSTOP_PICTURE		)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_START_MOVIE				)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_STOP_MOVIE				)->EnableWindow( false );
	GetDlgItem( IDC_CAS_EXPOSURE_TIME_SINGLE	)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_EXPOSURE_TIME_MOVIE		)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_PICTURES_PER_FILE		)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_TEMP_TARGET				)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_GAIN_SINGLE				)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_GAIN_MOVIE				)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_1			)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_2			)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_4			)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_8			)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_SETPATH					)->EnableWindow( true  );
}

void COctopusCamera::DisableDlgMovie( void ) 
{
	GetDlgItem( IDC_CAS_STARTSTOP_PICTURE	)->EnableWindow( false );
	GetDlgItem( IDC_CAS_STOP_MOVIE			)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_START_MOVIE			)->EnableWindow( false );
	DisableDlg();
}

void COctopusCamera::DisableDlg( void ) 
{
	GetDlgItem( IDC_CAS_EXPOSURE_TIME_SINGLE	)->EnableWindow( false );
	GetDlgItem( IDC_CAS_EXPOSURE_TIME_MOVIE		)->EnableWindow( false );
	GetDlgItem( IDC_CAS_PICTURES_PER_FILE		)->EnableWindow( false );
	GetDlgItem( IDC_CAS_TEMP_TARGET				)->EnableWindow( false );
	GetDlgItem( IDC_CAS_GAIN_SINGLE				)->EnableWindow( false );
	GetDlgItem( IDC_CAS_GAIN_MOVIE				)->EnableWindow( false );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_1			)->EnableWindow( false );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_2			)->EnableWindow( false );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_4			)->EnableWindow( false );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_8			)->EnableWindow( false );
	GetDlgItem( IDC_CAS_SETPATH					)->EnableWindow( false );
}
