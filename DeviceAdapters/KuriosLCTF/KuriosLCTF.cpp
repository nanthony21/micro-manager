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

	//TODO initialize properties. populate readonly properties.
		id ro
	spec ro
	opticalheadtype ro

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
