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
	CreateProperty("Wavelength", "500", MM::Integer, false, pAct, false);
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
int KuriosLCTF::onPort(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		pProp->Set(port_.c_str());
	}
	else if (eAct == MM::AfterSet) {
		pProp->Get(port_);
	}
	return DEVICE_OK;
}

int KuriosLCTF::onOutputMode(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int mode;
		kurios_Get_OutputMode(portHandle_, &mode);
		if (mode==1) {
			pProp->Set("Manual");
		} else if (mode==2) {
			pProp->Set("Sequence (internal clock)");
		} else if (mode==3) {
			pProp->Set("Sequence (external trig)");
		} else if (mode==4) {
			pProp->Set("Analog (internal clock)");
		} else if (mode==5) {
			pProp->Set("Analog (external trig)");
		}
	}
	else if (eAct == MM::AfterSet) {
		std::string msg;
		pProp->Get(msg);
		const char* m = msg.c_str();
		int mode;
		if (strcmp(m, "Manual")==0) {
			mode=1;
		} else if (strcmp(m, "equence (internal clock)")==0) {
			mode=2;
		} else if (strcmp(m, "Sequence (external trig)")==0) {
			mode=3;
		} else if (strcmp(m, "Analog (internal clock)")==0) {
			mode=4;
		} else if (strcmp(m, "Analog (external trig)")==0) {
			mode=5;
		} else {
			return DEVICE_ERR;
		}
		kurios_Set_OutputMode(portHandle_, mode);
	}
	return DEVICE_OK;
}

int KuriosLCTF::onBandwidthMode(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int bandwidth;
		kurios_Get_BandwidthMode(portHandle_, &bandwidth);
		if (bandwidth & Bandwidth::black) {
			pProp->Set("Black");
		} else if (bandwidth & Bandwidth::wide) {
			pProp->Set("Wide");
		} else if (bandwidth & Bandwidth::medium) {
			pProp->Set("Medium");
		} else if (bandwidth & Bandwidth::narrow) {
			pProp->Set("Narrow");
		} else if (bandwidth & Bandwidth::superNarrow) {
			pProp->Set("Super Narrow");
		}		
	}
	else if (eAct == MM::AfterSet) {
		std::string setting;
		int bandwidth;
		pProp->Get(setting);
		const char* bw = setting.c_str();
		if (strcmp(bw, "Black")==0) {
			bandwidth = Bandwidth::black;
		} else if (strcmp(bw, "Wide")==0) {
			bandwidth = Bandwidth::wide;
		} else if (strcmp(bw, "Medium")==0) {
			bandwidth = Bandwidth::medium;
		} else if (strcmp(bw, "Narrow")==0) {
			bandwidth = Bandwidth::narrow;
		} else if (strcmp(bw, "Super Narrow")==0) {
			bandwidth = Bandwidth::superNarrow;
		}
		kurios_Set_BandwidthMode(portHandle_, bandwidth);
	}
	return DEVICE_OK;
}

/*todo implementint KuriosLCTF::onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {

	}
	else if (eAct == MM::AfterSet) {

	}
	else if (eAct == MM::IsSequenceable) {
        pProp->SetSequenceable(1024);   //I have no idea how many steps the device actually supports.
    }
    else if (eAct == MM::StartSequence) {

    }
    else if (eAct == MM::StopSequence) {

    }
    else if (eAct == MM::AfterLoadSequence) { 
        std::vector<std::string> sequence =  pProp->GetSequence();
		kurios_Set_DeleteSequenceStep(portHandle_, 0); //Using a value of 0 here deletes the whole sequence.
		for (int i=0; i<sequence.size(); i++) {
			std::string step = sequence[i];
			kurios_Set_InsertSequenceStep(portHandle_, 0, std::atoi(step.c_str()), defaultIntervalMs_, defaultBandwidth_);
		}
    }
	return DEVICE_OK;
}*/

int KuriosLCTF::onSeqTimeInterval(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		kurios_Get_DefaultTimeIntervalForSequence(portHandle_, ((int*) &defaultIntervalMs_));
		pProp->Set(defaultIntervalMs_);
	}
	else if (eAct == MM::AfterSet) {
		pProp->Get(defaultIntervalMs_);
		kurios_Set_DefaultTimeIntervalForSequence(portHandle_, defaultIntervalMs_);
	}
	return DEVICE_OK;
}


int KuriosLCTF::onSequenceBandwidthMode(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		kurios_Get_DefaultBandwidthForSequence(portHandle_, &defaultBandwidth_);
		if (defaultBandwidth_ & Bandwidth::wide) {
			pProp->Set("Wide");
		} else if (defaultBandwidth_ & Bandwidth::medium) {
			pProp->Set("Medium");
		} else if (defaultBandwidth_ & Bandwidth::narrow) {
			pProp->Set("Narrow");
		} else if (defaultBandwidth_ & Bandwidth::superNarrow) {
			pProp->Set("Super Narrow");
		}
	}
	else if (eAct == MM::AfterSet) {
		std::string setting;
		pProp->Get(setting);
		const char* bw = setting.c_str();
		if (strcmp(bw, "Wide")==0) {
			defaultBandwidth_ = Bandwidth::wide;
		} else if (strcmp(bw, "Medium")==0) {
			defaultBandwidth_ = Bandwidth::medium;
		} else if (strcmp(bw, "Narrow")==0) {
			defaultBandwidth_ = Bandwidth::narrow;
		} else if (strcmp(bw, "Super Narrow")==0) {
			defaultBandwidth_ = Bandwidth::superNarrow;
		}
		kurios_Set_DefaultBandwidthForSequence(portHandle_, defaultBandwidth_);
	}
	return DEVICE_OK;
}

int KuriosLCTF::onStatus(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int status;
		kurios_Get_Status(portHandle_, &status);
		if (status==0) {
			pProp->Set("Initialization");
		} else if (status==1) {
			pProp->Set("Warm Up");
		} else if (status==2) {
			pProp->Set("Ready");
		}
	}
	return DEVICE_OK;
}

int KuriosLCTF::onTemperature(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		double temp;
		kurios_Get_Temperature(portHandle_, &temp);
		pProp->Set(temp);
	}
	return DEVICE_OK;
}

int KuriosLCTF::onTriggerOutMode(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int polarity;
		kurios_Get_TriggerOutSignalMode(portHandle_, &polarity);
		if (polarity==0) {
			pProp->Set("Normal");
		} else if (polarity == 1) {
			pProp->Set("Flipped");
		}
	}
	else if (eAct == MM::AfterSet) {
		std::string msg;
		pProp->Get(msg);
		const char * polarity = msg.c_str();
		int pol;
		if (strcmp(polarity, "Normal")==0) {
			pol = 0;
		} else if (strcmp(polarity, "Flipped")==0) {
			pol = 1;
		} else {
			return DEVICE_ERR;
		}
		kurios_Set_TriggerOutSignalMode(portHandle_, pol);
	}
	return DEVICE_OK;
}

int KuriosLCTF::onForceTrigger(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		pProp->Set("Idle");
	}
	else if (eAct == MM::AfterSet) {
		std::string msg;
		pProp->Get(msg);
		if (strcmp(msg.c_str(), "Trigger")==0) {
			kurios_Set_ForceTrigger(portHandle_);
		}
	}
	return DEVICE_OK;
}
