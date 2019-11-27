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
#include "../../MMCore/CoreUtils.h"
#include "AotfLibrary.h"
#include <stdint.h>
#include <functional>
#include <map>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#define SET_CT_ERRS 

// Device Names
const char* const g_ControllerName = "AOTF Controller";
const char* const g_TFName = "Tunable Filter";


class CTDriver {
	//This class implements all functionality without any reliance on micromanager specific stuff. It can be wrapped into a device adapter.
	//Not all possible functionality is implemented here. It should be enough for many applications though.
private:
	static const int ERR_OFFSET = 80;
public:
	static const int SERIAL_TIMEOUT = 5 + ERR_OFFSET;
	static const int NOT_INITIALIZED = 4 + ERR_OFFSET;
	static const int INVALID_CHANNEL = 2 + ERR_OFFSET;
	static const int INVALID_VALUE = 3 + ERR_OFFSET;
	static const int NOECHO = 6 + ERR_OFFSET;
	static const int ERR = 1 + ERR_OFFSET;
	static const int OK = 0;

	static const enum DriverType { SingleType, QuadType, OctalType };

	CTDriver();
	virtual ~CTDriver() {};
	int initialize();
	int reset();
	int numChannels() { return numChan_; };
	//Setters
	int setFrequencyMhz(uint8_t chan,  double freq);
	int setWavelengthNm(uint8_t chan, double wv);
	int setAmplitude(uint8_t chan,  unsigned int asf);
	int setGain(uint8_t chan, unsigned int gain);
	int setPhase(uint8_t chan, double phaseDegrees);
	int setTuningCoeffs(std::vector<double> coeffs);
	//Getters
	int getPhase(uint8_t chan, double& phaseDegrees);
	int getAmplitude(uint8_t chan, unsigned int& asf);
	int getFrequencyMhz(uint8_t chan, double& freq);
	int getWavelengthNm(uint8_t chan, double& wv); //Does this really need to be int?
	int getTemperature(std::string sensorType, double& temp);
	int getBoardInfo(std::string& info);
	int getTuningCoeffs(std::vector<double>& coeffs);
	//Calculators
	int freqToWavelength(double freq, double& wavelength);
	int wavelengthToFreq(double wavelength, double& freq);
private:
	int getChannelStr(uint8_t chan, std::string& str, bool allowWildcard);
	int setFreq(uint8_t chan, std::string freqStr);
	int sendCmd(std::string cmd); //Sends a command and checks that the response is a correct acknowledgement. assumes there is no return string
	int sendCmd(std::string cmd, std::string& response); //Sends a command and check the correct acknowledgement. returns the return string as `response`.
	virtual int tx(std::string str) = 0; //A function that sends the string over serial and terminates it with \r
	virtual int readUntil(std::string delimiter, std::string& out) = 0; //A function that reads a string from the device up until it hits the delimiter. The delimiter itself is not included in `out`
	virtual void clearPort() = 0;
	uint8_t numChan_; //The number of channels that the device has.
	bool initialized;
};

class AOTFLibCTDriver: public CTDriver {
public:
	AOTFLibCTDriver(uint8_t instance);
	~AOTFLibCTDriver();
private:
	int tx(std::string);
	void clearPort();
	int readUntil(std::string delim, std::string& out);
	HANDLE aotfHandle;
};

#include "CTBase.h" //Since CTBase is a template class we have to define everything in a header

class CTTunableFilter: public CTBase<CShutterBase<CTTunableFilter>, CTTunableFilter>  {
	//Uses a single channel of the RF driver to make a tunable filter that can set the center wavelength
public:
	CTTunableFilter();
	int Initialize();
	void GetName(char* pName) const {strcpy(pName, g_TFName);};
private:
	//Properties
	int onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct);
	void setWavelength(double wv);
	int onFrequency(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onAmplitude(MM::PropertyBase* pProp, MM::ActionType eAct);
	//ShutterAPI
    int SetOpen(bool open = true);
	int GetOpen(bool& open);
	int Fire(double deltaT) { deltaT; return DEVICE_UNSUPPORTED_COMMAND; };
	int updateWvs();
	double freq_;
	unsigned int asf_;
	bool shutterOpen_;
};


//Future device adapters for the same device
//1: Uses the multiple channels of the RF driver to make a tunable filter that can set it center wavelenght and it bandwidth (by spreading the frequencies of the various channels)
//2: Comb: Spreads the different cahnnels out so you can have 2-8 distinct wavelengths at once.