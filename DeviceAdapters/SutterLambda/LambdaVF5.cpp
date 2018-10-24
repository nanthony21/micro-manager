#include "SutterLambda.h"

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
		hub_->SetCommand(cmd, cmd, response, false);
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

		hub_->SetCommand(cmd, cmd, response, false);
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
		hub_->SetCommand(cmd, cmd, response, false);
		uSteps_ = uSteps;
	}
	return DEVICE_OK;
}

