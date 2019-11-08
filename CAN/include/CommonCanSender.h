/*
 * CommonCanSender.h
 *
 *  Created on: Apr 1, 2018
 *      Author: famez
 *      Implementation of can sender for the linux sockets layer
 */

#ifndef BACKENDS_SOCKETS_COMMONCANSENDER_H_
#define BACKENDS_SOCKETS_COMMONCANSENDER_H_

#include <memory>
#include <vector>

#include <mutex>
#include <thread>

#include <time.h>

#include <ICanSender.h>

namespace Can
{
class CommonCanSender : public ICanSender
{
  private:
	class CanFrameRing
	{
	  private:
		std::vector<CanFrame> mFrames;
		timespec mTxTimestamp;
		u32 mPeriod;
		size_t mCurrentpos;
		OnSendCallback mCallback;

	  public:
		CanFrameRing(u32 period, OnSendCallback callback = OnSendCallback())
			: mPeriod(period), mCurrentpos(0), mCallback(callback)
		{
			mTxTimestamp = {0, 0};
		}
		~CanFrameRing() {}
		CanFrameRing(const CanFrameRing &other) = default;
		CanFrameRing &operator=(const CanFrameRing &other) = delete;
		CanFrameRing(CanFrameRing &&other) = default;
		CanFrameRing &operator=(CanFrameRing &&other) = default;

		void setTxTimestamp(timespec timeStamp) { mTxTimestamp = timeStamp; }
		timespec getTxTimestamp() const { return mTxTimestamp; }

		void pushFrame(const CanFrame &frame);
		void setFrames(const std::vector<CanFrame> &);
		void shift();
		CanFrame &getCurrentFrame() { return mFrames[mCurrentpos]; }
		u32 getCurrentPeriod() const;
		const std::vector<CanFrame> &getFrames() const { return mFrames; }

		const OnSendCallback &getCallback() { return mCallback; }
	};

	mutable std::mutex mFramesLock;
	std::vector<CanFrameRing> mFrameRings;
	bool mFinished;
	std::unique_ptr<std::thread> mThread = nullptr;

  protected:
	virtual void _sendFrame(const CanFrame &frame) const = 0;

  public:
	CommonCanSender();
	virtual ~CommonCanSender();

	// ICanSender implementation
	bool initialize();
	bool finalize();
	bool sendFrame(CanFrame frame, u32 period,
				   OnSendCallback callback = OnSendCallback());
	bool sendFrames(std::vector<CanFrame> frames, u32 period,
					OnSendCallback callback = OnSendCallback());

	/*
	 * Sends the frame given as argument to the CAN network only once.
	 */
	void sendFrameOnce(const CanFrame &frame) override { _sendFrame(frame); }
	void unSendFrame(u32 id);
	void unSendFrames(const std::vector<u32> &ids);
	bool isSent(const std::vector<u32> &ids);
	bool isSent(u32 id);

	void run();
};

} /* namespace Can */

#endif /* BACKENDS_SOCKETS_CANSENDER_H_ */
