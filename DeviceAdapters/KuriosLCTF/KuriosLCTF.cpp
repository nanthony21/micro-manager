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


	//Find ports and add them aa property options
	unsigned char ports[1024];
	common_List(ports);
	std::string portStr = std::string((const char*)ports);
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
	portHandle_ = common_Open((char*) port_.c_str(), 115200, 1); //TODO is the baud right?
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
	const char* specRange;
	if (spectrumRange == SR::VIS) {
		specRange = SRNames.VIS;
	} else if (spectrumRange == SR::NIR) {
		specRange = SRNames.NIR;
	} else if (spectrumRange == SR::IR) {
		specRange = SRNames.IR;
	} else {
		specRange = "Unkn";
	}
	CPropertyAction* pAct = new CPropertyAction(this, &KuriosLCTF::onBandwidthMode);
	CreateProperty("Bandwidth", BWNames.WIDE, MM::String, false, pAct, false);
	pAct = new CPropertyAction(this, &KuriosLCTF::onSequenceBandwidthMode);
	CreateProperty("Sequence Bandwidth", BWNames.WIDE, MM::String, false, pAct, false);
	CreateProperty("Spectral Range", specRange, MM::String, true);
	std::string bw = "";
	if (availableBandwidthRange & BW::BLACK) {
		bw.append(BWNames.BLACK);
		AddAllowedValue("Bandwidth", BWNames.BLACK);
	} if (availableBandwidthRange & BW::WIDE) {
		bw.append(" Wide");
		AddAllowedValue("Bandwidth", BWNames.WIDE);
		AddAllowedValue("Sequence Bandwidth", BWNames.WIDE);
	} if (availableBandwidthRange & BW::MEDIUM) {
		bw.append(" Medium");
		AddAllowedValue("Bandwidth", BWNames.MEDIUM);
		AddAllowedValue("Sequence Bandwidth", BWNames.MEDIUM);
	} if (availableBandwidthRange & BW::NARROW) {
		bw.append(" Narrow");
		AddAllowedValue("Bandwidth", BWNames.NARROW);
		AddAllowedValue("Sequence Bandwidth", BWNames.NARROW);
	} if (availableBandwidthRange & BW::SUPERNARROW) {
		bw.append(" SuperNarrow");
		AddAllowedValue("Bandwidth", BWNames.SUPERNARROW);
		AddAllowedValue("Sequence Bandwidth", BWNames.SUPERNARROW);
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
	return DEVICE_OK;
}


void KuriosLCTF::GetName(char* name) const {
	assert(strlen(g_LCTFName) < CDeviceUtils::GetMaxStringLength());
	CDeviceUtils::CopyLimitedString(name, g_LCTFName);
}


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
		int ret = kurios_Get_OutputMode(portHandle_, &mode);
		if (ret<0){ return DEVICE_ERR; }
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
		} else if (strcmp(m, "Sequence (internal clock)")==0) {
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
		int ret = kurios_Set_OutputMode(portHandle_, mode);
		if (ret<0){ return DEVICE_ERR; }
	}
	return DEVICE_OK;
}

int KuriosLCTF::onBandwidthMode(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int bandwidth;
		int ret = kurios_Get_BandwidthMode(portHandle_, &bandwidth);
		if (ret<0){ return DEVICE_ERR; }
		if (bandwidth & BW::BLACK) {
			pProp->Set(BWNames.BLACK);
		} else if (bandwidth & BW::WIDE) {
			pProp->Set(BWNames.WIDE);
		} else if (bandwidth & BW::MEDIUM) {
			pProp->Set(BWNames.MEDIUM);
		} else if (bandwidth & BW::NARROW) {
			pProp->Set(BWNames.NARROW);
		} else if (bandwidth & BW::SUPERNARROW) {
			pProp->Set(BWNames.SUPERNARROW);
		}		
	}
	else if (eAct == MM::AfterSet) {
		std::string setting;
		int bandwidth;
		pProp->Get(setting);
		const char* bw = setting.c_str();
		if (strcmp(bw, BWNames.BLACK)==0) {
			bandwidth = BW::BLACK;
		} else if (strcmp(bw, BWNames.WIDE)==0) {
			bandwidth = BW::WIDE;
		} else if (strcmp(bw, BWNames.MEDIUM)==0) {
			bandwidth = BW::MEDIUM;
		} else if (strcmp(bw, BWNames.NARROW)==0) {
			bandwidth = BW::NARROW;
		} else if (strcmp(bw, BWNames.SUPERNARROW)==0) {
			bandwidth = BW::SUPERNARROW;
		} else {
			return DEVICE_ERR;
		}
		int ret = kurios_Set_BandwidthMode(portHandle_, bandwidth);
		if (ret<0){ return DEVICE_ERR; }
	}
	return DEVICE_OK;
}

int KuriosLCTF::onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int wv;
		int ret = kurios_Get_Wavelength(portHandle_, &wv);
		if (ret<0) { return ret; }
		pProp->Set((long) wv);
	}
	else if (eAct == MM::AfterSet) {
		long wv;
		pProp->Get(wv);
		int ret = kurios_Set_Wavelength(portHandle_, wv);
		if (ret<0) { return ret; }
	}
	else if (eAct == MM::IsSequenceable) {
        pProp->SetSequenceable(1024);   //According the thorlabs website it can store 1024 items.
    }
    else if (eAct == MM::StartSequence) {
		char* msg;
		int ret = this->GetProperty("Output Mode", msg);
		if (ret!=DEVICE_OK) { return ret;}
		origOutputMode_ = std::string(msg); //save the original output mode we were in. we'll go back to it when the sequence is stopped.
		ret = this->SetProperty("Output Mode", "Sequence (external trig)"); //For now we only support externally triggered sequencing. We would need to add further configuration properties to support the other modes.
		if (ret!=DEVICE_OK) { return ret;}
    }
    else if (eAct == MM::StopSequence) {
		int ret = this->SetProperty("Output Mode", origOutputMode_.c_str());
		if (ret!=DEVICE_OK) { return ret; }
    }
    else if (eAct == MM::AfterLoadSequence) { 
        std::vector<std::string> sequence =  pProp->GetSequence();
		int ret = kurios_Set_DeleteSequenceStep(portHandle_, 0); //Using a value of 0 here deletes the whole sequence.
		if (ret<0){ return DEVICE_ERR; }
		for (int i=0; i<sequence.size(); i++) {
			std::string step = sequence[i];
			ret = kurios_Set_InsertSequenceStep(portHandle_, 0, std::atoi(step.c_str()), defaultIntervalMs_, defaultBandwidth_);
			if (ret<0){ return DEVICE_ERR; }
		}
    }
	return DEVICE_OK;
}

int KuriosLCTF::onSeqTimeInterval(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int ret = kurios_Get_DefaultTimeIntervalForSequence(portHandle_, ((int*) &defaultIntervalMs_));
		if (ret<0){ return DEVICE_ERR; }
		pProp->Set(defaultIntervalMs_);
	}
	else if (eAct == MM::AfterSet) {
		pProp->Get(defaultIntervalMs_);
		int ret = kurios_Set_DefaultTimeIntervalForSequence(portHandle_, defaultIntervalMs_);
		if (ret<0){ return DEVICE_ERR; }
	}
	return DEVICE_OK;
}


int KuriosLCTF::onSequenceBandwidthMode(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int ret = kurios_Get_DefaultBandwidthForSequence(portHandle_, &defaultBandwidth_);
		if (ret<0){ return DEVICE_ERR; }
		if (defaultBandwidth_ & BW::WIDE) {
			pProp->Set(BWNames.WIDE);
		} else if (defaultBandwidth_ & BW::MEDIUM) {
			pProp->Set(BWNames.MEDIUM);
		} else if (defaultBandwidth_ & BW::NARROW) {
			pProp->Set(BWNames.NARROW);
		} else if (defaultBandwidth_ & BW::SUPERNARROW) {
			pProp->Set(BWNames.SUPERNARROW);
		}
	}
	else if (eAct == MM::AfterSet) {
		std::string setting;
		pProp->Get(setting);
		const char* bw = setting.c_str();
		if (strcmp(bw, BWNames.WIDE)==0) {
			defaultBandwidth_ = BW::WIDE;
		} else if (strcmp(bw, BWNames.MEDIUM)==0) {
			defaultBandwidth_ = BW::MEDIUM;
		} else if (strcmp(bw, BWNames.NARROW)==0) {
			defaultBandwidth_ = BW::NARROW;
		} else if (strcmp(bw, BWNames.SUPERNARROW)==0) {
			defaultBandwidth_ = BW::SUPERNARROW;
		}
		int ret = kurios_Set_DefaultBandwidthForSequence(portHandle_, defaultBandwidth_);
		if (ret<0){ return DEVICE_ERR; }
	}
	return DEVICE_OK;
}

int KuriosLCTF::onStatus(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int status;
		int ret = kurios_Get_Status(portHandle_, &status);
		if (ret<0){ return DEVICE_ERR; }
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
		int ret = kurios_Get_Temperature(portHandle_, &temp);
		if (ret<0){ return DEVICE_ERR; }
		pProp->Set(temp);
	}
	return DEVICE_OK;
}

int KuriosLCTF::onTriggerOutMode(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int polarity;
		int ret = kurios_Get_TriggerOutSignalMode(portHandle_, &polarity);
		if (ret<0){ return DEVICE_ERR; }
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
		int ret = kurios_Set_TriggerOutSignalMode(portHandle_, pol);
		if (ret<0){ return DEVICE_ERR; }
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
			int ret = kurios_Set_ForceTrigger(portHandle_);
			if (ret<0){ return DEVICE_ERR; }
		}
	}
	return DEVICE_OK;
}
