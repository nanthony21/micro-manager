///////////////////////////////////////////////////////////////////////////////
// FILE:          MotorizedAperture.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   MotorizedAperture 
//
// AUTHOR:        Nick Anthony, BPL, 2019 driver for lab-build motorized aperture for nikon TI microscope.


#include "MotorizedAperture.h"
#include <cstdio>
#include <cctype>
#include <string>
#include <math.h>
#include "../../MMDevice/ModuleInterface.h"
#include <sstream>
#include <algorithm> 


const char* g_ControllerName    = "TI Motorized Aperture";

const char* g_Term            = "\r"; //unique termination

const char* g_BaudRate_key        = "Baud Rate";
const char* g_Baud9600            = "9600";

using namespace std;

//Local utility functions.
std::string DoubleToString(double N) {
   ostringstream ss("");
   ss << N;
   return ss.str();
}

std::vector<double> getNumbersFromMessage(std::string MotorizedAperturemessage) {
   std::istringstream variStream(MotorizedAperturemessage);
   std::string prefix;
   double val;
   std::vector<double> values;
   variStream >> prefix;
   while (true) {
      variStream >> val;
      if (!variStream.fail()) {
         values.push_back(val);
      }
      else {
         break;
      }
   }
   return values;
}

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
   initialized_(false),
   initializedDelay_(false),
   answerTimeoutMs_(1000)
{
   InitializeDefaultErrorMessages();
   // pre-initialization properties
   // Port:
   CPropertyAction* pAct = new CPropertyAction(this, &MotorizedAperture::OnPort);
   CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
   SetProperty(MM::g_Keyword_Port, port_.c_str());
   
   pAct = new CPropertyAction(this, &MotorizedAperture::OnBaud);
   CreateProperty(g_BaudRate_key, "Undefined", MM::String, false, pAct, true);

   AddAllowedValue(g_BaudRate_key, g_Baud115200, (long)115200);
   AddAllowedValue(g_BaudRate_key, g_Baud9600, (long)9600);

   EnableDelay();
}


MotorizedAperture::~MotorizedAperture()
{
   Shutdown();
}

void MotorizedAperture::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_ControllerName);
}


bool MotorizedAperture::SupportsDeviceDetection(void)
{
   return true;
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
      for (std::string::iterator its = transformed.begin(); its != transformed.end(); ++its)
      {
         *its = (char)tolower(*its);
      }

      if (0 < transformed.length() && 0 != transformed.compare("undefined") && 0 != transformed.compare("unknown"))
      {
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
         ret = sendCmd("V?", serialnum_);
         if (ret != DEVICE_OK || serialnum_.length() < 5)
         {
            LogMessageCode(ret, true);
            LogMessage(std::string("MotorizedAperture not found on ") + port_.c_str(), true);
            LogMessage(std::string("MotorizedAperture serial no:") + serialnum_, true);
            ret = 1;
            serialnum_ = "0";
            pS->Shutdown();
         }
         else
         {
            // to succeed must reach here....
            LogMessage(std::string("MotorizedAperture found on ") + port_.c_str(), true);
            LogMessage(std::string("MotorizedAperture serial no:") + serialnum_, true);
            result = MM::CanCommunicate;
            GetCoreCallback()->SetSerialProperties(port_.c_str(),
               "600.0",
               baud_.c_str(),
               "0.0",
               "Off",
               "None",
               "1");
            serialnum_ = "0";
            pS->Initialize();
            ret = sendCmd("R1");
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
   SetErrorText(97, "The MotorizedAperture reports that it is not exercised.");
   SetErrorText(98, "The MotorizedAperture reports that it is not initialized.");

   //Configure the com port.
   GetCoreCallback()->SetSerialProperties(port_.c_str(),
      "600.0",
      baud_.c_str(),
      "0.0",
      "Off",
      "None",
      "1");

   // empty the Rx serial buffer before sending command
   int ret = PurgeComPort(port_.c_str());
   if (ret != DEVICE_OK)
      return ret;

   // Name
   ret = CreateProperty(MM::g_Keyword_Name, g_ControllerName, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Version number
   CPropertyAction* pAct = new CPropertyAction(this, &MotorizedAperture::OnSerialNumber);
   ret = CreateProperty("Version Number", "Version Number Not Found", MM::String, true, pAct);
   if (ret != DEVICE_OK)
      return ret;

   //Set MotorizedAperture to Standard Comms mode
   ret = sendCmd("B0",getFromMotorizedAperture_);
   if (ret != DEVICE_OK) {return ret;}
   ret = sendCmd("G0"); //disable the TTL port
   if (ret != DEVICE_OK) {return ret;}

   while (true){
      ret = getStatus();
      if (ret == 98) { //Needs initialization
         LogMessage("MotorizedAperture: Running initialization");
         sendCmd("I1");
         while (reportsBusy()){
            CDeviceUtils::SleepMs(100);
         }
      }
      else if (ret == 97) { //needs exercising
         LogMessage("MotorizedAperture: Running exercise");
         sendCmd("E1");
         while (reportsBusy()){
            CDeviceUtils::SleepMs(100);
         }
      }
      else if (ret != DEVICE_OK) {
         return ret;
      }
      else { //Device is ok
         break;
      }
   }

   // Wavelength
   std::string ans;
   ret = sendCmd("V?", ans);   //The serial number response also contains the tuning range of the device
   std::vector<double> nums = getNumbersFromMessage(ans);   //This will be in the format (revision level, shortest wavelength, longest wavelength, serial number).
   if (ret != DEVICE_OK)
      return ret;
   pAct = new CPropertyAction(this, &MotorizedAperture::OnWavelength);
   ret = CreateProperty("Wavelength", DoubleToString(wavelength_).c_str(), MM::Float, false, pAct);
   if (ret != DEVICE_OK)
      return ret;
   SetPropertyLimits("Wavelength", nums.at(1), nums.at(2));

   // Delay
   pAct = new CPropertyAction(this, &MotorizedAperture::OnDelay);
   ret = CreateProperty("Device Delay (ms.)", "200.0", MM::Float, false, pAct);
   if (ret != DEVICE_OK)
      return ret;
   SetPropertyLimits("Device Delay (ms.)", 0.0, 200.0);

   pAct = new CPropertyAction(this, &MotorizedAperture::OnSendToMotorizedAperture);
   ret = CreateProperty("String send to MotorizedAperture", "", MM::String, false, pAct);
   if (ret != DEVICE_OK) {
      return ret;
   }
   pAct = new CPropertyAction(this, &MotorizedAperture::OnGetFromMotorizedAperture);
   ret = CreateProperty("String from MotorizedAperture", "", MM::String, true, pAct);
   if (ret != DEVICE_OK) {
      return ret;
   }
   SetErrorText(99, "Device set busy for ");
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

 int MotorizedAperture::OnSpeed(MM::PropertyBase* pProp, MM::ActionType eAct) {
    int ret;
    switch (eAct) {
       case (MM::BeforeGet): {
          std::string ans;
          ret = sendCmd("S?", ans);
          if (ret != DEVICE_OK) { return ret; }
          vector<double> numbers = getNumbersFromMessage(ans);
          pProp->Set(numbers[0]);
          if (ret != DEVICE_OK) { return ret; }
          break;
       }
       case (MM::AfterSet): {
          double speed;
          // read value from property
          pProp->Get(speed);
          // write speed out to device....
          ostringstream cmd;
          cmd.setf(ios::fixed,ios::floatfield);
          cmd.precision(3);
          cmd << "S" << speed;
          ret = sendCmd(cmd.str());
          if (ret != DEVICE_OK)
             return ret;
          speed_ = speed;
          break;
       }
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
          vector<double> numbers = getNumbersFromMessage(ans); //TODO make this int
          pProp->Set(numbers[0]);
          if (ret != DEVICE_OK) { return ret; }
          break;
       }
       case (MM::AfterSet): {
          int position;
          // read value from property
          pProp->Get(position);
          // write speed out to device....
          ostringstream cmd;
          cmd << "A" << position;
          ret = sendCmd(cmd.str());
          if (ret != DEVICE_OK)
             return ret;
          position_ = position;
          break;
       }
   }
   return DEVICE_OK;
 }

bool MotorizedAperture::Busy() {
	std::string ans;
	int ret = sendCmd("B?", ans);
	if (ret != DEVICE_OK) {return ret;}
	if (strcmp("1", ans) == 0) {
		return true;
	} else {
		return false;
	}
}

int MotorizedAperture::sendCmd(std::string cmd, std::string& out) {
   int ret = sendCmd(cmd);
   if (ret != DEVICE_OK) {
      return ret;
   }
   GetSerialAnswer(port_.c_str(), "\r", out); //Try returning any extra response from the device.
   return DEVICE_OK;
}

int MotorizedAperture::sendCmd(std::string cmd) {
   int ret = SendSerialCommand(port_.c_str(), cmd.c_str(), "\r");
   if (ret != DEVICE_OK) {
      return DEVICE_SERIAL_COMMAND_FAILED;
   }
   std::string response;
   GetSerialAnswer(port_.c_str(), "\r", response);   //Read back the response and make sure it matches what we sent. If not there is an issue with communication.
   if (response != cmd) {
      SetErrorText(99, "The MotorizedAperture did not respond.");
      return 99;
   }
   return DEVICE_OK;
}

