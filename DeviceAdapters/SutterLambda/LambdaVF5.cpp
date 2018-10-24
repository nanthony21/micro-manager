#include "SutterLambda.h"

LambdaVF5::LambdaVF5(const char* name):
	Wheel(name, 0, "Lambda VF-5 Tunable Filter"),
	whiteLightMode_(false), 
	mEnabled_(true), 
	wv_(500),
	uSteps_(100)
{};

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
	if (eAct == MM::BeforeGet)
	{
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
	}
	else if (eAct == MM::AfterSet)
	{
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

		hub_->SetCommand(cmd);
		wv_ = wv;
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
	hub_->SetCommand(cmd);
}
