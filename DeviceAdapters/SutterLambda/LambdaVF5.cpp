#include "SutterLambda.h"

LambdaVF5::LambdaVF5(const char* name):
	WheelBase(name, 0, true, "Lambda VF-5 Tunable Filter (Channel A)"),
	mEnabled_(true), 
	wv_(500),
	uSteps_(1),
	tiltSpeed_(3),
	ttlInEnabled_(false),
	ttlOutEnabled_(false)
{};

int LambdaVF5::Initialize(){
	int ret = WheelBase::Initialize();
	if (ret != DEVICE_OK) { return ret;}

	CPropertyAction* pAct = new CPropertyAction(this, &LambdaVF5::onWavelength);
	ret = CreateProperty("Wavelength", "500", MM::Integer, false, pAct);
	SetPropertyLimits("Wavelength", 338, 900);
	if (ret != DEVICE_OK) { return ret; }
	
	pAct = new CPropertyAction(this, &LambdaVF5::onWheelTilt);
	ret = CreateProperty("Wheel Tilt (uSteps)", "100", MM::Integer, false, pAct);
	SetPropertyLimits("Wheel Tilt (uSteps)", 1, 267);
	if (ret != DEVICE_OK) { return ret; }
	
	pAct = new CPropertyAction(this, &LambdaVF5::onTiltSpeed);
	ret = CreateProperty("Tilt Speed", "3", MM::Integer, false, pAct);
	SetPropertyLimits("Tilt Speed", 0, 3);
	if (ret != DEVICE_OK) { return ret; }

	pAct = new CPropertyAction(this, &LambdaVF5::onTTLOut);
	ret = CreateProperty("TTL Out", "Disabled", MM::String, false, pAct);
	AddAllowedValue("TTL Out","Disabled");
	AddAllowedValue("TTL Out","Rising Edge");
	AddAllowedValue("TTL Out","Falling Edge");
	if (ret != DEVICE_OK) { return ret; }

	pAct = new CPropertyAction(this, &LambdaVF5::onTTLIn);
	ret = CreateProperty("TTL In", "Disabled", MM::String, false, pAct);
	AddAllowedValue("TTL In","Disabled");
	AddAllowedValue("TTL In","Rising Edge");
	AddAllowedValue("TTL In","Falling Edge");
	if (ret != DEVICE_OK) { return ret; }

	pAct = new CPropertyAction(this, &LambdaVF5::onSequenceType);
	ret = CreateProperty("Wavelength Sequence Type", "Arbitrary", MM::String, false, pAct);
	AddAllowedValue("Wavelength Sequence Type","Arbitrary");
	AddAllowedValue("Wavelength Sequence Type","Evenly Spaced");
	if (ret != DEVICE_OK) { return ret; }

	return DEVICE_OK;
}

int LambdaVF5::onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		std::vector<unsigned char> cmd;
		std::vector<unsigned char> response;
		cmd.push_back(0xDB);
		int ret = hub_->SetCommand(cmd, cmd, response);
		if (ret != DEVICE_OK) { return ret;}
		if (response.at(0) != 0x01) { //The first byte indicates which channel the response is for. if it isn't 0x01 (channel A) we have a problem.
			return DEVICE_ERR;
		}
		long wv = ((response.at(2) << 8) | (response.at(1)));
		wv = wv & 0x3fff; //The first two bits indicate the current tilt speed of the vf5.
		pProp->Set(wv);
		wv_ = wv;
		return DEVICE_OK;
	}
	else if (eAct == MM::AfterSet) {
		long wv;
		pProp->Get(wv);
		if ((wv > 900) || (wv < 338)) {
			return DEVICE_ERR;
		}
		std::vector<unsigned char> cmd;
		std::vector<unsigned char> response;
		cmd.push_back(0xDA);
		cmd.push_back(0x01);
		cmd.push_back((unsigned char) (wv));
		cmd.push_back(((unsigned char)(wv>>8) | (tiltSpeed_ << 6)));
		wv_ = wv;
		int ret = hub_->SetCommand(cmd);
		initializeDelayTimer();
		return ret;
		
	}
	else if (eAct == MM::IsSequenceable) {
		int max;
		if (sequenceEvenlySpaced_) { max = 3;} //in this case the "sequence" is actually the start, stop, and step of wavelengths
		else { max = 100;}
		pProp->SetSequenceable(max);
		return DEVICE_OK;
	}
	else if (eAct == MM::StartSequence) {
		ttlInEnabled_ = true;
		return configureTTL(ttlInRisingEdge_, ttlInEnabled_, false, 1);
	}
	else if (eAct == MM::StopSequence) {
		ttlInEnabled_ = false;
		return configureTTL(ttlInRisingEdge_, ttlInEnabled_, false, 1);
	}
	else if (eAct == MM::AfterLoadSequence) {
		std::vector<unsigned char> cmd;
		cmd.push_back(0xFA);
		if (sequenceEvenlySpaced_) {
			cmd.push_back(0xF1);
			cmd.push_back(1);
			std::vector<std::string> seq = pProp->GetSequence();
			for (int i=0; i<3; i++) {
				int wv = std::stoi(seq.at(0));
				cmd.push_back((unsigned char) (wv));
				cmd.push_back((unsigned char) wv>>8);
			}
		} else {
			cmd.push_back(0xF2);
			cmd.push_back(1);
			std::vector<std::string> seq = pProp->GetSequence();
			for (int i=0; i<seq.size(); i++){
				int wv = std::stoi(seq.at(i));
				cmd.push_back((unsigned char) (wv));
				cmd.push_back((unsigned char) wv>>8);
			}
			cmd.push_back(0);	//Terminate the sequence loading command.
			cmd.push_back(0);
		}
		return hub_->SetCommand(cmd);
	}
}

int LambdaVF5::onTiltSpeed(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		pProp->Set(tiltSpeed_);
	}
	else if (eAct == MM::AfterSet) {
		pProp->Get(tiltSpeed_);
	}
	return DEVICE_OK;
}

int LambdaVF5::onWheelTilt(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		pProp->Set(uSteps_);
	}
	else if (eAct == MM::AfterSet)
	{
		long uSteps;
		pProp->Get(uSteps);
		if ((uSteps > 273) || (uSteps < 0)) {
			return DEVICE_ERR;
		}
		std::vector<unsigned char> cmd;
		cmd.push_back(0xDE);
		cmd.push_back(0x01);
		cmd.push_back((unsigned char) (uSteps));
		cmd.push_back((unsigned char) uSteps>>8);
		int ret = hub_->SetCommand(cmd);
		if (ret != DEVICE_OK) { return ret;}
		initializeDelayTimer();
		uSteps_ = uSteps;
	}
	return DEVICE_OK;
}

int LambdaVF5::configureTTL( bool risingEdge, bool enabled, bool output, unsigned char channel){
	if (channel > 2) {
		LogMessage("Lambda VF5: Invalid TTL Channel specified", false);
		return DEVICE_ERR;
	}
	std::vector<unsigned char> cmd;
	cmd.push_back(0xFA);
	unsigned char action = 0xA0;
	action |= (output << 4);
	if (enabled) {
		if (risingEdge) { action |= 0x03; }
		else { action |= 0x04; }
	}
	cmd.push_back(action);
	cmd.push_back(channel);
	return hub_->SetCommand(cmd);
}

int LambdaVF5::onTTLOut(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		std::string setting;
		if (ttlOutEnabled_) {
			if (ttlOutRisingEdge_){
				setting = "Rising Edge";
			}
			else {
				setting = "Falling Edge";
			}
		} else {
			setting = "Disabled";
		}
		pProp->Set(setting.c_str());
		return DEVICE_OK;
	}
	else if (eAct == MM::AfterSet) {
		std::string setting;
		pProp->Get(setting);
		if (setting.compare("Disabled")==0) {
			ttlOutEnabled_ = false;
		}
		else if (setting.compare("Rising Edge")==0) {
			ttlOutEnabled_ = true;
			ttlOutRisingEdge_ = true;
		}
		else if (setting.compare("Falling Edge")==0) {
			ttlOutEnabled_ = true;
			ttlOutRisingEdge_ = false;
		}
		else { //The setting wasn't valid for some reason
			return DEVICE_ERR;
		}
		return configureTTL(ttlOutRisingEdge_, ttlOutEnabled_, true, 1);
	}
}

int LambdaVF5::onTTLIn(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		std::string setting;
		if (ttlInEnabled_) {
			if (ttlInRisingEdge_){
				setting = "Rising Edge";
			}
			else {
				setting = "Falling Edge";
			}
		} else {
			setting = "Disabled";
		}
		pProp->Set(setting.c_str());
		return DEVICE_OK;
	}
	else if (eAct == MM::AfterSet) {
		std::string setting;
		pProp->Get(setting);
		if (setting=="Disabled") {
			ttlInEnabled_ = false;
		}
		else if (setting=="Rising Edge") {
			ttlInEnabled_ = true;
			ttlInRisingEdge_ = true;
		}
		else if (setting=="Falling Edge") {
			ttlInEnabled_ = true;
			ttlInRisingEdge_ = false;
		}
		else { //The setting wasn't valid for some reason
			return DEVICE_ERR;
		}
		return configureTTL(ttlInRisingEdge_, ttlInEnabled_, false, 1);
	}
}

int LambdaVF5::onSequenceType(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		std::string setting;
		if (sequenceEvenlySpaced_) {
			setting = "Evenly Spaced";
		} else {
			setting = "Arbitrary";
		}
		pProp->Set(setting.c_str());
		return DEVICE_OK;
	}
	else if (eAct == MM::AfterSet) {
		std::string setting;
		pProp->Get(setting);
		if (setting=="Arbitrary") {
			sequenceEvenlySpaced_ = false;
		}
		else if (setting=="Evenly Spaced") {
			sequenceEvenlySpaced_ = true;
		}
		else { //The setting wasn't valid for some reason
			return DEVICE_ERR;
		}
		return DEVICE_OK;
	}
}