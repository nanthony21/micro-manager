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

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include <stdint.h>
#include <functional>


// Device Names
const char* const g_ControllerName = "AOTF Controller";

class CrystalTech: public CGenericBase<CrystalTech> {
public:
	CrystalTech();
	~CrystalTech();
	//Device API
	int Initialize();
	int Shutdown();
	void GetName(char* pName) const {strcpy(pName, g_ControllerName);};
	bool Busy() {return false;}; 
	bool SupportsDeviceDetection() { return false; };
	//Properties
	int onPort(MM::PropertyBase* pProp, MM::ActionType eAct);
	setChannel
		view the temperatures
		set amplitude
		set gain
		set frequency
		set phase
		set calibration
		get boardid
binding:     
	{
        using namespace std::placeholders;
        this->mds_ = new MDS(8, std::bind(&AOTF::sendSerial, this, _1), std::bind(&AOTF::retrieveSerial, this));
    }
private:
	int sendCmd(const char* cmd);
	int sendCmd(const char* cmd, std::string response);
	const char* port_;
}

class CTDriver {
#define CT_INVALID_CHANNEL 2
#define CT_INVALID_VALUE 3
#define CT_ERR 1
#define CT_OK 0
	//This class implements all functionality without any reliance on micromanager specific stuff. It can be wrapped into a device adapter.
public:
	CTDriver(uint8_t numChannels, std::function<int(std::string)> serialSend, std::function<std::string(void)> serialReceive);

	//Setters
	int setFrequencyMhz(uint8_t chan,  double freq);
	int setWavelengthNm(uint8_t chan, unsigned int wv);
	int setAmplitude(uint8_t chan,  unsigned int asf);
	int setGain(uint8_t chan, unsigned int gain);
	int setPhase(uint8_t chan, double phaseDegrees);
	//Getters
	int getPhase(uint8_t chan, double& phaseDegrees);
	int getAmplitude(uint8_t chan, unsigned int& asf);
	//int getFrequencyMhz
	//int getWavelengthNm
	/*
	
	temperature -> Read {C/O/A}//readonly 3 sensors
	boardid -> Version, PartNumber, Identify, ModelNumber, CTISerialNumber Date, Options
	*/
	//Unkn
	//calibration -> Identify, Tuning 
private:
	int getChannelStr(uint8_t chan, std::string& str, bool allowWildcard);
	int setFreq(uint8_t chan, std::string freqStr);
	std::function<int(std::string)> tx_;
	std::function<int(std::string)> rx_;
	uint8_t numChan_;
}

	CTDriver::CTDriver(uint8_t numChannels, std::function<int(std::string)> serialSend, std::function<std::string(void)> serialReceive) {
		tx_ = serialSend;
		rx_ = serialReceive;
		numChan_ = numChannels;
	}

	int CTDriver::setFrequencyMhz(uint8_t chan, double freq) {
		std::string freqStr = std::to_string((long double) freq); //formatted without any weird characters
		return this->setFreq(chan, freqStr);
	}

	int CTDriver::setWavelengthNm(uint8_t chan, unsigned int wv) {
		std::string freqStr = std::to_string((unsigned long long) wv); 
		freqStr = "#" + freqStr; //add the wavelength command modifier
		return this->setFreq(chan, freqStr);
	}

	int CTDriver::setFreq(uint8_t chan, std::string freqStr) {
		std::string chanStr;
		int ret = this->getChannelStr(chan, chanStr, true);
		if (ret!=CT_OK) { return ret; }
		std::string str = "dds frequency " + chanStr + " " + freqStr;
		return this->tx_(str);
	}

	int CTDriver::getChannelStr(uint8_t chan, std::string& chanStr, bool allowWildcard) {
		if ((chan==255) && (allowWildcard)) {
			chanStr = "*";
			return CT_OK;
		} else if (chan >= numChan_) {
			return CT_INVALID_CHANNEL;
		}
		chanStr = std::to_string((unsigned long long) chan); //This weird datatype is required to satisfy VC2010 implementation of to_string
		return CT_OK;
	}

	int CTDriver::setAmplitude(uint8_t chan,  unsigned int asf) {
		if (asf > 16383) { return CT_INVALID_VALUE; }
		std::string chanStr;
		int ret = this->getChannelStr(chan, chanStr, true);
		if (ret!=CT_OK) { return ret; }
		std::string cmd = "dds amplitude " + chanStr + " " + std::to_string((unsigned long long) asf);
		return this->tx_(cmd);
	}

	int CTDriver::setGain(uint8_t chan,  unsigned int gain) {
		if (gain > 31) { return CT_INVALID_VALUE; }
		std::string chanStr;
		int ret = this->getChannelStr(chan, chanStr, true);
		if (ret!=CT_OK) { return ret; }
		std::string cmd = "dds gain " + chanStr + " " + std::to_string((unsigned long long) gain);
		return this->tx_(cmd);
	}

	int CTDriver::setPhase(uint8_t chan, double phaseDegrees) {
		if ((phaseDegrees > 360.0) || (phaseDegrees < 0.0)) { return CT_INVALID_VALUE; }
		std::string chanStr;
		int ret = this->getChannelStr(chan, chanStr, true);
		if (ret != CT_OK) { return ret; }
		unsigned int val = (unsigned int) ((phaseDegrees / 360.0) * 16383);
		std::string cmd = "dds phase " + chanStr + " " + std::to_string((unsigned long long) val);
		return this->tx_(cmd);
	}

	int CTDriver::getPhase(uint8_t chan, double& phaseDegrees) {
		std::string chanStr;
		int ret = this->getChannelStr(chan, chanStr, false);
		if (ret!=CT_OK) { return ret; }
		std::string cmd = "dds phase " + chanStr;
		ret = this->tx_(cmd);
		if (ret!=CT_OK) { return ret; }
		std::string response;
		ret = this->rx_(response)
		if (ret!=CT_OK) { return ret; }
		//TODO parse the response and set phaseDegrees
		return CT_OK;
	}

	int CTDriver::getAmplitude(uint8_t chan, unsigned int& asf) {
		std::string chanStr;
		int ret = this->getChannelStr(chan, chanStr, false);
		if (ret!=CT_OK) { return ret; }
		std::string cmd = "dds amplitude " + chanStr;
		ret = this->tx_(cmd);
		if (ret!=CT_OK) { return ret; }
		std::string response;
		ret = this->rx_(response)
		if (ret!=CT_OK) { return ret; }
		//TODO parse the response and set asf
		return CT_OK;
	}