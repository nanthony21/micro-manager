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
	int onSetTuneCoeffs(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onReadOnlyTuneCoeffs(MM::PropertyBase* pProp, MM::ActionType eAct);
	int onBoardInfo(MM::PropertyBase* pProp, MM::ActionType eAct);
	uint8_t instNum; 
	std::vector<double> coeffs; //Length 5 starting all as zero.
};


template <class T, class U>
CTBase<T, U>::CTBase():
	instNum(0),
	driver_(NULL),
	coeffs(5, 0.0)//Length 5 starting all as zero.
{ 
	CPropertyAction* pAct = new CPropertyAction((U*)this, &CTBase::onSelDev); //This casting to T is needed to prevent errors.
	this->CreateProperty("Device", "None", MM::Integer, false, pAct, true);
	
	for (uint8_t i=0; i<8; i++) { //8 was chosen arbitrarily here.
		try{
			AOTFLibCTDriver* drv = new AOTFLibCTDriver(i);
			this->AddAllowedValue("Device", std::to_string((long long) i).c_str());
			delete drv;
		} catch (...) {
			break;
		}
	}

	pAct = new CPropertyAction((U*) this, &CTBase::onSetTuneCoeffs);
	this->CreateProperty("Set Tuning Coeffs (space separated: {1} {x} {x^2} {x^3} {x^4})", "0 0 0 0 0", MM::String, false, pAct, true);
}

template <class T, class U>
CTBase<T, U>::~CTBase() {
	delete this->driver_; //This will probably never be used becuase it's already deleted in `shutdown` maybe this will help with crashes though.
}

template <class T, class U>
int CTBase<T, U>::Initialize() {
	try {
		this->driver_ = new AOTFLibCTDriver(this->instNum);
	} catch (...) {
		return DEVICE_NOT_CONNECTED;
	}

	this->SetErrorText(CTDriver::ERR, "'Unspecified Error' in the Crystal Technologies 'CTDriver' device interface");
	this->SetErrorText(CTDriver::SERIAL_TIMEOUT, "'Serial Timeout Error' in the Crystal Technologies 'CTDriver' device interface");
	this->SetErrorText(CTDriver::NOT_INITIALIZED, "'Not Initialized Error' in the Crystal Technologies 'CTDriver' device interface");
	this->SetErrorText(CTDriver::INVALID_CHANNEL, "'Invalid Channel Error' in the Crystal Technologies 'CTDriver' device interface");
	this->SetErrorText(CTDriver::NOECHO, "'No Serial Echo Error' in the Crystal Technologies 'CTDriver' device interface");

	int ret = this->driver_->initialize();
	BREAK_MM_ERR

	ret = this->driver_->setTuningCoeffs(this->coeffs);
	BREAK_MM_ERR

	CPropertyAction* pAct = new CPropertyAction((U*) this, &CTBase::onReadOnlyTuneCoeffs);
	this->CreateProperty("Read Tuning Coeffs (space separated: {1} {x} {x^2} {x^3} {x^4})", "0 0 0 0 0", MM::String, true, pAct, false);

	return DEVICE_OK;
}

template <class T, class U>
int CTBase<T, U>::onSelDev(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		pProp->Set((long) this->instNum);
	} else AFTERSET {
		long num;
		pProp->Get(num);
		this->instNum = num;
	}
	return DEVICE_OK;
}

template <class T, class U>
int CTBase<T, U>::onSetTuneCoeffs(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		std::string response("");
		for (int i=0; i<5; i++) {
			std::ostringstream streamObj;
			streamObj << this->coeffs.at(i); //This should do smart numerical formatting. we'll see.
			response = response + streamObj.str() + " ";
		}
		response = response.substr(0, response.length()-1);//Get rid of the last space
		pProp->Set(response.c_str());
	} else AFTERSET {
		std::string csv;
		pProp->Get(csv);
		std::vector<std::string> coeffstr;
		boost::split(coeffstr, csv, boost::is_any_of(" "), boost::token_compress_on);
		if (coeffstr.size()!=5) {
			this->SetErrorText(99, "Wrong number of coefficient values, there should be 5.");
			return 99;
		}
		for (int i=0; i<5; i++) {
			double coeff = strtod(coeffstr.at(i).c_str(), NULL);
			this->coeffs.at(i) = coeff;
		}
	}
	return DEVICE_OK;
}

template <class T, class U>
int CTBase<T, U>::onReadOnlyTuneCoeffs(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		std::vector<double> getCoeffs(5, 0); //initialize to length 5 of zeros.
		int ret = this->driver_->getTuningCoeffs(getCoeffs);
		BREAK_MM_ERR
		std::string response("");
		for (int i=0; i<5; i++) {
			std::ostringstream streamObj;
			streamObj << getCoeffs[i]; //This should do smart numerical formatting. we'll see.
			response = response + streamObj.str() + " ";
		}
		response = response.substr(0, response.length()-1);//Get rid of the last space
		pProp->Set(response.c_str());
	}
	return DEVICE_OK;
}

template <class T, class U>
int CTBase<T, U>::Shutdown() { 
	delete this->driver_;	 //If the driver doesn't have it's destructor called then the USB driver will not allow the program to terminate until it is physically unplugged from the computer.
	this->driver_ = NULL;
	return DEVICE_OK;
}
	



