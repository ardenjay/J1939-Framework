/*
 * CanSniffer.cpp
 *
 *  Created on: Jun 5, 2018
 *      Author: fernado
 */

#include <Assert.h>
#include <CanSniffer.h>

using namespace Utils;

namespace Can
{
CanSniffer::CanSniffer(OnReceiveFramePtr recvCB, OnTimeoutPtr timeoutCB,
					   void *data)
	: mRcvCB(recvCB), mTimeoutCB(timeoutCB), mData(data)
{
}

CanSniffer::~CanSniffer()
{
	for (auto receiver = mReceivers.begin(); receiver != mReceivers.end();
		 ++receiver) {
		delete *receiver;
	}
}

void CanSniffer::setFilters(std::set<CanFilter> filters)
{
	for (auto receiver = mReceivers.begin(); receiver != mReceivers.end();
		 ++receiver) {
		(*receiver)->setFilters(filters);
	}
}

int CanSniffer::wait_fd(timeval tv, fd_set &rdfs) const
{
	int maxFd = -1;
	int result;

	do {
		FD_ZERO(&rdfs);

		for (auto receiver = mReceivers.begin();
			receiver != mReceivers.end(); ++receiver) {

			FD_SET((*receiver)->getFD(), &rdfs);

			if ((*receiver)->getFD() > maxFd)
				maxFd = (*receiver)->getFD();
		}

		if (maxFd == -1)
			return -EINVAL;

		result = select(maxFd + 1, &rdfs, NULL, NULL, &tv);

	} while (result == -1 && errno == EINTR);

	return result;
}

void CanSniffer::notify_recv(CommonCanReceiver *recv) const
{
	CanFrame canFrame;
	TimeStamp timestamp;

	bool ret = recv->receive(canFrame, timestamp);
	if (ret == false)
		return;

	ret = recv->filter(canFrame.getId());
	if (ret == false)
		return;

	(mRcvCB)(canFrame, timestamp, recv->getInterface(), mData);
}

void CanSniffer::notify(fd_set& rdfs) const
{
	for (auto receiver = mReceivers.begin();
			receiver != mReceivers.end(); ++receiver) {

		if (FD_ISSET((*receiver)->getFD(), &rdfs))
			notify_recv(*receiver);
	}
}

void CanSniffer::sniff(u32 timeout) const
{
	fd_set rdfs;
	timeval tv;
	int result;

	ASSERT(!mReceivers.empty());
	ASSERT(mRcvCB != nullptr);
	ASSERT(mTimeoutCB != nullptr);

	do {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) / 1000000;

		result = wait_fd(tv, rdfs);
		if (result < 0)
			continue;

		if (result == 0)
			(mTimeoutCB)();
		else
			notify(rdfs);
	} while (mRunning);
}

} /* namespace Can */
