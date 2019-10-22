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
#define ERR_PORT_CHANGE_FORBIDDEN    10004


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
