///////////////////////////////////////////////////////////////////////////////
// FILE:          SutterLambda.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Sutter Lambda controller adapter
// COPYRIGHT:     University of California, San Francisco, 2006
// LICENSE:       This file is distributed under the BSD license.
//                License text is included with the source distribution.
//
//                This file is distributed in the hope that it will be useful,
//                but WITHOUT ANY WARRANTY; without even the implied warranty
//                of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//                IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//                CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.
//
// CVS:           $Id$
//

/*
#ifdef WIN32
#include <windows.h>
#define snprintf _snprintf 
#endif
*/

#include "SutterLambda.h"
#include <vector>
#include <memory>
#include <cstdio>
#include <string>
#include <math.h>
#include "../../MMDevice/ModuleInterface.h"
#include "../../MMDevice/DeviceUtils.h"
#include <sstream>

// Wheels
const char* g_WheelAName = "Wheel-A";
const char* g_WheelBName = "Wheel-B";
const char* g_WheelCName = "Wheel-C";
const char* g_ShutterAName = "Shutter-A";
const char* g_ShutterBName = "Shutter-B";

#ifdef DefineShutterOnTenDashTwo
const char* g_ShutterAName10dash2 = "Shutter-A 10-2";
const char* g_ShutterBName10dash2 = "Shutter-B 10-2";
const double g_busyTimeoutMs = 500;
#endif

const char* g_DG4WheelName = "Wheel-DG4";
const char* g_DG4ShutterName = "Shutter-DG4";

const char* g_LambdaVF5Name = "VF-5";

const char* g_ShutterModeProperty = "Mode";
const char* g_FastMode = "Fast";
const char* g_SoftMode = "Soft";
const char* g_NDMode = "ND";
const char* g_ControllerID = "Controller Info";

using namespace std;

std::map<std::string, bool> g_Busy;
std::map<std::string, MMThreadLock*> gplocks_;

int g_DG4Position = 0;
bool g_DG4State = false;

void newGlobals(std::string p)
{
   if( g_Busy.end() ==  g_Busy.find(p))
   {
      g_Busy[p] = false;
      gplocks_[p] = new MMThreadLock();
   }
}

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
   RegisterDevice(g_WheelAName, MM::StateDevice, "Lambda 10 filter wheel A");
   RegisterDevice(g_WheelBName, MM::StateDevice, "Lambda 10 filter wheel B");
   RegisterDevice(g_WheelCName, MM::StateDevice, "Lambda 10 wheel C (10-3 only)");
   RegisterDevice(g_ShutterAName, MM::ShutterDevice, "Lambda 10 shutter A");
   RegisterDevice(g_ShutterBName, MM::ShutterDevice, "Lambda 10 shutter B");
#ifdef DefineShutterOnTenDashTwo
   RegisterDevice(g_ShutterAName10dash2, MM::ShutterDevice, "Lambda 10-2 shutter A");
   RegisterDevice(g_ShutterBName10dash2, MM::ShutterDevice, "Lambda 10-2 shutter B");
#endif
   RegisterDevice(g_DG4ShutterName, MM::ShutterDevice, "DG4 shutter");
   RegisterDevice(g_DG4WheelName, MM::StateDevice, "DG4 filter changer");
   RegisterDevice(g_LambdaVF5Name, MM::StateDevice, "Lambda VF-5 (10-3 only)");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
   if (deviceName == 0)
      return 0;

   if (strcmp(deviceName, g_WheelAName) == 0)
   {
      // create Wheel A
      Wheel* pWheel = new Wheel(g_WheelAName, 0);
      return pWheel;
   }
   else if (strcmp(deviceName, g_WheelBName) == 0)
   {
      // create Wheel B
      Wheel* pWheel = new Wheel(g_WheelBName, 1);
      return pWheel;
   }
   else if (strcmp(deviceName, g_WheelCName) == 0)
   {
      // create Wheel C
      Wheel* pWheel = new Wheel(g_WheelCName, 2);
      return pWheel;
   }
   else if (strcmp(deviceName, g_ShutterAName) == 0)
   {
      // create Shutter A
      Shutter* pShutter = new Shutter(g_ShutterAName, 0);
      return pShutter;
   }
   else if (strcmp(deviceName, g_ShutterBName) == 0)
   {
      // create Shutter B
      Shutter* pShutter = new Shutter(g_ShutterBName, 1);
      return pShutter;
   }
#ifdef DefineShutterOnTenDashTwo
   else if (strcmp(deviceName, g_ShutterAName10dash2) == 0)
   {
      // create Shutter A
      ShutterOnTenDashTwo* pShutter = new ShutterOnTenDashTwo(g_ShutterAName10dash2, 0);
      return pShutter;
   }
   else if (strcmp(deviceName, g_ShutterBName10dash2) == 0)
   {
      // create Shutter B
      ShutterOnTenDashTwo* pShutter = new ShutterOnTenDashTwo(g_ShutterBName10dash2, 1);
      return pShutter;
   }
#endif
   else if (strcmp(deviceName, g_DG4ShutterName) == 0)
   {
      // create DG4 shutter
      return new DG4Shutter();
   }
   else if (strcmp(deviceName, g_DG4WheelName) == 0)
   {
      // create DG4 Wheel
      return new DG4Wheel();
   }
   /*else if (strcmp(deviceName, g_LambdaVF5Name) == 0)
   {
	   //Create Lambda VF-5 tunable filter
	   return new LambdaVF5(g_LambdaVF5Name,0);
   }*/

   return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

/*
int LambdaVF5::onWhiteLightMode() { return 0; }
int LambdaVF5::onWavelength() { return 0; }
int LambdaVF5::onSpeed() { return 0; }
*/