#include "CrystalTechnology.h"

#define BREAK_MM_ERR if (ret != DEVICE_OK) { return ret; }
#define BEFOREGET if (eAct == MM::BeforeGet)
#define AFTERSET if (eAct == MM::AfterSet)

CTBase::CTBase(uint8_t numChans) { 
	this->numChans_ = numChans;
	CPropertyAction* pAct = new CPropertyAction(this, &CTBase::onPort);
	this->CreateStringProperty(MM::g_Keyword_Port, "Unkn", false, pAct, true);
}

int CTBase::Initialize() {
	{
		using namespace std::placeholders;
		this->driver_ = new CTDriver(numChans_, std::bind(&CTBase::tx_, this, _1), std::bind(&CTBase::rx_, this, _1));
	}
}

int CTBase::onPort(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		int ret = pProp->Set(this->port_);
		BREAK_MM_ERR
	} else AFTERSET {
		std::string str;
		int ret = pProp->Get(str);
		BREAK_MM_ERR
		this->port_ = str.c_str();
	}
	return DEVICE_OK;
}

int CTBase::Shutdown() {
	delete this->driver_; //not sure if this is really necesary.
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
	


