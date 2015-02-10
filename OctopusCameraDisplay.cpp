#include "stdafx.h"
#include "Octopus.h"
#include "OctopusCameraDisplay.h"
#include "OctopusGlobals.h"
#include "OctopusCameraDlg.h"
#include "OctopusClock.h"

#define COL_BLUE 0x00FF0000
#define COL_RED  0x000000FF

extern COctopusGlobals	  B;
extern COctopusCamera*    glob_m_pCamera;
extern COctopusGoodClock* glob_m_pGoodClock;

COctopusPictureDisplay::COctopusPictureDisplay(CWnd* pParent, u16 x, u16 y)
	: CDialog(COctopusPictureDisplay::IDD, pParent) 
{
	Width			 = x; 
	Height			 = y;

	LengthOneFrame   = x * y;
	FullFrame        = B.CCD_x_phys_e * B.CCD_y_phys_e;

	g_mp		     = 0.05;
	g_max		     = 0;
	g_min		     = 65535;
	g_mean           = 0.0;
	
	time_dx			 = 0.0;
	time_old		 = 0.0;
	camera_freq      = 1.0;
	display_freq     = 1.0;

	pictures_skipped = 0;
	pictures_written = 0;

	pPic_main        = NULL;

	MemoryFramesOld  = 1;
	g_NumFT          = 1;

	//allocate some memory
	first_picture		= new u16[ LengthOneFrame ];
	first_picture_flip	= new u16[ LengthOneFrame ];
	
	//may need to reallocate for very fast runs
	frames_to_save   = new u16[ LengthOneFrame * MemoryFramesOld ];

	pFileData        = NULL;
	pFileHead		 = NULL;

	Marking          = false;

	c_picture_size   = 700;
	zoom             = 1.0;	
	control_height   = 50;
	save_halflength  = 60;

	//focus
	B.focus_score    = 0.0;
	B.focus_beadX    = 0.0;
	B.focus_beadY    = 0.0;
	B.Focus_ROI_Set  = false;

	pic_x1 = 0;
	pic_y1 = 0;
	pic_x2 = 0;
	pic_y2 = 0;
	
}

BOOL COctopusPictureDisplay::OnInitDialog() 
{
	CDialog::OnInitDialog();
	return TRUE;
}

BEGIN_MESSAGE_MAP(COctopusPictureDisplay, CDialog)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

void COctopusPictureDisplay::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TIME, m_info);
}

COctopusPictureDisplay::~COctopusPictureDisplay() 
{
	Bitmap_main.DeleteObject();

	delete [] first_picture;		first_picture			= NULL;
	delete [] frames_to_save;		frames_to_save			= NULL;
	delete [] first_picture_flip;	first_picture_flip		= NULL;
	delete [] pPic_main;			pPic_main				= NULL;		   

	if ( pFileData != NULL ) 
	{
		fclose( pFileData );
		pFileData = NULL;
	}
	if ( pFileHead != NULL ) 
	{
		fclose( pFileHead );
		pFileHead = NULL;
	} 
}

void COctopusPictureDisplay::Create_Bitmap() 
{
	MoveWindow( 980, 8, c_picture_size + 13, c_picture_size + control_height + 33 );
	
	if ( Height > Width )
		zoom = (double)c_picture_size / (double)Height;
	else 
		zoom = (double)c_picture_size / (double)Width;
 
	screen_wp = (u16)( (double)Width  * zoom );
	screen_hp = (u16)( (double)Height * zoom );
	
	//bitmap placement control
	pic_x1 = 3;
	pic_y1 = 3 + control_height;
	pic_x2 = pic_x1 + screen_wp;
	pic_y2 = pic_y1 + screen_hp;

	CClientDC aDC( this );
	CDC* pDC = &aDC;
	
	//main bitmap
	BITMAP bm;
	Bitmap_main.CreateCompatibleBitmap( pDC, Width, Height );	
	Bitmap_main.GetObject( sizeof(BITMAP), &bm );
	pPic_main = new uC [ bm.bmHeight * bm.bmWidthBytes ];

}

void COctopusPictureDisplay::Draw_Bitmap( void ) 
{
	CClientDC aDC( this );
	CDC* pDC = &aDC;
	CDC hMemDC;
	hMemDC.CreateCompatibleDC( pDC );

	BITMAP bm;
	Bitmap_main.GetBitmap( &bm );
	hMemDC.SelectObject( &Bitmap_main );
	hMemDC.SetMapMode( MM_ANISOTROPIC );
	hMemDC.SetViewportOrg(bm.bmWidth - 1, 0);
	hMemDC.SetViewportExt(-1, 1);

    pDC->StretchBlt( pic_x1, pic_y1, \
		screen_wp, screen_hp, \
		&hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY );

}

void COctopusPictureDisplay::Update_Bitmap( u16 *pict, u16 FramesTransferred ) 
{

	u16 temp   = 0;
	u32 c	   = 0;
	u32 b      = 0;
	uC  val    = 0;
	u32 x      = 0;
	u32 y      = 0;
	
	g_mean     = 0.0;
	g_min      = 65535;
	g_max      = 0;
    g_NumFT    = FramesTransferred;

	//this section is just for saving data to file. 
	if ( g_NumFT == MemoryFramesOld )
	{
		//all is well - stick with old memory
	} 
	else
	{
		//yipes need to change memory size
		delete [] frames_to_save;     
		frames_to_save  = NULL;
		frames_to_save  = new u16[ LengthOneFrame * FramesTransferred ];
		MemoryFramesOld = g_NumFT;
	}

	//there may be more frames in this vector, 
	//but let's only look at the first one. 
	for ( c = 0; c < LengthOneFrame; c++ ) 
	{
		temp = *( pict + c );
		*( first_picture + c ) = temp;
		if ( temp < g_min ) 
			g_min = temp; 
		else if ( temp > g_max ) 
			g_max = temp;	
		g_mean = g_mean + temp;
	}

	g_mean = g_mean / LengthOneFrame;

	// these are the data to be saved - there may be multiple 
	// frames in one vector, hence c < LengthOneFrame * g_NumFT

	for ( c = 0; c < LengthOneFrame * g_NumFT; c++ ) 
	{
		*( frames_to_save + c ) = *( pict + c );
	}

    //if CONV amplifier, need to flip image...	
	if ( B.Ampl_Conv ) 
	{
		u32 xf = 0;

		for ( y = 0; y < Height; y++ ) 
		{
			for ( x = 0; x < Width; x++ ) 
			{
				xf = Width - 1 - x;
				*( first_picture_flip + ( y * Width ) + xf ) = \
				*( first_picture      + ( y * Width ) + x  );
			}
		}
	} 

	u16 *good_pic = first_picture;

	if( B.Ampl_Conv )
		good_pic = first_picture_flip;

	if ( B.automatic_gain ) 
	{
		g_mp = 255.0 / double( g_max - g_min ); 
	} 
	else 
	{
		g_mp = B.manual_gain;
	}

	//just show the first frame
	for ( c=0, b=0; c < LengthOneFrame; c++ ) 
	{	
		val = (uC)((double)( *( good_pic + c ) - g_min ) * g_mp);
		*(pPic_main + b++) = val;
		*(pPic_main + b++) = val;
		*(pPic_main + b++) = val;
		*(pPic_main + b++) = val;
	}

	B.time_now = (double)glob_m_pGoodClock->End();
	time_dx    = B.time_now - time_old;
	time_old   = B.time_now;

	if( time_dx > 0 ) 
	{
		camera_freq  = double(g_NumFT) / time_dx;
	    display_freq = 1.0             / time_dx;
	}

	BITMAP bm;
	Bitmap_main.GetBitmap( &bm );	
	Bitmap_main.SetBitmapBits( bm.bmWidthBytes * bm.bmHeight, pPic_main );

	Draw_Bitmap();

	UpdateTitle();

	pictures_skipped++;

	if( B.savetofile && (pictures_skipped > B.pictures_to_skip) ) 
	{
		WritePic();
		pictures_skipped = 0;
	}
}      

bool COctopusPictureDisplay::Open_A_File( void ) 
{
	CString temp;
	B.files_written++;

	temp.Format(_T("%ld.dat"), B.files_written );
	filename = B.pathname + temp;	
	pFileData = fopen ( filename , _T("wb"));
	
	temp.Format(_T("%ld.dth"), B.files_written );
	filename = B.pathname + temp;
	pFileHead = fopen ( filename , _T("wt"));

	//pFileHead = fopen( filename , _T("wt"));//_wfopen( filename , _T("wt"));
	if ( pFileData != NULL && pFileHead != NULL ) 
		return true;
	else 
		return false;
}

void COctopusPictureDisplay::Close_The_File( void )
{
	if( pFileHead == NULL || pFileData == NULL ) return;

	pictures_written = 0;

	fclose( pFileData );
	pFileData = NULL;

	fclose( pFileHead );
	pFileHead = NULL;
}

void COctopusPictureDisplay::WritePic( void ) 
{

	if( pictures_written >= B.pics_per_file ) 
	{
		Close_The_File();
		pictures_written = 0;
	}

	// if the file is closed - due to any number of reasons 
	// => open a new one
	
	if( pFileHead == NULL || pFileData == NULL ) 
	{ 
		if ( !Open_A_File() ) return; 
	}

	u16 h = Height;
	u16 w = Width;
	
	CString str;

	for ( u16 n = 0; n < g_NumFT; n++)
	{
		  str.Format(_T("N:%ld H:%d W:%d T:%.3f Filter:%d X:%.3f Y:%.3f Z:%.3f ND:%d F1:%.3f F2:%.3f F3:%.3f \n"),
		        pictures_written, h, w, B.time_now, B.filter_wheel, \
				B.MPC_m_nXCurr_Micron, B.MPC_m_nYCurr_Micron, B.MPC_m_nZCurr_Micron, \
				B.nd_setting, \
				B.ADC_20Hz, B.ADC_1Hz, B.ADC_01Hz);
			
		
/*
	B.ADC_20Hz = 0.0;
	B.ADC_1Hz  = 0.0;
	B.ADC_01Hz = 0.0;
	MPC_m_nXCurr_Micron;
	double MPC_m_nYCurr_Micron;
	double MPC_m_nZCurr_Micron;
*/

		pictures_written++;

		fprintf( pFileHead, str );	
	}
	fflush( pFileHead );

	fwrite( frames_to_save, sizeof(u16), w * h * g_NumFT, pFileData );
	fflush( pFileData );

	B.expt_time = B.time_now - B.savetime;
	B.expt_frame++;
}

void COctopusPictureDisplay::UpdateTitle( void ) 
{
	CString temp;
	
	temp.Format(_T("Min:%d Max:%d Cutoff:%.1f Mean:%.1f Focus:%.2f Cam_Freq(Hz):%.1f PicsTransfer:%d\n"), \
		         g_min, g_max, g_mp, g_mean, B.focus_score, camera_freq, g_NumFT);
	temp.AppendFormat(_T("Frames in current file:%d  Total saved frames:%d Time(s):%.1f SaveDuration(s):%.1f \n"), \
		         pictures_written, B.expt_frame, B.time_now, B.expt_time ); 
	temp.AppendFormat(_T("Volts:%.4f ADC Ch1:%.4f"), B.volts, B.ADC_1);

	m_info.SetWindowText( temp );
}

/**************************************************************************
********************* MOUSE ***********************************************
**************************************************************************/

void COctopusPictureDisplay::ValidateMarkersAndSetROI( void ) 
{	
	//first, correct offset
	Marker1.x = Marker1.x - 6;
	Marker2.x = Marker2.x - 6;
	Marker1.y = Marker1.y - 6 - control_height;
	Marker2.y = Marker2.y - 6 - control_height;

	//second, correct scale
	Marker1.x = (u32)( (double)Marker1.x / zoom );
	Marker2.x = (u32)( (double)Marker2.x / zoom );
	Marker1.y = (u32)( (double)Marker1.y / zoom );
	Marker2.y = (u32)( (double)Marker2.y / zoom );

	//we are now in true camera pixels

	//correct for bottom to top dragging etc.
	if( Marker2.x < Marker1.x ) 
	{
		u32 temp = Marker1.x;
		Marker1.x = Marker2.x;	
		Marker2.x = temp;	
	} 

	if( Marker2.y < Marker1.y ) 
	{
		u32 temp = Marker1.y;
		Marker1.y = Marker2.y;	
		Marker2.y = temp;	
	} 

	u32 temp = Marker1.x;

	Marker1.x = B.ROI_actual.x2 - Marker2.x;
	Marker2.x = B.ROI_actual.x2 - temp;

	Marker1.y += B.ROI_actual.y1;
	Marker2.y += B.ROI_actual.y1;

	ROI.x1 = Marker1.x;
	ROI.x2 = Marker2.x;
	ROI.y1 = Marker1.y;
	ROI.y2 = Marker2.y;

	if( ( ROI.x2 - ROI.x1 < 14 ) ||  ( ROI.y2 - ROI.y1 < 14 ) ) 
	{
		ROI.x1 = 1;
		ROI.y1 = 1;
		ROI.x2 = ROI.x1 + 14;
		ROI.y2 = ROI.y1 + 14;
		AfxMessageBox(_T("ROI too small - making it a bit bigger."));
	} 

	B.ROI_target  = ROI;
	B.ROI_changed = true;
}

void COctopusPictureDisplay::MouseMove( CPoint point ) 
{
	
	if( point.x < pic_x1 ) return;
	if( point.x > pic_x2 ) return;
	if( point.y < pic_y1 ) return;
	if( point.y > pic_y2 ) return;
	
	Draw_Bitmap();

	CClientDC aDC( this );
	CDC* pDC = &aDC;
	
	CPen pen(PS_SOLID,2,COL_BLUE);
	pDC->SelectObject(&pen);
	pDC->MoveTo( Marker1.x , Marker1.y); 
	pDC->LineTo( point.x   , Marker1.y); 
	pDC->LineTo( point.x   , point.y  );
	pDC->LineTo( Marker1.x , point.y  );
	pDC->LineTo( Marker1.x , Marker1.y);
}

void COctopusPictureDisplay::LeftButtonDown( CPoint point ) 
{	
	if( point.x < pic_x1 ) return;
	if( point.x > pic_x2 ) return;
	if( point.y < pic_y1 ) return;
	if( point.y > pic_y2 ) return;
	
	Marker1 = point;
	Marking = true;
}

void COctopusPictureDisplay::LeftButtonUp( CPoint point ) 
{
	if( point.x < pic_x1 ) return;
	if( point.x > pic_x2 ) return;
	if( point.y < pic_y1 ) return;
	if( point.y > pic_y2 ) return;
	
	Marker2 = point;	
	Marking = false;

	ValidateMarkersAndSetROI();
}

void COctopusPictureDisplay::OnMouseMove( UINT nFlags, CPoint point ) 
{
	if ( B.Camera_Thread_running ) return;
	if ( Marking ) MouseMove( point );
}

void COctopusPictureDisplay::OnLButtonUp( UINT nFlags, CPoint point ) 
{
	if ( B.Camera_Thread_running ) return;
	LeftButtonUp( point );
}

void COctopusPictureDisplay::OnLButtonDown( UINT nFlags, CPoint point ) 
{
	if ( B.Camera_Thread_running ) return;
	LeftButtonDown( point );
}

void COctopusPictureDisplay::OnRButtonDown( UINT nFlags, CPoint point ) 
{	
	if ( B.Camera_Thread_running ) return;
	glob_m_pCamera->SetROI_To_Default();
}

BOOL COctopusPictureDisplay::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int id = LOWORD(wParam);    // Notification code
	if( id == 1 ) return FALSE; // Trap RTN key
	if( id == 2 ) return FALSE; // Trap ESC key
    return CDialog::OnCommand(wParam, lParam);
}