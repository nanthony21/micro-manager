#include "CrystalTechnology.h"

#define BREAKERR if (ret!=0) { return ret; }
#define BEFOREGET if (eAct == MM::BeforeGet)
#define AFTERSET if (eAct == MM::AfterSet)

CTTunableFilter::CTTunableFilter():
	CTBase(),
	wv_(0),
	bw_(0)
{}

int CTTunableFilter::Initialize() {
	int ret = CTBase::Initialize();
	BREAKERR

	CPropertyAction* pAct = new CPropertyAction(this, &CTTunableFilter::onFrequency);
	ret = this->CreateProperty("Frequency (MHz)", "0", MM::Float, false, pAct, false);
	BREAKERR
	//this->SetPropertyLimits

	pAct = new CPropertyAction(this, &CTTunableFilter::onWavelength);
	ret = this->CreateProperty("Wavelength", "0", MM::Float, false, pAct, false);
	BREAKERR
	//TODO set limits

	//pAct = new CPropertyAction(this, &CTTunableFilter::onBandwidth);
	//ret = this->CreateProperty("Bandwidth", "0", MM::Float, false, pAct, false);
	//BREAKERR
	//TODO set limits
	return DEVICE_OK;
}

int CTTunableFilter::onFrequency(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		double freq;
		driver_->wavelengthToFreq(this->wv_, freq);
		pProp->Set(freq);
	} else AFTERSET {
		double freq;
		pProp->Get(freq);
		driver_->setFrequencyMhz(0, freq);
	}
	return DEVICE_OK;
}

int CTTunableFilter::onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		pProp->Set(this->wv_);
	} else AFTERSET {
		double wavelength;
		double freq;
		pProp->Get(wavelength);
		driver_->wavelengthToFreq(wavelength, freq);
		this->SetProperty("Frequency (MHz)", std::to_string((long double)freq).c_str());
	}
	return DEVICE_OK;
}

//Amplitude

/*int CTTunableFilter::onBandwidth(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		pProp->Set((long)this->bw_);
	} else AFTERSET {
		int bandwidth;
		pProp->Get((long&)bandwidth);
		this->setBandwidth(bandwidth);
	}
	return DEVICE_OK;

}*/

/*void CTTunableFilter::setWavelength(double wv) {
	/*int chans = driver_.numChannels();
	for (int i=0; i<chans; i++) {
		int newWv = wv - bw_/2 + (bw_ * (double) i / (chans-1));
		this->wvs_[i] = newWv;
		driver_.setWavelengthNm(i, newWv);
	}
	this->wv_ = wv;
}*/

/*void CTTunableFilter::setBandwidth(double bw) {
	int chans = driver_.numChannels();
	for (int i=0; i<chans; i++) {
		int newWv = wv_ - bw/2 + (bw * (double) i / (chans-1));
		this->wvs_[i] = newWv;
		driver_.setWavelengthNm(i, newWv);
	}
	this->bw_ = bw; //todo
}*/

int CTTunableFilter::updateWvs() {
	for (int i=0; i<driver_->numChannels(); i++) {
		int ret = driver_->getWavelengthNm(i, wvs_[i]);
		BREAKERR
	}
	return DEVICE_OK;
}