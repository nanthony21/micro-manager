#include "CrystalTechnology.h"

#define BREAKERR if (ret!=0) { return ret; }
#define BEFOREGET if (eAct == MM::BeforeGet)
#define AFTERSET if (eAct == MM::AfterSet)

CTTunableFilter::CTTunableFilter():
	CTBase(),
	freq_(0),
	asf_(0)
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

	pAct = new CPropertyAction(this, &CTTunableFilter::onAmplitude);
	ret = this->CreateProperty("Amplitude", "0", MM::Integer, false, pAct, false);
	BREAKERR
	this->SetPropertyLimits("Amplitude", 0, 16383);

	return DEVICE_OK;
}

int CTTunableFilter::onFrequency(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		double freq;
		int ret = driver_->getFrequencyMhz(0, freq);
		BREAKERR
		this->freq_ = freq;
		pProp->Set(this->freq_);
	} else AFTERSET {
		double freq;
		pProp->Get(freq);
		this->freq_ = freq;
		int ret = driver_->setFrequencyMhz(0, freq);
		BREAKERR
	}
	return DEVICE_OK;
}

int CTTunableFilter::onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		double freq;
		this->GetProperty("Frequency (MHz)", freq);
		double wv;
		int ret = driver_->freqToWavelength(freq, wv); 
		BREAKERR
		pProp->Set(wv);
	} else AFTERSET {
		double wavelength;
		double freq;
		pProp->Get(wavelength);
		int ret = driver_->wavelengthToFreq(wavelength, freq);
		BREAKERR
		this->SetProperty("Frequency (MHz)", std::to_string((long double)freq).c_str());
	}
	return DEVICE_OK;
}

int CTTunableFilter::onAmplitude(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		pProp->Set((long) this->asf_);
	} else AFTERSET {
		long amp;
		pProp->Get(amp);
		this->asf_ = amp;
		if (this->shutterOpen_) {
			int ret = driver_->setAmplitude(0, this->asf_);
			BREAKERR
		}
	}
	return DEVICE_OK;
}

int CTTunableFilter::SetOpen(bool open) {
	this->shutterOpen_ = open;
	unsigned int amp = open ? this->asf_ : 0;
	int ret = driver_->setAmplitude(0, amp);
	BREAKERR
	return DEVICE_OK;
}

int CTTunableFilter::GetOpen(bool& open) {
	open = this->shutterOpen_;
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
}

int CTTunableFilter::updateWvs() {
	for (int i=0; i<driver_->numChannels(); i++) {
		int ret = driver_->getWavelengthNm(i, wvs_[i]);
		BREAKERR
	}
	return DEVICE_OK;
}*/