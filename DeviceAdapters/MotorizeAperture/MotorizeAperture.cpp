///////////////////////////////////////////////////////////////////////////////
// FILE:          MotorizedAperture.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   MotorizedAperture 
//
// AUTHOR:        Nick Anthony, BPL, 2019 driver for lab-build motorized aperture for nikon TI microscope.

#ifdef WIN32
#include <windows.h>
#define snprintf _snprintf 
#endif

#include "MotorizeAperture.h"
//#include <cstdio>
//#include <cctype>
//#include <string>
//#include <math.h>
#include "../../MMDevice/ModuleInterface.h"
//#include <sstream>
//#include <algorithm> 


const char* g_ControllerName    = "TI Motorized Aperture";

const char* g_Term            = "\r"; //unique termination

const char* g_BaudRate_key        = "Baud Rate";
const char* g_Baud9600            = "9600";


///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
   RegisterDevice(g_ControllerName, MM::GenericDevice, "MotorizedAperture");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
   if (deviceName == 0) return 0;
   if (strcmp(deviceName, g_ControllerName) == 0) 
   {
      return new MotorizedAperture();
   }
   return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}


///////////////////////////////////////////////////////////////////////////////
// MotorizedAperture Hub
///////////////////////////////////////////////////////////////////////////////

MotorizedAperture::MotorizedAperture() :
   baud_(g_Baud9600),
   initialized_(false)
{
   InitializeDefaultErrorMessages();
   // pre-initialization properties
   // Port:
   CPropertyAction* pAct = new CPropertyAction(this, &MotorizedAperture::OnPort);
   CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
   SetProperty(MM::g_Keyword_Port, port_.c_str());
}


MotorizedAperture::~MotorizedAperture()
{
   Shutdown();
}

void MotorizedAperture::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_ControllerName);
}

MM::DeviceDetectionStatus MotorizedAperture::DetectDevice(void)
{
   // all conditions must be satisfied...
   MM::DeviceDetectionStatus result = MM::Misconfigured;

   try
   {
      long baud;
      GetProperty(g_BaudRate_key, baud);

      std::string transformed = port_;
      for (std::string::iterator its = transformed.begin(); its != transformed.end(); ++its) { //convert port name to lower case
         *its = (char)tolower(*its);
      }

      if ((0 < transformed.length()) && (0 != transformed.compare("undefined")) && (0 != transformed.compare("unknown"))) { //If tyhe port name exists and is not "undefined" or "unknown"
         int ret = 0;
         MM::Device* pS;

         // the port property seems correct, so give it a try
         result = MM::CanNotCommunicate;
         // device specific default communication parameters
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_AnswerTimeout, "2000.0");
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_BaudRate, baud_.c_str());
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_DelayBetweenCharsMs, "0.0");
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_Handshaking, "Off");
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_Parity, "None");
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_StopBits, "1");
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), "Verbose", "1");
         pS = GetCoreCallback()->GetDevice(this, port_.c_str());
         pS->Initialize();

         PurgeComPort(port_.c_str());
		 std::string response;
         ret = sendCmd("ID?", response);
         if ((ret != DEVICE_OK) || (response.compare("I am Motorized Aperture") != 0)) {
            LogMessageCode(ret, true);
            LogMessage(std::string("MotorizedAperture not found on ") + port_.c_str(), true);
            LogMessage(std::string("Got response:") + response, true);
            ret = 1;
            pS->Shutdown();
         }
         else { // to succeed must reach here....
            LogMessage(std::string("MotorizedAperture found on ") + port_.c_str(), true);
            result = MM::CanCommunicate;
            pS->Initialize();
            pS->Shutdown();
         }
      }
   }
   catch (...)
   {
      LogMessage("Exception in DetectDevice!", false);
   }
   return result;
}

int MotorizedAperture::Initialize()
{

   // empty the Rx serial buffer before sending command
   int ret = PurgeComPort(port_.c_str());
   if (ret != DEVICE_OK)
      return ret;

   // Name
   ret = CreateProperty(MM::g_Keyword_Name, g_ControllerName, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Speed
   CPropertyAction* pAct = new CPropertyAction(this, &MotorizedAperture::OnSpeed);
   ret = CreateProperty("Speed", "0", MM::Float, false, pAct);
   if (ret != DEVICE_OK)
      return ret;
   SetPropertyLimits("Speed", 0, 100);

   //Position
   pAct = new CPropertyAction(this, &MotorizedAperture::OnPosition);
   ret = CreateProperty("Position", "0", MM::Integer, false, pAct);
   if (ret != DEVICE_OK)
      return ret;
   SetPropertyLimits("Position", 0, 510);

   //Acceleration
   pAct = new CPropertyAction(this, &MotorizedAperture::OnAccel);
   ret = CreateProperty("Accel", "0", MM::Float, false, pAct);
   if (ret != DEVICE_OK)
      return ret;
   SetPropertyLimits("Accel", 0, 500);

   //Home
   pAct = new CPropertyAction(this, &MotorizedAperture::OnHome);
   ret = CreateProperty("Home", "", MM::String, false, pAct);
   AddAllowedValue("Home","");
   AddAllowedValue("Home","Run");
   if (ret != DEVICE_OK) { return ret; }

    //Status
   pAct = new CPropertyAction(this, &MotorizedAperture::OnStatus);
   ret = CreateProperty("StatusOk", "", MM::String, true, pAct);
   if (ret != DEVICE_OK) { return ret; }

   SetErrorText(99, "Device set busy for ");
   initialized_ = true;
   return DEVICE_OK;
}

int MotorizedAperture::Shutdown() {
   initialized_ = false;
   return DEVICE_OK;
}

//////////////// Action Handlers (MotorizedAperture) /////////////////

int MotorizedAperture::OnPort(MM::PropertyBase* pProp, MM::ActionType pAct)
{
   if (pAct == MM::BeforeGet)
   {
      pProp->Set(port_.c_str());
   }
   else if (pAct == MM::AfterSet)
   {
      if (initialized_)
      {
         pProp->Set(port_.c_str());
         return DEVICE_INVALID_INPUT_PARAM;
      }
      pProp->Get(port_);
   }

   return DEVICE_OK;
}

int MotorizedAperture::OnBaud(MM::PropertyBase* pProp, MM::ActionType eAct) {
   if (eAct == MM::BeforeGet) {
      pProp->Set(baud_.c_str());
   }
   else if (eAct == MM::AfterSet) {
      if (initialized_) {
         pProp->Set(baud_.c_str());
         return DEVICE_INVALID_INPUT_PARAM;
      }
      pProp->Get(baud_);
   }
   return DEVICE_OK;
}

int MotorizedAperture::OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct) {
    int ret;
    switch (eAct) {
       case (MM::BeforeGet): {
          std::string ans;
          ret = sendCmd("A?", ans);
          if (ret != DEVICE_OK) { return ret; }
          int number = std::stoi(ans);
		  long retVal = int(number);
          pProp->Set(retVal);
          if (ret != DEVICE_OK) { return ret; }
          break;
       }
       case (MM::AfterSet): {
		   if (!status_) {
			   SetErrorText(99, "The MotorizedAperture firmware reports that its status is not ok. no movement will be permitted until this is fixed.");
			   return 99;
		   }
          long position;
          // read value from property
          pProp->Get(position);
          // write speed out to device....
          std::ostringstream cmd;
          cmd << "A" << position;
          ret = sendCmd(cmd.str());
          if (ret != DEVICE_OK)
             return ret;
          break;
       }
   }
   return DEVICE_OK;
 }

 int MotorizedAperture::OnSpeed(MM::PropertyBase* pProp, MM::ActionType eAct) {
    int ret;
    switch (eAct) {
       case (MM::BeforeGet): {
          std::string ans;
          ret = sendCmd("S?", ans);
          if (ret != DEVICE_OK) { return ret; }
          double number = std::stod(ans);
          pProp->Set(number);
          break;
       }
       case (MM::AfterSet): {
          double speed;
          // read value from property
          pProp->Get(speed);
          // write speed out to device....
          std::ostringstream cmd;
          cmd.setf(std::ios::fixed, std::ios::floatfield);
          cmd.precision(3);
          cmd << "S" << speed;
          ret = sendCmd(cmd.str());
          if (ret != DEVICE_OK)
             return ret;
          break;
       }
   }
   return DEVICE_OK;
 }

int MotorizedAperture::OnAccel(MM::PropertyBase* pProp, MM::ActionType eAct) {
    int ret;
    switch (eAct) {
       case (MM::BeforeGet): {
		  std::string ans;
          ret = sendCmd("a?", ans);
          if (ret != DEVICE_OK) { return ret; }
          double number = std::stod(ans);
          pProp->Set(number);
          break;
       }
       case (MM::AfterSet): {
          double accel;
          // read value from property
          pProp->Get(accel);
          // write speed out to device....
          std::ostringstream cmd;
          cmd.setf(std::ios::fixed, std::ios::floatfield);
          cmd.precision(3);
          cmd << "a" << accel;
          ret = sendCmd(cmd.str());
          if (ret != DEVICE_OK)
             return ret;
          break;
       }
   }
   return DEVICE_OK;
 }
 
int MotorizedAperture::OnHome(MM::PropertyBase* pProp, MM::ActionType eAct) {
	switch (eAct) {
	case (MM::AfterSet): {
		std::string setting;
		pProp->Get(setting);
		if (setting.compare("Run")==0) {
			std::ostringstream cmd;
			cmd << "H";
			int ret = sendCmd(cmd.str());
			if (ret != DEVICE_OK)
				return ret;
			break;
		}
	}
	case (MM::BeforeGet): {
		pProp->Set("");
		break;
	}
	}
	return DEVICE_OK;
}

int MotorizedAperture::OnStatus(MM::PropertyBase* pProp, MM::ActionType eAct) {
	switch (eAct) {
    case (MM::BeforeGet): {
		std::string ans;
        int ret = sendCmd("s?", ans);
        if (ret != DEVICE_OK) { return ret; }
        int number = std::stoi(ans);
		if (number != 0) {
			pProp->Set("True");
			status_ = true;
		} else {
			pProp->Set("False");
			status_ = false;
		}
        break;
    }
	}
	return DEVICE_OK;
}

bool MotorizedAperture::Busy() {
	std::string ans;
	int ret = sendCmd("B?", ans);
	if (ret != DEVICE_OK) {return true;}
	if (strcmp("1", ans.c_str()) == 0) {
		return true;
	} else {
		return false;
	}
}


int MotorizedAperture::sendCmd(std::string cmd, std::string& out) {

   PurgeComPort(port_.c_str());
   int ret = SendSerialCommand(port_.c_str(), cmd.c_str(), "\r");
   if (ret != DEVICE_OK) {
      return DEVICE_SERIAL_COMMAND_FAILED;
   }
   std::string response;
   GetSerialAnswer(port_.c_str(), "\r", response);   //Read back the response and make sure it matches what we sent. If not there is an issue with communication.
   if (response != cmd) {
	  std::ostringstream err;
	  err << "Motorized aperture expected response: '";
	  err << cmd;
	  err << "' But got: '";
	  err << response;
	  err << "'";
	  SetErrorText(99, err.str().c_str());
      return 99;
   }
   GetSerialAnswer(port_.c_str(), "\r", out); //Try returning any extra response from the device. Even if there is no response we still need to read out the \r
   return DEVICE_OK;
}


/* *********Steps to NA conversion******************
Note: ~= is used here to denote proportionality
Initial formulas:
	Magnification = TubeFocalLength / ObjectionFocalLength ---- M = F / f
	ApertureDiameter = 2 * NA * f ------- D = 2 * NA * f
	Put this together to get: NA = (D * M) / (2 * F)

Geometry of motorized aperture:
	x - x_0 = xPerSteps * (steps - steps_0)
	x = r * cos(theta) //r = radius from center of aperture to rotator knob. theta angle of rotation from horizontal.
	theta_0 is the angle at which the aperture is closed, NA=0. there is a corresponding x_0 = r * cos(theta_0).
	theta > theta_0, x > x_0

	Assume:
	D ~= theta - theta_0

	put this together to get:
	theta = r*arcos(x)
	theta_0 = r*arcos(x_0)
	D = r * (arcos(x) - arcos(x_0))
	
	Final Equation:
		NA ~= (arcos(C*(steps-steps_0)) - arcos(C*steps_0)) * M



