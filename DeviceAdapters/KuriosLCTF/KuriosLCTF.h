///////////////////////////////////////////////////////////////////////////////
// FILE:          KuriosLCTF.h
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

#pragma once

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include "KURIOS_COMMAND_LIB.h"
#include <string>
#include <stdint.h>


const char* g_LCTFName = "Kurios LCTF";

class KuriosLCTF: public CGenericBase<KuriosLCTF> {
public:
	KuriosLCTF();
	~KuriosLCTF();

	//Device API
	int Initialize();
	int Shutdown();
	void GetName(char* pName) const;
	bool Busy(){return false;};

	//Properties
	int onPort(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onOutputMode(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onBandwidthMode(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onSeqTimeInterval(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onSequenceBandwidthMode(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onStatus(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onTemperature(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onTriggerOutMode(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onForceTrigger(MM::PropertyBase* pProp, MM::ActionType eAct);
private:
	int portHandle_;
	std::string port_;
	enum SpectralRange {vis=1, nir=2, ir=4};
	enum Bandwidth {black=1, wide=2, medium=4, narrow=8, superNarrow=16};
	long defaultIntervalMs_;
	int defaultBandwidth_;
	std::string origOutputMode_;
};