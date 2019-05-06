///////////////////////////////////////////////////////////////////////////////
// FILE:          MotorizedAperture.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Motorized Aperture build by Nick Anthony for Nikon TI microscopes.
//
//
// AUTHOR:        Nick Anthony, Backman Biophotonics Lab, Northwestern University
// COPYRIGHT:     
// LICENSE:       
//

#ifndef _MOTORIZED_APERTURE_
#define _MOTORIZED_APERTURE_


#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"

#include <string>
#include <map>

//////////////////////////////////////////////////////////////////////////////
#define ERR_PORT_CHANGE_FORBIDDEN 109

class MotorizedAperture : public CGenericBase<MotorizedAperture>
{
public:
	MotorizedAperture();
	~MotorizedAperture();

	// Device API
	// ---------
	int Initialize();
	int Shutdown();
	void GetName(char* pszName) const;
	bool Busy();
	
	int DeInitialize() {initialized_ = false; return DEVICE_OK;};
	bool Initialized() {return initialized_;};
	  
	// device discovery
	bool SupportsDeviceDetection(void);
	MM::DeviceDetectionStatus DetectDevice(void);

	// action interface
	// ---------------
	int OnPort    (MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnBaud	(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnPosition (MM::PropertyBase* pProp, MM::ActionType eAct);  
	int OnSpeed (MM::PropertyBase* pProp, MM::ActionType eAct); 
	int OnAccel (MM::PropertyBase* pProp, MM::ActionType eAct); 
	  
private:
	std::string port_;
	std::string baud_;
	bool initialized_;
	bool initializedDelay_;
	double answerTimeoutMs_;
	double position_; // the cached values
	double speed_;
	double accel_;
	MM::MMTime changedTime_;
	MM::MMTime delay_;
	std::vector<double> sequence_;

	int sendCmd(std::string cmd, std::string& out);	//Send a command and save the response in `out`.
	int sendCmd(std::string cmd);	//Send a command that does not repond with any extra information.
	int getStatus();
	bool reportsBusy();
};

#endif //_MOTORIZED_APERTURE_
