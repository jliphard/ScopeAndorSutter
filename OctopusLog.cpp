#include "stdafx.h"
#include "Octopus.h"
#include "OctopusClock.h"
#include "OctopusLog.h"
#include <stdio.h>

extern COctopusGoodClock* glob_m_pGoodClock;

COctopusLog::COctopusLog()
{
	pFile = NULL;
	entry = 0;
	Open_File();
}

COctopusLog::~COctopusLog() 
{
	if ( pFile != NULL ) 
	{
		fclose( pFile );
		pFile = NULL;
	}
}

bool COctopusLog::Open_File( void ) 
{
	pFile = fopen( "debuglog.txt" , "wt");

	if ( pFile != NULL ) 
		return true;
	else 
		return false;
}

void COctopusLog::Write( CString step ) 
{
	if( pFile == NULL ) return; 

	CString out;

	systemtime = CTime::GetCurrentTime();
	
	CString t = systemtime.Format(_T(" %m/%d/%y %H:%M:%S "));

	out.Format(_T("N:%d T:%.3f"), entry++, (double)glob_m_pGoodClock->End());
	out.Append( t );
	out.Append( step );
	out.Append(_T("\n"));

	fprintf( pFile, out );	
	fflush( pFile );
}