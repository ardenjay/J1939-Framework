/*
 * CanSniffer.h
 *
 *  Created on: Jun 5, 2018
 *      Author: fernado
 */

#ifndef CANSNIFFER_H_
#define CANSNIFFER_H_

#include <vector>

#include "CommonCanReceiver.h"

typedef void (*OnReceiveFramePtr)(const Can::CanFrame &frame,
								  const Utils::TimeStamp &tStamp,
								  const std::string &interface, void *data);
typedef bool (*OnTimeoutPtr)();

namespace Can
{
class CanSniffer
{
  private:
	OnReceiveFramePtr mRcvCB = nullptr;
	OnTimeoutPtr mTimeoutCB = nullptr;
	void *mData = nullptr; // Data to be passed to the OnReceiveFramePtr
						   // callback
	std::vector<CommonCanReceiver *> mReceivers;
	bool mRunning = true;

	int wait_fd(timeval tv, fd_set &) const;
	void notify(fd_set &) const;
	void notify_recv(CommonCanReceiver *recv) const;
  public:
	CanSniffer() {}
	CanSniffer(OnReceiveFramePtr recvCB, OnTimeoutPtr timeoutCB,
			   void *data = nullptr);
	CanSniffer(const CanFilter &other) = delete;
	CanSniffer(CanFilter &&other) = delete;
	virtual ~CanSniffer();

	CanSniffer &operator=(const CanFilter &other) = delete;
	CanSniffer &operator=(CanFilter &&other) = delete;

	/*
	 * Add a receiver from where to receive the frames. CanSniffer becomes the
	 * owner and will deallocate the receiver.
	 */
	void addReceiver(CommonCanReceiver *receiver)
	{
		mReceivers.push_back(receiver);
	}
	void sniff(u32 timeout) const;
	void setFilters(std::set<CanFilter> filters);
	int getNumberOfReceivers() const { return mReceivers.size(); }
	void reset() { mRunning = true; }
	void finish() { mRunning = false; }
	void setOnRecv(OnReceiveFramePtr recvCB) { mRcvCB = recvCB; }
	void setOnTimeout(OnTimeoutPtr timeoutCB) { mTimeoutCB = timeoutCB; }
	void setData(void *data) { mData = data; }
};

} /* namespace Can */

#endif /* CANSNIFFER_H_ */
