///////////////////////////////////////////////////////////////////////////////
// FILE:          AOTF.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   
//
// AUTHOR:        Lukas Kapitein / Erwin Peterman 24/08/2009


#pragma once

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"

//////////////////////////////////////////////////////////////////////////////
// Error codes
//
#define ERR_UNKNOWN_POSITION         10002
#define ERR_PORT_CHANGE_FORBIDDEN    10004
#define ERR_SET_POSITION_FAILED      10005
#define ERR_INVALID_STEP_SIZE        10006
#define ERR_INVALID_MODE             10008
#define ERR_UNRECOGNIZED_ANSWER      10009
#define ERR_UNSPECIFIED_ERROR        10010
#define ERR_COMMAND_ERROR            10201
#define ERR_PARAMETER_ERROR          10202
#define ERR_RECEIVE_BUFFER_OVERFLOW  10204
#define ERR_COMMAND_OVERFLOW         10206
#define ERR_PROCESSING_INHIBIT       10207
#define ERR_PROCESSING_STOP_ERROR    10208

#define ERR_OFFSET 10100
#define ERR_AOTF_OFFSET 10200
#define ERR_INTENSILIGHTSHUTTER_OFFSET 10300

//These variables are defined in moduleAPI.cpp, we include them here to make them global to the device adapter.
extern const char* g_AOTF; 
extern const char* g_mAOTF;
extern const char* g_Int;
extern const char* g_Maxint;
extern const char* g_mChannel; 
extern const char* g_Channel_1;
extern const char* g_Channel_2; 	
extern const char* g_Channel_3;	
extern const char* g_Channel_4;
extern const char* g_Channel_5;			
extern const char* g_Channel_6;	
extern const char* g_Channel_7;
extern const char* g_Channel_8;
extern const char* g_DelayBetweenChannels;
