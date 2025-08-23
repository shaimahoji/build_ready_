//-------------------------------
// common/SatelliteView.h
//-------------------------------
#pragma once

class SatelliteView {
public:
	virtual ~SatelliteView() {}
    virtual char getObjectAt(size_t x, size_t y) const = 0;
};