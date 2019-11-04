///////////////////////////////////////////////////////////////////////////////
// FILE:          KuriosLCTF.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:  Thorlabs Kurios LCTF
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

#include "KuriosLCTF.h"

KuriosLCTF::KuriosLCTF() {
	SetErrorText(DEVICE_SERIAL_TIMEOUT, "Serial port timed out without receiving a response.");

	CPropertyAction* pAct = new CPropertyAction(this, &KuriosLCTF::onPort);
	CreateProperty("ComPort", "Undefined", MM::String, false, pAct, true);


	//Find NKT ports and add them a property options.

	unsigned char ports[1024];
	int ret = common_List(ports);
	std::string portStr = std::string(ports);
	size_t pos = 0;
	std::string token;
	std::string delimiter = ",";
	while (true) {
		pos = portStr.find(delimiter);
		token = portStr.substr(0, pos);
		AddAllowedValue("ComPort", token.c_str());
		portStr.erase(0, pos + delimiter.length());
		if (pos == std::string::npos) {break;}
	}
}



KuriosLCTF::~KuriosLCTF() {}

//Device API
int KuriosLCTF::Initialize() {
	portHandle_ = common_Open((char*) port_.c_str(), 57600, 1); //TODO is the baud right?
	if (portHandle_<0){return DEVICE_ERR;}

	//For some reason Micromanager tries accessing properties before initialization. For this reason we don't create properties until after initialization.
	unsigned char* id;
	int ret = kurios_Get_ID(portHandle_, id);
	if (ret<0){return DEVICE_ERR;}
	CreateProperty("ID", (const char*) id, MM::String, true);

	unsigned char spectrumRange;
	unsigned char availableBandwidthRange;
	ret = kurios_Get_OpticalHeadType(portHandle_, &spectrumRange, &availableBandwidthRange); 
	if (ret<0){return DEVICE_ERR;}
	std::string specRange;
	if (spectrumRange == SpectralRange::vis) {
		specRange = "Visible";
	} else if (spectrumRange == SpectralRange::nir) {
		specRange = "Near IR";
	} else if (spectrumRange == SpectralRange::ir) {
		specRange = "Infrared";
	} else {
		specRange = "Unkn";
	}
	CPropertyAction* pAct = new CPropertyAction(this, &KuriosLCTF::onBandwidthMode);
	CreateProperty("Bandwidth", "Wide", MM::String, false, pAct, false);
	pAct = new CPropertyAction(this, &KuriosLCTF::onSequenceBandwidthMode);
	CreateProperty("Sequence Bandwidth", "Wide", MM::String, false, pAct, false);
	CreateProperty("Spectral Range", specRange.c_str(), MM::String, true);
	std::string bw = "";
	if (availableBandwidthRange & Bandwidth::black) {
		bw.append("Black");
		AddAllowedValue("Bandwidth", "Black");
	} if (availableBandwidthRange & Bandwidth::wide) {
		bw.append(" Wide");
		AddAllowedValue("Bandwidth", "Wide");
		AddAllowedValue("Sequence Bandwidth", "Wide");
	} if (availableBandwidthRange & Bandwidth::medium) {
		bw.append(" Medium");
		AddAllowedValue("Bandwidth", "Medium");
		AddAllowedValue("Sequence Bandwidth", "Medium");
	} if (availableBandwidthRange & Bandwidth::narrow) {
		bw.append(" Narrow");
		AddAllowedValue("Bandwidth", "Narrow");
		AddAllowedValue("Sequence Bandwidth", "Narrow");
	} if (availableBandwidthRange & Bandwidth::superNarrow) {
		bw.append(" SuperNarrow");
		AddAllowedValue("Bandwidth", "Super Narrow");
		AddAllowedValue("Sequence Bandwidth", "Super Narrow");
	}
	CreateProperty("Available Bandwidths", bw.c_str(), MM::String, true);

	//Wavelength
	int max;
	int min;
	ret = kurios_Get_Specification(portHandle_, &max, &min);
	if (ret<0) { return DEVICE_ERR; }
	pAct = new CPropertyAction(this, &KuriosLCTF::onWavelength);
	CreateProperty("Wavelength", "500", MM::Float, false, pAct, false);
	SetPropertyLimits("Wavelength", min, max);

	//Output mode
	pAct = new CPropertyAction(this, &KuriosLCTF::onOutputMode);
	CreateProperty("Output Mode", "Manual", MM::String, false, pAct, false);
	AddAllowedValue("Output Mode", "Manual");
	AddAllowedValue("Output Mode", "Sequence (internal clock)");
	AddAllowedValue("Output Mode", "Sequence (external trig)");
	AddAllowedValue("Output Mode", "Analog (internal clock)");
	AddAllowedValue("Output Mode", "Analog (external trig)");

	pAct = new CPropertyAction(this, &KuriosLCTF::onSeqTimeInterval);
	CreateProperty("Sequence Time Interval (ms)", "1000", MM::Integer, false, pAct, false);
	SetPropertyLimits("Sequence Time Interval (ms)", 1, 60000);

	pAct = new CPropertyAction(this, &KuriosLCTF::onStatus);
	CreateProperty("Status", "Initialization", MM::String, true, pAct, false);
	AddAllowedValue("Status", "Initialization");
	AddAllowedValue("Status", "Warm Up");
	AddAllowedValue("Status", "Ready");

	pAct = new CPropertyAction(this, &KuriosLCTF::onTemperature);
	CreateProperty("Temperature", "0", MM::Float, true, pAct, false);

	pAct = new CPropertyAction(this, &KuriosLCTF::onTriggerOutMode);
	CreateProperty("Trigger Out Polarity", "Normal", MM::String, false, pAct, false);
	AddAllowedValue("Trigger Out Polarity", "Normal");
	AddAllowedValue("Trigger Out Polarity", "Flipped");

	pAct = new CPropertyAction(this, &KuriosLCTF::onForceTrigger);
	CreateProperty("Force Trigger", "Idle", MM::String, false, pAct, false);
	AddAllowedValue("Force Trigger", "Idle");
	AddAllowedValue("Force Trigger", "Trigger");

	return DEVICE_OK;
}

int KuriosLCTF::Shutdown() {
	int ret = common_Close(portHandle_);
	if (ret<0) {return DEVICE_ERR;}
}


void KuriosLCTF::GetName(char* name) const {
	assert(strlen(g_LCTFName) < CDeviceUtils::GetMaxStringLength());
	CDeviceUtils::CopyLimitedString(name, g_LCTFName);
}

bool KuriosLCTF::Busy(){return false;};

//Properties
