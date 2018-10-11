///////////////////////////////////////////////////////////////////////////////
// FILE:          SutterLambda.h
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
// AUTHOR:        Nenad Amodaj, nenad@amodaj.com, 10/26/2005
//                Nico Stuurman, Oct. 2010
//
// CVS:           $Id$
//

#ifndef _SUTTER_LAMBDA_H_
#define _SUTTER_LAMBDA_H_

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include <string>
#include <map>

//////////////////////////////////////////////////////////////////////////////
// Error codes
//
#define ERR_UNKNOWN_POSITION         10002
#define ERR_INVALID_SPEED            10003
#define ERR_PORT_CHANGE_FORBIDDEN    10004
#define ERR_SET_POSITION_FAILED      10005
#define ERR_UNKNOWN_SHUTTER_MODE     10006
#define ERR_UNKNOWN_SHUTTER_ND       10007
#define ERR_NO_ANSWER                10008

class SutterHub : public HubBase<SutterHub>
{
public:
	SutterHub();
	~SutterHub();

	//Device API
	int Initialize();
	int Shutdown() { return DEVICE_OK; };
	void GetName(char* pName) const;
	bool Busy() { return busy_; };

	bool SupportsDeviceDetection() { return true; };
	MM::DeviceDetectionStatus DetectDevice();

	//Hub API
	int DetectInstalledDevices();

	//Action API
	int OnPort(MM::PropertyBase* pProp, MM::ActionType eAct);

	//From old sutterutil class
	bool ControllerBusy();
	int GoOnLine(unsigned long answerTimeoutMs);
	int GetControllerType(unsigned long answerTimeoutMs, std::string& type, std::string& id);
	int GetStatus(unsigned long answerTimeoutMs, unsigned char* status);
	int SetCommand(const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, const unsigned long answerTimeoutMs, std::vector<unsigned char>& response, const bool responseRequired = true, const bool CRExpected = true);

	//Make comms thread safe
	static MMThreadLock& GetLock() { return lock_; };
private:
	std::string port_;
	bool busy_;
	bool initialized_;
	static MMThreadLock lock_;

};

class Wheel : public CStateDeviceBase<Wheel>
{
public:
   Wheel(const char* name, unsigned id);
   ~Wheel();
  
   // MMDevice API
   // ------------
   int Initialize();
   int Shutdown();
  
   void GetName(char* pszName) const;
   bool Busy();
   unsigned long GetNumberOfPositions()const {return numPos_;}

   // action interface
   // ----------------
   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnSpeed(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnDelay(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnBusy(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnAnswerTimeout(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   bool SetWheelPosition(unsigned pos);

   bool initialized_;
   unsigned numPos_;
   const unsigned id_;
   std::string name_;
   unsigned curPos_;
   bool open_;
   unsigned speed_;
   double answerTimeoutMs_;
   Wheel& operator=(Wheel& /*rhs*/) {assert(false); return *this;}
};

class Shutter : public CShutterBase<Shutter>
{
public:
   Shutter(const char* name, int id);
   ~Shutter();

   bool Busy();
   void GetName(char* pszName) const;
   int Initialize();
   int Shutdown();
      
   // Shutter API
   int SetOpen(bool open = true);
   int GetOpen(bool& open);

   // action interface
   // ----------------
   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnMode(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnND(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnControllerID(MM::PropertyBase* pProp, MM::ActionType eAct);
   // for device discovery:
   bool SupportsDeviceDetection(void);
   MM::DeviceDetectionStatus DetectDevice(void);


private:
   bool SetShutterPosition(bool state);
   bool SetShutterMode(const char* mode);
   bool SetND(unsigned int nd);

   bool initialized_;
   const int id_;
   std::string name_;
   unsigned int nd_;
   std::string controllerType_;
   std::string controllerId_;
   double answerTimeoutMs_;
   MM::MMTime changedTime_;
   std::string curMode_;
   Shutter& operator=(Shutter& /*rhs*/) {assert(false); return *this;}
};


/*
class LambdaVF5: public Wheel //CStateDeviceBase<LambdaVF5>
{
public:
	LambdaVF5(const char* name, unsigned id) : Wheel(name, id) {};

	//VF-5 special commands
	int onWhiteLightMode();// (bool enabled);
	int onWavelength();//(unsigned int wavelength);
	int onSpeed();//(unsigned int speed);
	int onWheelTilt();
	int onMotorsEnabled();
private:
	int getStatus();


};*/
#endif //_SUTTER_LAMBDA_H_
