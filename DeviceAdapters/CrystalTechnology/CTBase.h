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
	int onSelDev(MM::PropertyBase* pProp, MM::ActionType eAct);
	const char* devSerialNumber;
};


template <class T, class U>
CTBase<T, U>::CTBase():
	port_("Undefined"),
	driver_(NULL)
{ 
	CPropertyAction* pAct = new CPropertyAction((U*)this, &CTBase::onSelDev); //This casting to T is needed to prevent errors.
	this->CreateStringProperty("Serial No.", "Unkn", false, pAct, true);
	std::map<std::string, CTDriver::DriverType> devMap = CTDriverCyAPI::getConnectedDevices();
	for ( std::pair<std::string, CTDriver::DriverType> pair : devMap) {
		std::string devDescrip;
		std::string serialNum = pair.first;
		CTDriver::DriverType dType = pair.second;
		this->AddAllowedValue("Serial No.", serialNum);
	}
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
int CTBase<T, U>::onSelDev(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		pProp->Set(this->devSerialNumber);
	} else AFTERSET {
		std::string str;
		pProp->Get(str);
		this->devSerialNumber = str.c_str();
	}
	return DEVICE_OK;
}

template <class T, class U>
int CTBase<T, U>::Shutdown() { return DEVICE_OK;}
	


