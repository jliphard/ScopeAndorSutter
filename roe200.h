// roe200.h
//
#if !defined(ROE200h)
#define ROE200h

#include "stdafx.h"
#include "ftd2xx.h"
#include "OctopusGlobals.h"

class ROE200
{

public:

	ROE200(void);
	~ROE200(void);

	int Initialize( void );
	void Close( void );
	void GotoCenter( void );

	void StepXYZ( double x, double y, double z, int v );

	void TryUpdatePosition( void );

	double GetX( void );
	double GetY( void );
	double GetZ( void );

protected:

	bool busy;
	void UpdatePosition( void );

	double CalculateMoveDuration( double xn, double yn, double zn, double xt, double yt, double zt, double vel );
	double CalculateMoveDuration( double dx, double dy, double dz, double vel );

	void OnVectorMove_XZ( double x, double z, int v );
	void OnVectorMove_X(  double x,           int v );
	void OnVectorMove_Z(  double z,           int v );

	//done
	void Step_X( long target_steps  , long currentsteps_y, long currentsteps_z );
	void Step_Z( long currentsteps_x, long currentsteps_y, long target_steps );


	FT_HANDLE ftHandle;

	double	gXCurr_Micron;
	double	gYCurr_Micron;
	double	gZCurr_Micron;

	long    gXCurr_Step;
	long    gYCurr_Step;
	long    gZCurr_Step;
};

#endif // !defined(ROE200h)
