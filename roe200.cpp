// roe200.cpp : Defines the entry point for the DLL application.

#include "stdafx.h"
#include "ftd2xx.h"
#include "roe200.h"
#include <math.h>
#include "OctopusGlobals.h"

extern COctopusGlobals	  B;

ROE200::ROE200(void) {  busy = false; }
ROE200::~ROE200(void) { }

void ROE200::Close( void )
{
	DWORD ret_bytes;
	char rxbuf[13];
	//flush the buffer first - usually lots of crap in there
	FT_Read( ftHandle, rxbuf, 1, &ret_bytes);
	
	while( ret_bytes > 0 ) 
		FT_Read( ftHandle, rxbuf, 1, &ret_bytes);	

	FT_ResetDevice( ftHandle );
	FT_SetTimeouts( ftHandle, 300, 300 );
	FT_Purge( ftHandle, FT_PURGE_RX || FT_PURGE_TX );
	
	Sleep(1000);

	FT_Close( ftHandle );
	
	//Sleep(1000);
}
int ROE200::Initialize( void )
{	
	FT_STATUS ftStatus;

	ftStatus = FT_OpenEx("Sutter Instrument ROE-200", FT_OPEN_BY_DESCRIPTION, &ftHandle);
	
	if (ftStatus != FT_OK) 
	{
		return int(ftStatus); 
	}

	unsigned char txbuf[25], rxbuf[25];
	
	DWORD ret_bytes;

	FT_ResetDevice( ftHandle );
	FT_SetTimeouts( ftHandle, 300, 300 );
	FT_Purge( ftHandle, FT_PURGE_RX || FT_PURGE_TX );
	FT_SetBaudRate( ftHandle, 128000 );
	
	txbuf[0] = 0x22;

	FT_Write( ftHandle, txbuf, 1, &ret_bytes );
	FT_Read(  ftHandle, rxbuf, 1, &ret_bytes );
	
	//if no response maybe windows was busy... read it again
	if( ret_bytes == 0 ) 
		FT_Read(ftHandle, rxbuf, 1, &ret_bytes);
	
	FT_SetTimeouts( ftHandle, 300, 300);
	
	return 0;
}

void ROE200::StopMovingNow( void ) 
{	
	unsigned char txbuf[3], rxbuf[3];
	DWORD ret_bytes;

	FT_ResetDevice( ftHandle );
	FT_Purge( ftHandle, FT_PURGE_RX || FT_PURGE_TX );
	FT_ResetDevice( ftHandle );
	FT_SetTimeouts( ftHandle, 3000, 3000 );
	Sleep(150);

	txbuf[0] = 0x03; //CTRL-C
	FT_Write( ftHandle, txbuf, 1, &ret_bytes );
	FT_Read(  ftHandle, rxbuf, 1, &ret_bytes );
	Sleep(2);

	if(ret_bytes == 0) 
		FT_Read( ftHandle, rxbuf, 1, &ret_bytes);

}

double ROE200::CalculateMoveDuration( double dx, double dy, double dz, double vel )
{
/*
The value of X may range from 0x00 to 0x0F.  0x0F is the fastest speed 
for S moves and is about 1.3 mm/s.  Assuming more than one axis is 
involved in the move, X is the speed for the axis making with the 
longest travel in the three axis move.  We require all axes to finish 
the move at the same time, thus the speed of movement of the other 
axes will be slower in order to force the moving axes to finish simultaneously.

A value for X of 0x0F encodes the fastest speed along the longest 
axis and is about 1.3mm/sec.  A value of 0x00 for the low nibble is the 
slowest speed and is 1/16th of the fastest speed. This means if X is 0x08, the 
speed of S movement will only be half of the speed when X is set at 0x09.

JTL - that last sentence has got to be wrong!
*/

//all units are in microns
double moveduration_mS = 0.0;

//which one is largest?
double max_Microns = dx;
if (dy > max_Microns) { max_Microns = dy; }
if (dz > max_Microns) { max_Microns = dz; }

double vel_MicronsPerSec = 5000.0 / ( 16.0 - vel );

moveduration_mS = ( max_Microns / vel_MicronsPerSec ) * 1000;

if ( moveduration_mS > 15000 ) moveduration_mS = 15000;

return moveduration_mS;

}

void ROE200::StepXYZ( double dx, double dy, double dz, int v )
{

	char txbuf[12];
	for ( int i=0; i<12; i++ ) txbuf[i] = 0;
	
	DWORD ret_bytes;
		
	//update current position
	UpdatePosition();
	
	//m_nXCur, etc. now have the current position in microns, in steps of 1 micron
	long new_x = long(gXCurr_Micron + dx);
	if ((new_x < 0) || (new_x > 25000)) { return; }
	
	long new_y = long(gYCurr_Micron + dy);
	if ((new_y < 0) || (new_y > 25000)) { return; }
	
	long new_z = long(gZCurr_Micron + dz);
	if ((new_z < 0) || (new_z > 25000)) { return; }
	
    	//'O' = Stream position information
	//turn it off with 'F'
	txbuf[0] =  'F'; 
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//'S' = Straight line with user set velocity
	txbuf[0] = 'S';
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//set speed
	memcpy(txbuf,(char*)&v,1);
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);
 
	double moveduration_mS = CalculateMoveDuration( dx, dy, dz, v );
	
	//ok, at this point, we should be good
	long xc = new_x * 16;
	long yc = new_y * 16;
	long zc = new_z * 16;

	memcpy(txbuf + 0,(char*)&xc,4);
	memcpy(txbuf + 4,(char*)&yc,4);
	memcpy(txbuf + 8,(char*)&zc,4);

	FT_Write( ftHandle, txbuf, 12, &ret_bytes);
	
	Sleep( (unsigned long) moveduration_mS );
}

void ROE200::GotoCenter( void ) 
{
	char txbuf[12]; 
	for (int i=0;i<12;i++) txbuf[i] = 0;
	DWORD ret_bytes;

	double xt = 12500.0;
	double yt = 12500.0;
	double zt = 12500.0;

	//update current position
	UpdatePosition();

	double dx = fabs( gXCurr_Micron - xt );
	double dy = fabs( gYCurr_Micron - yt );
	double dz = fabs( gZCurr_Micron - zt );
	
	int v = 14;
    
	double moveduration_mS = CalculateMoveDuration( dx, dy, dz, v );
	
	//'O' = Stream position information
	//turn it off with 'F'
	txbuf[0] =  'F'; 
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//'S' = Straight line with user set velocity
	txbuf[0] = 'S';
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//set speed
	memcpy(txbuf + 0,(char*)&v,1);
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//set target
	long x = long(xt * 16.0);
	long y = long(yt * 16.0);
	long z = long(zt * 16.0);

	memcpy(txbuf +  0,(char*)&x,4);
	memcpy(txbuf +  4,(char*)&y,4);
	memcpy(txbuf +  8,(char*)&z,4);

	//we start moving now
	FT_Write(ftHandle, txbuf, 12, &ret_bytes);
	
	Sleep( (unsigned long) moveduration_mS );

}

void ROE200::TryUpdatePosition( void ) 
{
	if ( !busy )
	{
		UpdatePosition(); 
	}
	else
		return;
}

void ROE200::UpdatePosition( void ) 
{
	char txbuf[1], rxbuf[13];
	DWORD ret_bytes;
	
	//flush the buffer first - usually lots of crap in there
	FT_Read( ftHandle, rxbuf, 1, &ret_bytes);
	
	while( ret_bytes > 0 ) 
		FT_Read( ftHandle, rxbuf, 1, &ret_bytes);	

	//'C' = get current position
	txbuf[0] = 'C'; 
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(50);

	FT_Read( ftHandle, rxbuf, 13, &ret_bytes);	
	Sleep(50);
		
	if( ret_bytes == 0 ) 
		FT_Read( ftHandle, rxbuf, 13, &ret_bytes);

	long x = 400; //if these numbers are ever returned by this function, there's a problem
	long y = 500;
	long z = 600;

	memcpy( &x, rxbuf + 0, 4 );
	memcpy( &y, rxbuf + 4, 4 );
	memcpy( &z, rxbuf + 8, 4 );

	gXCurr_Step = x / 256;
	gYCurr_Step = y / 256;
	gZCurr_Step = z / 256;
	
	gXCurr_Micron = double( gXCurr_Step ) / 16.0;
	gYCurr_Micron = double( gYCurr_Step ) / 16.0;
	gZCurr_Micron = double( gZCurr_Step ) / 16.0;

}

double ROE200::GetX( void ) { return gXCurr_Micron; };
double ROE200::GetY( void ) { return gYCurr_Micron; };
double ROE200::GetZ( void ) { return gZCurr_Micron; };

