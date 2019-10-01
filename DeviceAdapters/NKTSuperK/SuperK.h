// NKT_SuperK.h

#pragma once

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include "x64NKTSDK\NKTPDLL.h"
#include <string>
#include <map>
#include <stdint.h>

//Offsets so we can initialize NKT specific errors without overwriting the default errors. The offset is added to the value of the error when it is generated. We also register error messages with MMCore using the offset.
#define PortResultsOffset 50 //0-4
#define DeviceResultsOffset 55 //0-6
#define RegisterResultsOffset 62 //0-15


class SuperKDevice{
private: 
	uint8_t address_;
	uint8_t type_;
public:
	SuperKDevice(uint8_t nktDevType);
	void setNKTAddress(uint8_t address);
	uint8_t getNKTAddress();
	uint8_t getNKTType();
};

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
	//Hub API
	int DetectInstalledDevices();
	//Properties
	int onPort(MM::PropertyBase* pProp, MM::ActionType eAct);
	//Hub utility functions
	std::string getPort();
	uint8_t getDeviceAddress(SuperKDevice* devPtr);
	int registerReadU8(SuperKDevice* dev, uint8_t regId, uint8_t* val);
	int registerWriteU8(SuperKDevice* dev, uint8_t regId, uint8_t val);
	int registerReadU16(SuperKDevice* dev, uint8_t regId, uint16_t* val);
	int registerWriteU16(SuperKDevice* dev, uint8_t regId, uint16_t val);
	int registerReadS16(SuperKDevice* dev, uint8_t regId, int16_t* val);
	int deviceGetStatusBits(SuperKDevice* dev, unsigned long* val);
private:
	std::string port_;
	std::map<uint8_t, uint8_t> deviceAddressMap_;
	MMThreadLock lock_; //Make comms thread safe
	MMThreadLock& getLock() { return lock_; };
	int populateDeviceAddressMap();
	void setNKTErrorText();
};



class SuperKExtreme: public CShutterBase<SuperKExtreme>, public SuperKDevice {
public:
	SuperKExtreme();
	~SuperKExtreme();

	//Device API
	int Initialize();
	int Shutdown();
	void GetName(char* pName) const;
	bool Busy(){return false;};
	
	//Shutter API TODO implement
	int SetOpen(bool open = true);
	int GetOpen(bool& open);
	int Fire(double deltaT);

	//Properties
	int onEmission(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onPower(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onInletTemperature(MM::PropertyBase* pProp, MM::ActionType eAct);
private:
	std::string name_;
	SuperKHub* hub_;
	MM::MMTime emissionChangedTime_;
	bool emissionOn_;
};


class SuperKVaria: public CGenericBase<SuperKVaria>, public SuperKDevice {
public:
	SuperKVaria();
	~SuperKVaria();

	//Device API
	int Initialize();
	int Shutdown();
	void GetName(char* pName) const;
	bool Busy();//TODO implement

	//Properties
	int onMonitorInput(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onBandwidth(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onLWP(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onSWP(MM::PropertyBase* pProp, MM::ActionType eAct);
private:
	int updateFilters(); //Use the wavelength and bandwidth settings to set the swp and lwp filters.
	std::string name_;
	SuperKHub* hub_;
	double swp_;
	double lwp_;
	double wavelength_;
	double bandwidth_;
};