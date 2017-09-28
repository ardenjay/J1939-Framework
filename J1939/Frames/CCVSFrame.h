/*
 * CCVSFrame.h
 *
 *  Created on: Sep 23, 2017
 *      Author: famez
 */

#ifndef FRAMES_CCVSFRAME_H_
#define FRAMES_CCVSFRAME_H_

#include "../J1939Frame.h"

#define CCVS_FRAME_MAX_SIZE			8

namespace J1939 {

class CCVSFrame: public J1939Frame {

public:
	enum EPtoState {
		PTO_DISABLED,
		PTO_SET,
		PTO_NOT_AVAILABLE
	};

private:
	float 		mWheelSpeed;			//Km/h
	bool 		mClutchPressed;
	bool 		mBrakePressed;
	bool 		mCruiseControlActive;
	EPtoState 	mPtoState;

public:
	CCVSFrame();
	CCVSFrame(u16 wheelSpeed, bool clutchPressed, bool brakePressed, bool cruiseControlActive, EPtoState ptoState);
	virtual ~CCVSFrame();

	bool isBrakePressed() const {
		return mBrakePressed;
	}

	void setBrakePressed(bool brakePressed) {
		mBrakePressed = brakePressed;
	}

	bool isClucthPressed() const {
		return mClutchPressed;
	}

	void setClucthPressed(bool clucthPressed) {
		mClutchPressed = clucthPressed;
	}

	bool isCruiseControlActive() const {
		return mCruiseControlActive;
	}

	void setCruiseControlActive(bool cruiseControlActive) {
		mCruiseControlActive = cruiseControlActive;
	}

	EPtoState getPtoState() const {
		return mPtoState;
	}

	void setPtoState(EPtoState ptoState) {
		mPtoState = ptoState;
	}

	float getWheelSpeed() const {
		return mWheelSpeed;
	}

	void setWheelSpeed(float wheelSpeed) {
		mWheelSpeed = wheelSpeed;
	}

	//Implements J1939Frame methods
	void decodeData(const u8* buffer, size_t length);
	void encodeData(u8* buffer, size_t& length);
};

} /* namespace J1939 */

#endif /* FRAMES_CCVSFRAME_H_ */