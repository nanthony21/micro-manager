#include "SuperK.h"

SuperKDevice::SuperKDevice(uint8_t devType): type_(devType) {}

void SuperKDevice::setNKTAddress(uint8_t address) {
	address_ = address;
}

uint8_t SuperKDevice::getNKTAddress() {
	return address_;
}

uint8_t SuperKDevice::getNKTType() {
	return type_;
}

