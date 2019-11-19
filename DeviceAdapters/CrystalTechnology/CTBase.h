#pragma once

#define BREAK_MM_ERR if (ret != DEVICE_OK) { return ret; }
#define BEFOREGET if (eAct == MM::BeforeGet)
#define AFTERSET if (eAct == MM::AfterSet)

template <class T, class U>
class CTBase: public T {
	//Serves as an abstract base class for micromanager device adapters using the CTDriver class.
public:
	CTBase();
	~CTBase();
	//Device API
	int Initialize();
	int Shutdown();
	virtual void GetName(char* pName) const = 0;
	bool Busy() {return false;}; 
	bool SupportsDeviceDetection() { return false; };

protected:
	CTDriverCyAPI* driver_;
private:
	//Properties
	int onPort(MM::PropertyBase* pProp, MM::ActionType eAct);
	int tx_(std::string cmd);
	int rx_(std::string& response);
	const char* port_;
};


template <class T, class U>
CTBase<T, U>::CTBase():
	port_("Undefined"),
	driver_(NULL)
{ 
	CPropertyAction* pAct = new CPropertyAction((U*)this, &CTBase::onPort); //This casting to T is needed to prevent errors.
	this->CreateStringProperty(MM::g_Keyword_Port, "Unkn", false, pAct, true);
}

template <class T, class U>
CTBase<T, U>::~CTBase() {
	delete this->driver_; //Not sure if this is necessary
}

template <class T, class U>
int CTBase<T, U>::Initialize() {
	{
		using namespace std::placeholders;
		try {
			this->driver_ = new CTDriverCyAPI(this->devSerialNumber);
		} catch {
			return DEVICE_NOT_CONNECTED;
		}
	}
	return DEVICE_OK;
}

template <class T, class U>
int CTBase<T, U>::onPort(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		pProp->Set(this->port_);
	} else AFTERSET {
		std::string str;
		pProp->Get(str);
		this->port_ = str.c_str();
	}
	return DEVICE_OK;
}

template <class T, class U>
int CTBase<T, U>::Shutdown() { return DEVICE_OK;}

template <class T, class U>
int CTBase<T, U>::tx_(std::string cmd) {
	int ret = this->SendSerialCommand(port_, cmd.c_str(), "\r");
	BREAK_MM_ERR
	return CTDriver::OK;
}

template <class T, class U>
int CTBase<T, U>::rx_(std::string& response) {
	int ret = this->GetSerialAnswer(port_, "\r", response);
	BREAK_MM_ERR
	return CTDriver::OK;
}
	


