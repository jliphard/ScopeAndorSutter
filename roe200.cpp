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

double ROE200::CalculateMoveDuration( double xn, double yn, double zn, double xt, double yt, double zt, double vel )
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

double dx = fabs( xn - xt );
double dy = fabs( yn - yt );
double dz = fabs( zn - zt );

//which one is largest?
double max_Microns = dx;
if (dy > max_Microns) { max_Microns = dy; }
if (dz > max_Microns) { max_Microns = dz; }

double vel_MicronsPerSec = 5000.0 / ( 16.0 - vel );

moveduration_mS = ( max_Microns / vel_MicronsPerSec ) * 1000;

if ( moveduration_mS > 15000 ) moveduration_mS = 15000;

return moveduration_mS;

}

void ROE200::MoveFwdBack_XZ( long step_micron, int cycles, int v, double probeangle )
{

	//Diagonal move
	//where did we start the cycle?
	//update current position
	UpdatePosition();

	//XCurr, etc. now have the current position in microns or steps
	double xcenter       = gXCurr_Micron;
	double zcenter       = gZCurr_Micron;

	//where do we want to go? Calculate the target coordinates
	double alpha      = 90.0;
	double gamma      = 90.0 - probeangle;
	double Pi         = 3.14159;
	double dx         = step_micron * sin(gamma * Pi / 180.0) / sin(alpha * Pi / 180.0);
	double dz         = sqrt(pow(step_micron,2.0) - pow(dx,2.0));
	
	double xfront     = xcenter - dx;
	if ( step_micron < 0 ) { dz = -1 * dz; }
	double zfront     = zcenter - dz;

	double xback      = xcenter + dx;
	if ( step_micron < 0 ) { dz = -1 * dz; }
	double zback      = zcenter + dz;

	for( int i = 0; i < cycles; i++ )
	{
		//go to 'front'
		OnVectorMove_XZ( xfront, zfront, v );
		Sleep(1000);
		//go to 'back'
		OnVectorMove_XZ( xback, zback, v );
		Sleep(1000);
	}

	//go back to beginning
	OnVectorMove_XZ( xcenter, zcenter, v );
}

void ROE200::MoveFwdBackFast_X( long amplitude_micron, int cycles, int v, int waittime_ms )
{

	//where did we start the cycle?
	//update current position
	UpdatePosition();

	//XCurr, etc. now have the current position in microns or steps
	double xcenter    = gXCurr_Micron;

	//where do we want to go? Calculate the target coordinates
	double dx         = amplitude_micron / 2;

	double xfront     = xcenter - dx;
	double xback      = xcenter + dx;

	for( int i = 0; i < cycles; i++ )
	{
		OnVectorMove_X( xfront, v ); 
		Sleep(waittime_ms);
		OnVectorMove_X( xback, v );
		Sleep(waittime_ms);
	}

	OnVectorMove_X( xcenter, v );
}

void ROE200::MoveFwdBackFast_Z( long amplitude_micron, int cycles, int v, int waittime_ms )
{

	//where did we start the cycle?
	//update current position
	UpdatePosition();

	//XCurr, etc. now have the current position in microns or steps
	double zcenter    = gZCurr_Micron;

	//where do we want to go? Calculate the target coordinates
	double dz         = amplitude_micron / 2;

	double zfront     = zcenter - dz;
	double zback      = zcenter + dz;

	for( int i = 0; i < cycles; i++ )
	{
		OnVectorMove_Z( zfront, v ); 
		Sleep(waittime_ms);
		OnVectorMove_Z( zback, v );
		Sleep(waittime_ms);
	}

	OnVectorMove_Z( zcenter, v );
}

void ROE200::MoveFwdBackSlow_X( long amplitude_micron, int cycles, int v_micron_per_sec, int waittime_ms)
{

	busy = true;
	//where did we start the cycle?
	//update current position
	UpdatePosition();

	//XCurr now has the current position in steps
	long xcenter_steps = double(gXCurr_Micron) * 16.0;
	long ycenter_steps = double(gYCurr_Micron) * 16.0;
	long zcenter_steps = double(gZCurr_Micron) * 16.0;

	//long dx_half       = amplitude_micron * 16 / 2; 

	//Assume 1 micron steps, always
	long steps         = amplitude_micron;
	long steps_half    = amplitude_micron / 2;
	long steps_fourth  = amplitude_micron / 4;
	
	//where do we want to go? Calculate the target coordinates
	//long xfront     = xcenter_steps - dx_half;
	//long xback      = xcenter_steps + dx_half;

	long target_steps = xcenter_steps;
	
	//extra delay
	//20 m/s =     no delay
	//10 m/s =  50 ms delay
	//1 m/s  = 950 ms delay
	
	//move to front

	//M command runs at 20 Hz at most
	//Max speed is therefore 20 microns per sec

	if( v_micron_per_sec < 1 )
		v_micron_per_sec = 1;
	else if ( v_micron_per_sec > 20 )
		v_micron_per_sec = 20;

	if( waittime_ms < 500 )
		waittime_ms = 500;
	else if ( waittime_ms > 10000 )
		waittime_ms = 10000;

	int delay_ms = (1000 / v_micron_per_sec) - 20;

	int i = 0; 

	for(i = 1; i <= steps_half; i++) 
	{
		target_steps = target_steps - 16; //move forward in steps of 1 micron
		Step_X( target_steps, ycenter_steps, zcenter_steps );
		Sleep( delay_ms );
	}

	Sleep(waittime_ms);

	for(int c = 0; c < cycles; c++ )
	{
		//now i'm at the front, move back
		for(i = 1; i <= steps; i++) 
		{
			target_steps = target_steps + 16; //move backward in steps of 1 micron
			Step_X( target_steps, ycenter_steps, zcenter_steps );
			Sleep( delay_ms );
		}
		Sleep(waittime_ms);
		//now i'm at the back, move forward
		for(i = 1; i <= steps; i++) 
		{
			target_steps = target_steps - 16; //move forward in steps of 1 micron
			Step_X( target_steps, ycenter_steps, zcenter_steps );
			Sleep( delay_ms );
		}
		Sleep(waittime_ms);
	}
	
	//end at the front - move to center so we are back where we started the cycle
	for(i = 1; i <= steps_half; i++) 
	{
		target_steps = target_steps + 16; //move backward in steps of 2 micron
		Step_X( target_steps, ycenter_steps, zcenter_steps );
		Sleep( delay_ms );
	}

	busy = false;
}

void ROE200::MoveFwdBackSlow_Z( long amplitude_micron, int cycles, int v_micron_per_sec, int waittime_ms)
{
	busy = true;
	//where did we start the cycle?
	//update current position
	UpdatePosition();

	//XCurr now has the current position in steps
	long xcenter_steps = double(gXCurr_Micron) * 16.0;
	long ycenter_steps = double(gYCurr_Micron) * 16.0;
	long zcenter_steps = double(gZCurr_Micron) * 16.0;

	//long dx_half       = amplitude_micron * 16 / 2; 

	//Assume 1 micron steps, always
	long steps         = amplitude_micron;
	long steps_half    = amplitude_micron / 2;
	long steps_fourth  = amplitude_micron / 4;
	
	//where do we want to go? Calculate the target coordinates
	//long xfront     = xcenter_steps - dx_half;
	//long xback      = xcenter_steps + dx_half;

	long target_steps = zcenter_steps;
	
	//extra delay
	//20 m/s =     no delay
	//10 m/s =  50 ms delay
	//1 m/s  = 950 ms delay
	
	//move to front

	//M command runs at 20 Hz at most
	//Max speed is therefore 20 microns per sec

	if( v_micron_per_sec < 1 )
		v_micron_per_sec = 1;
	else if ( v_micron_per_sec > 20 )
		v_micron_per_sec = 20;

	if( waittime_ms < 500 )
		waittime_ms = 500;
	else if ( waittime_ms > 10000 )
		waittime_ms = 10000;

	int delay_ms = (1000 / v_micron_per_sec) - 20;

	int i = 0; 

	for(i = 1; i <= steps_half; i++) 
	{
		target_steps = target_steps - 16; //move forward in steps of 1 micron
		Step_Z( xcenter_steps, ycenter_steps, target_steps );
		Sleep( delay_ms );
	}

	Sleep( waittime_ms );

	for(int c = 0; c < cycles; c++ )
	{
		//now i'm at the front, move back
		for(i = 1; i <= steps; i++) 
		{
			target_steps = target_steps + 16; //move backward in steps of 1 micron
			//Step_X( target_steps, ycenter_steps, zcenter_steps );
			Step_Z( xcenter_steps, ycenter_steps, target_steps );
			Sleep( delay_ms );
		}
		
		Sleep( waittime_ms );
		
		//now i'm at the back, move forward
		for(i = 1; i <= steps; i++) 
		{
			target_steps = target_steps - 16; //move forward in steps of 1 micron
			//Step_X( target_steps, ycenter_steps, zcenter_steps );
			Step_Z( xcenter_steps, ycenter_steps, target_steps );
			Sleep( delay_ms );
		}
		
		Sleep(waittime_ms);
	}
	
	//end at the front - move to center so we are back where we started the cycle
	for(i = 1; i <= steps_half; i++) 
	{
		target_steps = target_steps + 16; //move backward in steps of 2 micron
		//Step_X( target_steps, ycenter_steps, zcenter_steps );
		Step_Z( xcenter_steps, ycenter_steps, target_steps );
		Sleep( delay_ms );
	}

	busy = false;
}

void ROE200::MoveFwdOrBackOnce_XZ( long step_micron, int v,  double probeangle )
{
	
	//DIAGONAL MOVE
	//where did we start the cycle?
	//update current position
	UpdatePosition();

	//XCurr, etc. now have the current position in microns or steps
	double xbeg       = gXCurr_Micron;
	double zbeg       = gZCurr_Micron;

	//where do we want to go? Calculate the target coordinates
	double alpha      = 90.0;
	double gamma      = 90.0 - probeangle;
	double Pi         = 3.14159;
	double dx         = step_micron * sin(gamma * Pi / 180.0) / sin(alpha * Pi / 180.0);
	double dz         = sqrt(pow(step_micron,2.0) - pow(dx,2.0));
	double xend       = xbeg - dx;
	if ( step_micron < 0 ) { dz = -1 * dz; }
	double zend       = zbeg - dz;

	OnVectorMove_XZ( xend, zend, v ); 
}

void ROE200::MoveFwdOrBackOnce_X( long step_micron, int v )
{
	//where did we start the cycle?
	//update current position
	UpdatePosition();

	//XCurr, etc. now have the current position in microns or steps
	double xbeg       = gXCurr_Micron;

	//where do we want to go? Calculate the target coordinates
	double dx         = step_micron;
	double xend       = xbeg - dx;

	OnVectorMove_X( xend, v ); 
}

void ROE200::MoveFwdOrBackOnce_Z( long step_micron, int v )
{
	//where did we start the cycle?
	//update current position
	UpdatePosition();

	//XCurr, etc. now have the current position in microns or steps
	double zbeg       = gZCurr_Micron;

	//where do we want to go? Calculate the target coordinates
	double dz         = step_micron;
	double zend       = zbeg - dz;

	OnVectorMove_Z( zend, v ); 
}

void ROE200::OnVectorMove_XZ( double x, double z, int v ) 
{
	//this function is needed to avoid accumulation of rounding errors in repeated cycling.
	//go to specific position at end of cycle
	
	if ((x < 0.0) || (x > 25000.0)) { return; }
	if ((z < 0.0) || (z > 25000.0)) { return; }

	//update current position
	UpdatePosition();

	double dx = fabs( gXCurr_Micron - x );
	double dz = fabs( gZCurr_Micron - z );

	double moveduration_mS = CalculateMoveDuration( dx, 0.0, dz, v );

	char txbuf[14]; 
	for (int i=0;i<14;i++) txbuf[i] = 0;
	DWORD ret_bytes;

	//ok, at this point, we are ready to go
	long xt = long(x * 16);
	long yt = gYCurr_Step;
	long zt = long(z * 16);
		
	//'O' = Stream position information
	//turn it off with 'F'
	txbuf[0] =  'F'; 
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//'S' = Straight line with user set velocity
	txbuf[0] =  'S';
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//set speed
	memcpy(txbuf + 0,(char*)&v,1); //in units of 0 to 14, whatever that is
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//set target
	memcpy(txbuf +  0,(char*)&xt,4);
	memcpy(txbuf +  4,(char*)&yt,4);
	memcpy(txbuf +  8,(char*)&zt,4);

	//we start moving now
	FT_Write( ftHandle, txbuf, 12, &ret_bytes);
	
	Sleep( (unsigned long) moveduration_mS );

}

void ROE200::OnVectorMove_X( double x, int v ) 
{
	//this function is needed to avoid accumulation of rounding errors in repeated cycling.
	//go to specific position at end of cycle
	
	if ((x < 0.0) || (x > 25000.0)) { return; }

	//update current position
	UpdatePosition();

	double dx = fabs( gXCurr_Micron - x );

	double moveduration_mS = CalculateMoveDuration( dx, 0.0, 0.0, v );

	char txbuf[14]; 
	for (int i=0;i<14;i++) txbuf[i] = 0;
	DWORD ret_bytes;

	//ok, at this point, we are ready to go
	long xt = long(x * 16);
	long yt = gYCurr_Step;
	long zt = gZCurr_Step;
		
	//'O' = Stream position information
	//turn it off with 'F'
	txbuf[0] =  'F'; 
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//'S' = Straight line with user set velocity
	txbuf[0] =  'S';
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//set speed
	memcpy(txbuf + 0,(char*)&v,1); //in units of 0 to 14, whatever that is
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//set target
	memcpy(txbuf +  0,(char*)&xt,4);
	memcpy(txbuf +  4,(char*)&yt,4);
	memcpy(txbuf +  8,(char*)&zt,4);

	//we start moving now
	FT_Write( ftHandle, txbuf, 12, &ret_bytes);
	
	Sleep( (unsigned long) moveduration_mS );

}

void ROE200::OnVectorMove_Z( double z, int v ) 
{
	//this function is needed to avoid accumulation of rounding errors in repeated cycling.
	//go to specific position at end of cycle
	
	if ((z < 0.0) || (z > 25000.0)) { return; }

	//update current position
	UpdatePosition();

	double dz = fabs( gZCurr_Micron - z );

	double moveduration_mS = CalculateMoveDuration( 0.0, 0.0, dz, v );

	char txbuf[14]; 
	for (int i=0;i<14;i++) txbuf[i] = 0;
	DWORD ret_bytes;

	//ok, at this point, we are ready to go
	long xt = gXCurr_Step;
	long yt = gYCurr_Step;
	long zt = long(z * 16);
		
	//'O' = Stream position information
	//turn it off with 'F'
	txbuf[0] =  'F'; 
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//'S' = Straight line with user set velocity
	txbuf[0] =  'S';
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//set speed
	memcpy(txbuf + 0,(char*)&v,1); //in units of 0 to 14, whatever that is
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	//set target
	memcpy(txbuf +  0,(char*)&xt,4);
	memcpy(txbuf +  4,(char*)&yt,4);
	memcpy(txbuf +  8,(char*)&zt,4);

	//we start moving now
	FT_Write( ftHandle, txbuf, 12, &ret_bytes);
	
	Sleep( (unsigned long) moveduration_mS );

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

void ROE200::StepFB_X( double dx )
{

	char txbuf[12];
	for ( int i=0; i<12; i++ ) txbuf[i] = 0;
	
	DWORD ret_bytes;
		
	//update current position
	UpdatePosition();
	
	//m_nXCur, etc. now have the current position in microns, in steps of 1 micron
	long new_x = long(gXCurr_Micron + dx);
	if ((new_x < 0) || (new_x > 25000)) { return; }
	
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
	u16 v = 3;
	memcpy(txbuf,(char*)&v,1);
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);
 
	double moveduration_mS = CalculateMoveDuration( dx, 0, 0, v );
	
	//ok, at this point, we should be good
	long xc = new_x * 16;
	long yc = gYCurr_Micron * 16;
	long zc = gZCurr_Micron * 16;

	memcpy(txbuf + 0,(char*)&xc,4);
	memcpy(txbuf + 4,(char*)&yc,4);
	memcpy(txbuf + 8,(char*)&zc,4);

	FT_Write( ftHandle, txbuf, 12, &ret_bytes);
	
	Sleep( (unsigned long) moveduration_mS );

}

void ROE200::StepFB_Z( double dz )
{

	char txbuf[12];
	for ( int i=0; i<12; i++ ) txbuf[i] = 0;
	
	DWORD ret_bytes;
		
	//update current position
	UpdatePosition();
	
	//m_nXCur, etc. now have the current position in microns, in steps of 1 micron
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
	u16 v = 3;
	memcpy(txbuf,(char*)&v,1);
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);
 
	double moveduration_mS = CalculateMoveDuration( 0, 0, dz, v );
	
	//ok, at this point, we should be good
	long xc = gXCurr_Micron * 16;
	long yc = gYCurr_Micron * 16;
	long zc = new_z * 16;

	memcpy(txbuf + 0,(char*)&xc,4);
	memcpy(txbuf + 4,(char*)&yc,4);
	memcpy(txbuf + 8,(char*)&zc,4);

	FT_Write( ftHandle, txbuf, 12, &ret_bytes);
	
	Sleep( (unsigned long) moveduration_mS );

}

void ROE200::Step_X( long target_steps, long currentsteps_y, long currentsteps_z )
{
	char txbuf[12];
	for ( int i=0; i<12; i++ ) txbuf[i] = 0;
	
	DWORD ret_bytes;
	
	long xc = target_steps;
	long yc = currentsteps_y;
	long zc = currentsteps_z;

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
	u16 v = 0;
	memcpy(txbuf,(char*)&v,1);
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	memcpy(txbuf + 0, (char*)&xc, 4);
	memcpy(txbuf + 4, (char*)&yc, 4);
	memcpy(txbuf + 8, (char*)&zc, 4);

	FT_Write( ftHandle, txbuf, 12, &ret_bytes );

	Sleep(100);
}

void ROE200::Step_Z( long currentsteps_x, long currentsteps_y, long target_steps )
{

	char txbuf[12];
	for ( int i=0; i<12; i++ ) txbuf[i] = 0;
	
	DWORD ret_bytes;
	
	long xc = currentsteps_x;
	long yc = currentsteps_y;
	long zc = target_steps;

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
	u16 v = 0;
	memcpy(txbuf,(char*)&v,1);
	FT_Write( ftHandle, txbuf, 1, &ret_bytes);
	Sleep(30);

	memcpy(txbuf + 0, (char*)&xc, 4);
	memcpy(txbuf + 4, (char*)&yc, 4);
	memcpy(txbuf + 8, (char*)&zc, 4);

	FT_Write( ftHandle, txbuf, 12, &ret_bytes );

	Sleep(100);
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

