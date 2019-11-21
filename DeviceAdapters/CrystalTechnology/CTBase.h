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
	devSerialNumber("Undefined"),
	driver_(NULL)
{ 
	CPropertyAction* pAct = new CPropertyAction((U*)this, &CTBase::onSelDev); //This casting to T is needed to prevent errors.
	this->CreateStringProperty("Serial No.", "Unkn", false, pAct, true);
	std::vector<std::string> devs = CTDriverCyAPI::getConnectedDevices();
	if (devs.size()==0) {
		this->AddAllowedValue("Serial No.", "No Devices Found");
	} else {
		std::map<std::string, CTDriver::DriverType>::iterator it;
		for (std::vector<std::string>::iterator it = devs.begin() ; it != devs.end(); ++it) {
			std::string devDescrip = *it;
			this->AddAllowedValue("Serial No.", devDescrip.c_str());
		}
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
		} catch (...) {
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
	


