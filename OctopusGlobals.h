/*
Copyright (c) 2003 Jan T. Liphardt (JTLiphardt@lbl.gov), 
Lawrence Berkeley National Laboratory
*/

#if !defined(AFX_H_OctopusGlobals)
#define AFX_H_OctopusGlobals

#include "stdafx.h"

struct COctopusROI 
{
	int x1;
	int x2;
	int y1;
	int y2;
	int bin;
};

struct COctopusGlobals 
{
	u32	files_written;
	bool	savetofile;
	u16	W;
	u16	H;
	bool	automatic_gain;
	double	manual_gain;
	u32	pics_per_file;
	u16	CCD_x_phys_e;
	u16	CCD_y_phys_e;
	bool	Camera_Thread_running;
	CString pathname;
	u16     bin;
	u16*    memory;
	double  savetime;
	u32     expt_frame;
	double  expt_time;
	double  time_now;
	bool    Ampl_Conv;
	u32     Ampl_Setting;
	u32     Ampl_Setting_old;
	u32     CameraExpTime_ms;
	bool    Andor_new;
	
	bool        ROI_changed;
	COctopusROI ROI_actual;
	COctopusROI ROI_target;
	COctopusROI Focus_ROI;
	bool        Focus_ROI_Set;
	double 	    volts;
	u32         pictures_to_skip;

	//let's try this again
	bool        SetFocus;

	//autofocus
	bool    focus_in_progress;
	double  focus_score;
	double	focus_beadX;
	double	focus_beadY;
	double	focus_beadX_LM;
	double	focus_beadY_LM;
	double  focus_beadX_sd;
	double  focus_beadY_sd; 
	double  focus_bead_fwhm; 
	double	focus_min;
	double	focus_max;

	//MPC200
	bool MPC200_loaded;
	double MPC_m_nXCurr_Micron;
	double MPC_m_nYCurr_Micron;
	double MPC_m_nZCurr_Micron;
};

#endif
