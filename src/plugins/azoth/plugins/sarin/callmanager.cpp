/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "callmanager.h"
#include <QFuture>
#include <QElapsedTimer>
#include <util/threads/futures.h>
#include "toxthread.h"
#include "util.h"
#include "threadexceptions.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	CallManager::CallManager (ToxThread *thread, Tox *tox, QObject *parent)
	: QObject { parent }
	, Thread_ { thread }
	, ToxAv_
	{
		[tox]
		{
			TOXAV_ERR_NEW error;
			const auto toxAV = toxav_new (tox, &error);
			if (error != TOXAV_ERR_NEW_OK)
				throw std::runtime_error { "Failed to create tox AV instance: " + std::to_string (error) };
			return toxAV;
		} (),
		&toxav_kill
	}
	{
		toxav_callback_call (ToxAv_.get (),
				[] (ToxAV*, uint32_t friendNum, bool audio, bool video, void *udata)
				{
					static_cast<CallManager*> (udata)->HandleIncomingCall (friendNum);
				},
				this);
		toxav_callback_call_state (ToxAv_.get (),
				[] (ToxAV*, uint32_t friendNum, uint32_t state, void *udata)
				{
					static_cast<CallManager*> (udata)->HandleStateChanged (friendNum, state);
				},
				this);
		toxav_callback_audio_receive_frame (ToxAv_.get (),
				[] (ToxAV*, uint32_t friendNum, const int16_t *frames,
						size_t size, uint8_t channels, uint32_t rate,
						void *udata)
				{
					static_cast<CallManager*> (udata)->HandleAudio (friendNum, frames, size, channels, rate);
				},
				this);
	}

	namespace
	{
		const int AudioBitRate = 16;
		const int VideoBitRate = 0;
	}

	QFuture<CallManager::InitiateResult> CallManager::InitiateCall (const QByteArray& pkey)
	{
		return Thread_->ScheduleFunction ([this, pkey] (Tox *tox)
				{
					return InitiateResult::FromMaybe (GetFriendId (tox, pkey), UnknownFriendException {}) >>
							[this] (qint32 id)
							{
								TOXAV_ERR_CALL error;
								toxav_call (ToxAv_.get (), id, AudioBitRate, VideoBitRate, &error);
								return error != TOXAV_ERR_CALL_OK ?
										InitiateResult::Left (CallInitiateException { error }) :
										InitiateResult::Right ({ AudioBitRate });
							};
				});
	}

	QFuture<CallManager::WriteResult> CallManager::WriteData (int32_t callIdx, const QByteArray& data)
	{
		return Thread_->ScheduleFunction ([this, data, callIdx] (Tox*) -> WriteResult
				{
					const auto perFrame = av_DefaultSettings.audio_frame_duration * av_DefaultSettings.audio_sample_rate * av_DefaultSettings.audio_channels / 1000;
					const auto dataShift = perFrame * sizeof (int16_t);

					int currentPos = 0;
					for (; currentPos + dataShift < static_cast<uint> (data.size ()); currentPos += dataShift)
					{
						uint8_t prepared [RTP_PAYLOAD_SIZE] = { 0 };
						const auto size = toxav_prepare_audio_frame (ToxAv_.get (), callIdx,
								prepared, RTP_PAYLOAD_SIZE,
								reinterpret_cast<const int16_t*> (data.constData () + currentPos), perFrame);
						if (size < 0)
						{
							qWarning () << Q_FUNC_INFO
									<< "unable to prepare frame";
							throw FramePrepareException { size };
						}

						const auto rc = toxav_send_audio (ToxAv_.get (), callIdx, prepared, size);
						if (rc)
						{
							qWarning () << Q_FUNC_INFO
									<< "unable to send frame of size"
									<< size
									<< "; rc:"
									<< rc;
							throw FrameSendException { rc };
						}
						//qDebug () << "sent frame of size" << size;
					}

					return { data.mid (currentPos) };
				});
	}

	QFuture<CallManager::AcceptCallResult> CallManager::AcceptCall (int32_t friendIdx)
	{
		return Thread_->ScheduleFunction ([this, friendIdx] (Tox*)
				{
					TOXAV_ERR_ANSWER error;
					toxav_answer (ToxAv_.get (), friendIdx, AudioBitRate, VideoBitRate, &error);
					return error != TOXAV_ERR_ANSWER_OK ?
							AcceptCallResult::Left (CallAnswerException { error }) :
							AcceptCallResult::Right ({ AudioBitRate });
				});
	}

	void CallManager::HandleIncomingCall (int32_t friendNum)
	{
		Util::Sequence (this, Thread_->GetFriendPubkey (friendNum)) >>
				[this, friendNum] (const QByteArray& pubkey) { gotIncomingCall (pubkey, friendNum); };
	}

	void CallManager::HandleStateChanged (int32_t friendIdx, uint32_t state)
	{
		emit callStateChanged (friendIdx, state);
	}

	void CallManager::HandleAudio (int32_t call, const int16_t *frames, int size, int channels, int rate)
	{
		const QByteArray data { reinterpret_cast<const char*> (frames), static_cast<int> (size * sizeof (int16_t)) };
		emit gotFrameParams (call, channels, rate);
		emit gotFrame (call, data);
	}
}
}
}
