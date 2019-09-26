#include "SuperK.h"

SuperKDevice::SuperKDevice(): address_(255){}

uint8_t SuperKDevice::getNKTAddress() {
	return address_;
}

void SuperKDevice::setNKTAddress(uint8_t address) {
	address_ = address;
}