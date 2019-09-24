// NKT_SuperK.h

#pragma once
#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include <string>
#include <map>

class SuperKHub: public HubBase<SuperKHub> {
	SuperKHub();
	//Device API
	int Initialize();
	int Shutdown();
	bool SupportsDeviceDetection() { return true; };
	MM::DeviceDetectionStatus DetectDevice();
	//Hub API
	int DetectInstalledDevices();
	//Properties
	int onPort(MM::PropertyBase* pProp, MM::ActionType eAct);
}

class SuperKExtreme: public ShutterBase<SuperKExtreme>
{
	SuperKExtreme(uint8 address);

	//Device API
	int Initialize();
	int Shutdown();
	void GetName(char* pName) const;
	bool Busy();
	
	//Shutter API
	int SetOpen(bool open = true);
	int GetOpen(bool& open);
	int Fire(double deltaT);

	//Properties
	int onEmission(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onPower(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onInletTemperature(MM::PropertyBase* pProp, MM::ActionType eAct);
private:
	std::string port_;
};

/*
class SuperKVaria {
	SuperKVaria(uint8 address);

	//Properties
	int onMonitorInput(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onBandwidth(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onLWP(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onSWP(MM::PropertyBase* pProp, MM::ActionType eAct);
private:
	void updateFilters();
	checkStatus();
}*/