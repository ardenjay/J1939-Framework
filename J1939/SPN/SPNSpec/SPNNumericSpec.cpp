/*
 * SPNNumericSpec.cpp
 *
 *  Created on: Oct 24, 2017
 *      Author: root
 */

#include <iostream>
#include <sstream>
#include <string.h>
#include <string>

#include <Assert.h>
#include <Utils.h>

#include <J1939Common.h>
#include <SPN/SPNSpec/SPNNumericSpec.h>

namespace J1939
{
SPNNumericSpec::SPNNumericSpec(double formatGain, double formatOffset,
							   u8 byteSize, const std::string &units)
	: mFormatGain(formatGain), mFormatOffset(formatOffset), mByteSize(byteSize),
	  mUnits(units){

		  ASSERT(byteSize > 0) ASSERT(byteSize <= SPN_NUMERIC_MAX_BYTE_SYZE)

	  }

	  SPNNumericSpec::~SPNNumericSpec()
{
}

u32 SPNNumericSpec::getMaxValue() const
{
	return 0xFAFFFFFF >> (4 - mByteSize) * 8;
}

double SPNNumericSpec::formatValue(u32 value) const
{
	double aux = value;

	// Apply gain and offset
	return aux * mFormatGain + mFormatOffset;
}

} /* namespace J1939 */
