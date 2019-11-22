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
	CTDriver* driver_;
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
	
	for (uint8_t i=0; i<8; i++) { //8 was chosen arbitrarily here.
		try{
			AOTFLibCTDriver drv(i)
			this->AddAllowedValue("Device", std::toString((long long) i);
		} catch () {
			break;
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
			this->driver_ = new AOTFLibCTDriver(this->instNum);
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
		pProp->Set(this->instNum);
	} else AFTERSET {
		pProp->Get(this->instNum);
	}
	return DEVICE_OK;
}

template <class T, class U>
int CTBase<T, U>::Shutdown() { return DEVICE_OK;}
	


