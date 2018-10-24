#include "SutterLambda.h"

LambdaVF5::LambdaVF5(const char* name):
	WheelBase(name, 0, true, "Lambda VF-5 Tunable Filter"),
	whiteLightMode_(false), 
	mEnabled_(true), 
	wv_(500),
	uSteps_(100)
{};

int LambdaVF5::Initialize(){
	int ret = WheelBase::Initialize();
	if (ret != DEVICE_OK) { return ret;}

	CPropertyAction* pAct = new CPropertyAction(this, &LambdaVF5::onWhiteLightMode);
	ret = CreateProperty("White Light Mode", "0", MM::Integer, false, pAct);
	if (ret != DEVICE_OK) { return ret; }

	pAct = new CPropertyAction(this, &LambdaVF5::onWavelength);
	ret = CreateProperty("Wavelength", "500", MM::Integer, false, pAct);
	if (ret != DEVICE_OK) { return ret; }
	
	pAct = new CPropertyAction(this, &LambdaVF5::onWheelTilt);
	ret = CreateProperty("Wheel Tilt (uSteps)", "100", MM::Integer, false, pAct);
	if (ret != DEVICE_OK) { return ret; }

	pAct = new CPropertyAction(this, &LambdaVF5::onSequenceTriggerChannel);
	ret = CreateProperty("Sequencing TTL Channel", "0", MM::Integer, false, pAct);
	if (ret != DEVICE_OK) { return ret; }
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
		hub_->SetCommand(cmd);
		whiteLightMode_ = (bool) whiteLightMode;
	}
	return DEVICE_OK;
}

int LambdaVF5::onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		std::vector<unsigned char> cmd;
		std::vector<unsigned char> response;
		cmd.push_back(0xDB);
		hub_->SetCommand(cmd, cmd, response);
		if (response.at(0) != 0x01) {
			return DEVICE_ERR;
		}
		long wv = ((response.at(1) << 8) | (response.at(2)));
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
		cmd.push_back((unsigned char) ((wv << 8) | (speed_ << 6)));
		cmd.push_back((unsigned char)wv);
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
	return DEVICE_OK;
}

int LambdaVF5::onWheelTilt(MM::PropertyBase* pProp, MM::ActionType eAct) {
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
		hub_->SetCommand(cmd);
		uSteps_ = uSteps;
	}
	return DEVICE_OK;
}

int LambdaVF5::configureTTL( bool risingEdge, bool enabled, bool output, unsigned int channel){
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
