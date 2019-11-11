#include "CrystalTechnology.h"

#define BREAKERR if (ret!=0) { return ret; }

CTTunableFilter::CTTunableFilter(): CTBase() {}

int CTTunableFilter::Initialize() {
	int ret = CTBase.Initialize();
	BREAKERR

	CPropertyAction* pAct = new CPropertyAction(this, &CTTunableFilter::onWavelength);
	this->CreateProperty(

	pAct = new CPropertyAction(this, &CTTunableFilter::onBandwidth);
}