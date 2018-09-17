///////////////////////////////////////////////////////////////////////////////
// FILE:          VariLC.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   VariLC Polarization Adapter
//
//
// AUTHOR:        Rudolf Oldenbourg, MBL, w/ Arthur Edelstein and Karl Hoover, UCSF, Sept, Oct 2010
//				  modified Amitabh Verma Apr. 17, 2012
// COPYRIGHT:     
// LICENSE:       
//

#ifndef _VARILC_H_
#define _VARILC_H_


#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"

#include <string>
#include <map>

//////////////////////////////////////////////////////////////////////////////
#define ERR_PORT_CHANGE_FORBIDDEN 109

// MMCore name of serial port
std::string port_;

int ClearPort(MM::Device& device, MM::Core& core, const char* port);

class VariLC : public CGenericBase<VariLC>
{
   public:
      VariLC();
      ~VariLC();

      // Device API
      // ---------
      int Initialize();
      int Shutdown();

      void GetName(char* pszName) const;
      bool Busy();
	  int GetVLCSerialAnswer (const char* portName, const char* term, std::string& ans);
	  int OnDelay (MM::PropertyBase* pProp, MM::ActionType eAct);

//      int Initialize(MM::Device& device, MM::Core& core);
      int DeInitialize() {initialized_ = false; return DEVICE_OK;};
      bool Initialized() {return initialized_;};
	  
	  // device discovery
      bool SupportsDeviceDetection(void);
      MM::DeviceDetectionStatus DetectDevice(void);

      // action interface
      // ---------------
      int OnPort    (MM::PropertyBase* pProp, MM::ActionType eAct);
	  int OnBaud	(MM::PropertyBase* pProp, MM::ActionType eAct);
      int OnBriefMode (MM::PropertyBase* pProp, MM::ActionType eAct);
      int OnWavelength (MM::PropertyBase* pProp, MM::ActionType eAct);
	  int OnSerialNumber (MM::PropertyBase* pProp, MM::ActionType eAct);	  
      int OnSendToVariLC (MM::PropertyBase* pProp, MM::ActionType eAct);
      int OnGetFromVariLC (MM::PropertyBase* pProp, MM::ActionType eAct);

	  //sequence interface
	  int IsPropertySequenceable(const char* name, bool& isSequenceable) const;
	  int GetPropertySequenceMaxLength(const char* name, long& nrEvents) const;
	  int StartPropertySequence(const char* propertyName);
	  int StopPropertySequence(const char* propertyName);
	  int ClearPropertySequence(const char* propertyName);
	  int AddToPropertySequence(const char* propertyName, const char* value);	//Add one value to the sequence
	  int SendPropertySequence(const char* propertyName);	//Signal that we are done sending sequence values so that the adapter can send the whole sequence to the device
	  
   private:
      // Command exchange with MMCore
      std::string command_;
	  std::string baud_;
      bool initialized_;
	  bool initializedDelay_;
      double answerTimeoutMs_;
	  bool briefModeQ_;
      double wavelength_; // the cached value
	  std::string serialnum_;
      std::string sendToVariLC_;
      std::string getFromVariLC_;
	  MM::MMTime changedTime_;
	  MM::MMTime delay;
	  std::vector<double> sequence_;
      std::vector<double> getNumbersFromMessage(std::string variLCmessage, bool prefixQ);
	  std::string DoubleToString(double N);
	  int sendCmd(std::string cmd, std::string& out);	//Send a command and save the response in `out`.
	  int sendCmd(std::string cmd);	//Send a command that does not repond with any extra information.
};




#endif //_VARILC_H_
