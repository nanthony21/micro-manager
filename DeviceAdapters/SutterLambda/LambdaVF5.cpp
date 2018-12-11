#include "SutterLambda.h"

LambdaVF5::LambdaVF5(const char* name):
	WheelBase(name, 0, true, "Lambda VF-5 Tunable Filter (Channel A)"),
	whiteLightMode_(false), 
	mEnabled_(true), 
	wv_(500),
	uSteps_(1),
	tiltSpeed_(3)
{};

int LambdaVF5::Initialize(){
	int ret = WheelBase::Initialize();
	if (ret != DEVICE_OK) { return ret;}

	CPropertyAction* pAct = new CPropertyAction(this, &LambdaVF5::onWhiteLightMode);
	ret = CreateProperty("White Light Mode", "0", MM::Integer, false, pAct);
	if (ret != DEVICE_OK) { return ret; }

	pAct = new CPropertyAction(this, &LambdaVF5::onWavelength);
	ret = CreateProperty("Wavelength", "500", MM::Integer, false, pAct);
	SetPropertyLimits("Wavelength", 338, 900);
	if (ret != DEVICE_OK) { return ret; }
	
	pAct = new CPropertyAction(this, &LambdaVF5::onWheelTilt);
	ret = CreateProperty("Wheel Tilt (uSteps)", "100", MM::Integer, false, pAct);
	SetPropertyLimits("Wheel Tilt (uSteps)", 1, 267);
	if (ret != DEVICE_OK) { return ret; }

	pAct = new CPropertyAction(this, &LambdaVF5::onSequenceTriggerChannel);
	ret = CreateProperty("Sequencing TTL Channel", "0", MM::Integer, false, pAct);
	SetPropertyLimits("Sequencing TTL Channel", 0, 2);
	if (ret != DEVICE_OK) { return ret; }
	
	pAct = new CPropertyAction(this, &LambdaVF5::onTiltSpeed);
	ret = CreateProperty("Tilt Speed", "3", MM::Integer, false, pAct);
	SetPropertyLimits("Tilt Speed", 0, 3);
	if (ret != DEVICE_OK) { return ret; }

	return DEVICE_OK;
}

int LambdaVF5::onSequenceTriggerChannel(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet){
		pProp->Set((long)sequenceTriggerTTL_);
	}
	else if (eAct == MM::AfterSet) {
		long newChannel;
		pProp->Get(newChannel);
		if ((newChannel < 0) || (newChannel > 2)){
			LogMessage("Lambda VF5: Invalid TTL channel was specified.");
			return DEVICE_ERR;
		}
		else{
			sequenceTriggerTTL_ = newChannel;
			return DEVICE_OK;
		}
	}
	return DEVICE_OK;
}

int LambdaVF5::onWhiteLightMode(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet)
	{
		pProp->Set((long)whiteLightMode_);
	}
	else if (eAct == MM::AfterSet)
	{
		long whiteLightMode;
		pProp->Get(whiteLightMode);
		std::vector<unsigned char> cmd;
		std::vector<unsigned char> response ;
		unsigned char _ = whiteLightMode ? 0xAA : 0xAC;
		cmd.push_back(_);
		int ret = hub_->SetCommand(cmd);
		if (ret != DEVICE_OK) { return ret;}
		whiteLightMode_ = (bool) whiteLightMode;
	}
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
		return hub_->SetCommand(cmd);
		
	}
	else if (eAct == MM::IsSequenceable) {
		pProp->SetSequenceable(100);
		return DEVICE_OK;
	}
	else if (eAct == MM::StartSequence) {
		return configureTTL(true, true, false, sequenceTriggerTTL_);
	}
	else if (eAct == MM::StopSequence) {
		return configureTTL(true,false,false, sequenceTriggerTTL_);
	}
	else if (eAct == MM::AfterLoadSequence) {
		std::vector<unsigned char> cmd;
		cmd.push_back(0xFA);
		cmd.push_back(0xF2);
		cmd.push_back(sequenceTriggerTTL_);
		std::vector<std::string> seq = pProp->GetSequence();
		for (int i=0; i<seq.size(); i++){
			int wv = std::stoi(seq.at(i));
			cmd.push_back((unsigned char) (wv << 8));
			cmd.push_back((unsigned char) wv);
		}
		cmd.push_back(0);	//Terminate the sequence loading command.
		cmd.push_back(0);
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
	if (eAct == MM::AfterSet)
	{
		long uSteps;
		pProp->Get(uSteps);
		if ((uSteps > 273) || (uSteps < 0)) {
			return DEVICE_ERR;
		}
		std::vector<unsigned char> cmd;
		std::vector<unsigned char> response;
		cmd.push_back(0xDE);
		cmd.push_back((unsigned char) uSteps);
		int ret = hub_->SetCommand(cmd);
		if (ret != DEVICE_OK) { return ret;}
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
