#include "CrystalTechnology.h"

#define BREAKERR if (ret!=0) { return ret; }
#define BEFOREGET if (eAct == MM::BeforeGet)
#define AFTERSET if (eAct == MM::AfterSet)

CTTunableFilter::CTTunableFilter(): CTBase() {}

int CTTunableFilter::Initialize() {
	int ret = CTBase.Initialize();
	BREAKERR

	CPropertyAction* pAct = new CPropertyAction(this, &CTTunableFilter::onWavelength);
	ret = this->CreateProperty("Wavelength", "0", MM::Float, false, pAct, false);
	BREAKERR
		//TODO set limits

	pAct = new CPropertyAction(this, &CTTunableFilter::onBandwidth);
	ret = this->CreateProperty("Bandwidth", "0", MM::Float, false, pAct, false);
	BREAKERR
		//TODO set limits
}

int CTTunableFilter::onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		pProp->Set((long)this->wv);
	} else AFTERSET {
		int wavelength;
		pProp->Get(wavelength);
		this->setWavelength(wavelength);
	}
	return DEVICE_OK;
}

int CTTunableFilter::onBandwidth(MM::PropertyBase* pProp, MM::ActionType eAct) {
	BEFOREGET {
		pProp->Set(this->bw);
	} else AFTERSET {
		int bandwidth;
		pProp->Get(bandwidth);
		this->setBandwidth(bandwidth);
	}
	return DEVICE_OK;

}

void CTTunableFilter::setWavelength(double wv) {
	this->wv = wv;
}

void CTTunableFilter::setBandwidth(double wv) {

}

int CTTunableFilter::updateWvs() {
	for (int i=0; i<driver_->numChannels(); i++) {
		int ret = driver_->getWavelengthNm(i, wvs[i]);
		BREAKERR
	}
	return DEVICE_OK;
}