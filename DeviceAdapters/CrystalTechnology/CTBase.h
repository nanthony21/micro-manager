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
	std::string devName; //This stores the `friendly name` of the USB device through the CyAPI
};


template <class T, class U>
CTBase<T, U>::CTBase():
	devName("Undefined"),
	driver_(NULL)
{ 
	CPropertyAction* pAct = new CPropertyAction((U*)this, &CTBase::onSelDev); //This casting to T is needed to prevent errors.
	this->CreateStringProperty("Device", "Unkn", false, pAct, true);
	std::vector<std::string> devs = CTDriverCyAPI::getConnectedDevices();
	if (devs.size()==0) {
		this->AddAllowedValue("Device", "No Devices Found");
	} else {
		std::map<std::string, CTDriver::DriverType>::iterator it;
		for (std::vector<std::string>::iterator it = devs.begin() ; it != devs.end(); ++it) {
			std::string devDescrip = *it;
			this->AddAllowedValue("Device", devDescrip.c_str());
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
		try {
			this->driver_ = new CTDriverCyAPI(this->devName);
		} catch (...) {
			return DEVICE_NOT_CONNECTED;
		}
		int ret = this->driver_->initialize();
		BREAK_MM_ERR
	}
	return DEVICE_OK;
}

template <class T, class U>
int CTBase<T, U>::onSelDev(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		pProp->Set(this->devName.c_str());
	} else AFTERSET {
		std::string str;
		pProp->Get(str);
		this->devName = str;
	}
	return DEVICE_OK;
}

template <class T, class U>
int CTBase<T, U>::Shutdown() { return DEVICE_OK;}
	


