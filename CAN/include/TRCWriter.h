/*
 * TRCWriter.h
 *
 *  Created on: Jun 8, 2018
 *      Author: fernado
 */

#ifndef TRCWRITER_H_
#define TRCWRITER_H_

#include <fstream>
#include <iostream>

#include "CanFrame.h"
#include "Utils.h"

namespace Can
{
class TRCWriter
{
  private:
	std::ofstream mFileStream;
	unsigned int mCounter;

  public:
	TRCWriter();
	TRCWriter(const std::string &file);
	virtual ~TRCWriter();

	void write(const CanFrame &frame, const Utils::TimeStamp &timeStamp);

	bool open(const std::string &file);
	void close();

	class TRCWriteException : public std::exception
	{
	};
};

} /* namespace Can */

#endif /* TRCWRITER_H_ */
