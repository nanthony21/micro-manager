#include "CrystalTechnology.h"

#define BREAK_MM_ERR if (ret != DEVICE_OK) { return ret; }

int CTBase::Initialize() {
	{
		using namespace std::placeholders;
		driver_ = new CTDriver(numChans_, std::bind(&CTBase::tx_, this, _1), std::bind(&CTBase::rx_, this, _1));
	}
}

int CTBase::Shutdown() {
	delete driver_; //not sure if this is really necesary.
}

int CTBase::tx_(std::string cmd) {
	int ret = this->SendSerialCommand(port_, cmd.c_str(), "\r");
	BREAK_MM_ERR
	return CTDriver::OK;
}

int CTBase::rx_(std::string& response) {
	int ret = this->GetSerialAnswer(port_, "\r", response);
	BREAK_MM_ERR
	return CTDriver::OK;
}
	


