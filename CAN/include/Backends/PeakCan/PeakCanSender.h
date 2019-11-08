/*
 * CanSender.h
 *
 *  Created on: Apr 1, 2018
 *      Author: famez
 *      Implementation of can sender for the linux sockets layer
 */

#ifndef BACKENDS_SOCKETS_PEAKCANSENDER_H_
#define BACKENDS_SOCKETS_PEAKCANSENDER_H_

#include <CommonCanSender.h>

#include <Backends/PeakCan/PeakCanSymbols.h>

namespace Can
{
namespace PeakCan
{
class PeakCanSender : public CommonCanSender
{
  private:
	TPCANHandle mCurrentHandle;

  protected:
	void _sendFrame(const CanFrame &frame) const override;

  public:
	PeakCanSender(TPCANHandle handle);
	virtual ~PeakCanSender();
};

} // namespace PeakCan
} /* namespace Can */

#endif /* BACKENDS_SOCKETS_CANSENDER_H_ */
