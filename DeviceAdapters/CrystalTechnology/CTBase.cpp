#include "CrystalTechnology.h"

#define BREAK_MM_ERR if (ret != DEVICE_OK) { return ret; }
#define BEFOREGET if (eAct == MM::BeforeGet)
#define AFTERSET if (eAct == MM::AfterSet)

template <class T>
CTBase<T>::CTBase():
	port_("Undefined"),
	driver_(NULL)
{ 
	CPropertyAction* pAct = new CPropertyAction(this, &CTBase::onPort);
	this->CreateStringProperty(MM::g_Keyword_Port, "Unkn", false, pAct, true);
}

template <class T>
int CTBase<T>::Initialize() {
	{
		using namespace std::placeholders;
		this->driver_ = CTDriver(std::bind(&CTBase::tx_, this, _1), std::bind(&CTBase::rx_, this, _1));
	}
}

template <class T>
int CTBase<T>::onPort(MM::PropertyBase* pProp, MM::ActionType eAct) {
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

template <class T>
int CTBase<T>::Shutdown() {}

template <class T>
int CTBase<T>::tx_(std::string cmd) {
	int ret = this->SendSerialCommand(port_, cmd.c_str(), "\r");
	BREAK_MM_ERR
	return CTDriver::OK;
}

template <class T>
int CTBase<T>::rx_(std::string& response) {
	int ret = this->GetSerialAnswer(port_, "\r", response);
	BREAK_MM_ERR
	return CTDriver::OK;
}
	


