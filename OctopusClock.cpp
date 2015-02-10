//////////////////////////////////////////////////////////////////////
// OctopusClock.cpp: implementation of the COctopusGoodClock class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Octopus.h"
#include "OctopusClock.h"

COctopusGoodClock::COctopusGoodClock() { // get the frequency of the counter
	
	if(QueryPerformanceFrequency( (LARGE_INTEGER *)&Frequency )) 
	{  
		NotInitialized = false;
	} else {
		NotInitialized = true;
	}

	if(QueryPerformanceCounter( (LARGE_INTEGER *)&BeginTime )) 
	{  
		NotInitialized = false;
	} else {
		NotInitialized = true;
	}
}

COctopusGoodClock::~COctopusGoodClock() {}

double COctopusGoodClock::End(void) const{   // stop timing and get elapsed time in seconds

      if(NotInitialized) return 0.0;         // error - couldn't get frequency

      // get the ending counter value
      QueryPerformanceCounter((LARGE_INTEGER *)&EndTime);

      // determine the elapsed counts
      __int64 Elapsed = EndTime - BeginTime;

      // convert counts to time in seconds and return it
      return (double)Elapsed / (double)Frequency;
}
