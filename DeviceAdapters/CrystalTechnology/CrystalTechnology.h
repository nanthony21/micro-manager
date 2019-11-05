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
	dds
	calibration
	temperature
	modulation?
	chirp?
	boardid
private:
	int sendCmd(const char* cmd);
	int sendCmd(const char* cmd, std::string response);
	const char* port_;
}