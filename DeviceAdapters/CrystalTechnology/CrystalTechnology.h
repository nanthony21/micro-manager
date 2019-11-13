///////////////////////////////////////////////////////////////////////////////
// FILE:          CrystalTechnology.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Crystal Technology AOTF Controllers
// COPYRIGHT:     2019 Backman Biophotonics Lab, Northwestern University
//                All rights reserved.
//
//                This library is free software; you can redistribute it and/or
//                modify it under the terms of the GNU Lesser General Public
//                License as published by the Free Software Foundation.
//
//                This library is distributed in the hope that it will be
//                useful, but WITHOUT ANY WARRANTY; without even the implied
//                warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//                PURPOSE. See the GNU Lesser General Public License for more
//                details.
//
//                IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//                LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//                EXEMPLARY, OR CONSEQUENTIAL DAMAGES.
//
//                You should have received a copy of the GNU Lesser General
//                Public License along with this library; if not, write to the
//                Free Software Foundation, Inc., 51 Franklin Street, Fifth
//                Floor, Boston, MA 02110-1301 USA.
//
// AUTHOR:        Nick Anthony

#pragma once

#include "../../MMDevice/DeviceBase.h"
#include <stdint.h>
#include <functional>


// Device Names
const char* const g_ControllerName = "AOTF Controller";

template <class T>
class CTBase: public CGenericBase<T> {
	//Serves as an abstract base class for micromanager device adapters using the CTDriver class.
public:
	CTBase();
	~CTBase();
	//Device API
	int Initialize();
	int Shutdown();
	virtual void GetName(char* pName) const = 0;
	bool Busy() {return false;}; 
	bool SupportsDeviceDetection() { return false; };

protected:
	CTDriver* driver_;
private:
	//Properties
	int onPort(MM::PropertyBase* pProp, MM::ActionType eAct);
	int tx_(std::string cmd);
	int rx_(std::string& response);
	const char* port_;
};

class CTTunableFilter: public CTBase<CTTunableFilter> {
	//Uses the multiple channels of the RF driver to make a tunable filter that can set it center wavelenght and it bandwidth (by spreading the frequencies of the various channels)
public:
	CTTunableFilter();
	int Initialize();
	void GetName(char* pName) const {strcpy(pName, g_ControllerName);};
private:
	//Properties
	int onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onBandwidth(MM::PropertyBase* pProp, MM::ActionType eAct);
	void setWavelength(double wv);
	void setBandwidth(double bw);
	int updateWvs();
	int wv;
	int wvs[8]; //all 8 may not be used.
	int bw;
};

class CTDriver {
	//This class implements all functionality without any reliance on micromanager specific stuff. It can be wrapped into a device adapter.
	//Not all possible functionality is implemented here. It should be enough for many applications though.
public:
	static const int INVALID_CHANNEL = 2;
	static const int INVALID_VALUE = 3;
	static const int ERR = 1;
	static const int OK = 0;

	CTDriver(std::function<int(std::string)> serialSend, std::function<std::string(void)> serialReceive);

	int reset();
	int numChannels() { return numChan_; };
	//Setters
	int setFrequencyMhz(uint8_t chan,  double freq);
	int setWavelengthNm(uint8_t chan, unsigned int wv);
	int setAmplitude(uint8_t chan,  unsigned int asf);
	int setGain(uint8_t chan, unsigned int gain);
	int setPhase(uint8_t chan, double phaseDegrees);
	//Getters
	int getPhase(uint8_t chan, double& phaseDegrees);
	int getAmplitude(uint8_t chan, unsigned int& asf);
	int getFrequencyMhz(uint8_t chan, double& freq);
	int getWavelengthNm(uint8_t chan, unsigned int& wv); //Does this really need to be int?
	int getAllTemperatures(std::string& temps);
	int getBoardInfo(std::string& info);
	int getTuningCoeff(std::string& coeffs);

	//get # of channels.

	//Unkn
	//calibration -> Identify, Tuning 
private:
	int getChannelStr(uint8_t chan, std::string& str, bool allowWildcard);
	int setFreq(uint8_t chan, std::string freqStr);
	std::function<int(std::string)> tx_; //A function that sends the string over serial and terminates it with \r
	std::function<int(std::string)> rx_; //A function that reads a \r terminated line from serial 
	uint8_t numChan_; //The number of channels that the device has.
}

