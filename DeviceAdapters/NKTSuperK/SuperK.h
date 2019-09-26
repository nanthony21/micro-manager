// NKT_SuperK.h

#pragma once

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include "x64NKTSDK\NKTPDLL.h"
#include <string>
#include <map>
#include <stdint.h>

class SuperKHub: public HubBase<SuperKHub> {
public:
	SuperKHub();
	~SuperKHub();
	//Device API
	int Initialize();
	int Shutdown();
	void GetName(char* pName) const {}; //TODO implement
	bool Busy() {return false;}; //TODO implement
	bool SupportsDeviceDetection() { return false; };
	//MM::DeviceDetectionStatus DetectDevice();
	//Hub API
	int DetectInstalledDevices();
	//Properties
	int onPort(MM::PropertyBase* pProp, MM::ActionType eAct);
	std::string getPort();
private:
	std::string port_;
};

class SuperKDevice{
private: 
	uint8_t address_;
public:
	SuperKDevice();
	virtual void setNKTAddress(uint8_t address);
	virtual uint8_t getNKTAddress();
};

class SuperKExtreme: public CGenericBase<SuperKExtreme>, public SuperKDevice {
public:
	SuperKExtreme();
	~SuperKExtreme();

	//Device API
	int Initialize();
	int Shutdown();
	void GetName(char* pName) const;
	bool Busy(){return false;};
	
	//Shutter API
	//int SetOpen(bool open = true);
	//int GetOpen(bool& open);
	//int Fire(double deltaT);

	//Properties
	int onEmission(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onPower(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onInletTemperature(MM::PropertyBase* pProp, MM::ActionType eAct);
private:
	std::string name_;
	SuperKHub* hub_;
};

/*
class SuperKVaria: public SuperKDevice {
public:
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
	SuperKHub hub_;
};*/