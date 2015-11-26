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

#include "calldatawriter.h"
#include <util/threads/futures.h>
#include "callmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	CallDataWriter::CallDataWriter (int32_t callIdx, CallManager *callMgr, QObject *parent)
	: QObject { parent }
	, CallIdx_ { callIdx}
	, Mgr_ { callMgr }
	{
	}

	qint64 CallDataWriter::WriteData (const QAudioFormat& fmt, const QByteArray& data)
	{
		if (IsWriting_)
		{
			qDebug () << Q_FUNC_INFO << "already writing, queuing up" << data.size ();
			Buffer_ += data;
			return data.size ();
		}

		qDebug () << Q_FUNC_INFO << "sending" << data.size () << "with buffer of size" << Buffer_.size ();

		IsWriting_ = true;

		Util::Sequence (this, Mgr_->WriteData (CallIdx_, fmt, Buffer_ + data)) >>
				[this] (auto&& result)
				{
					IsWriting_ = false;

					Buffer_.prepend (result.Leftover_);
				};

		Buffer_.clear ();

		return data.size ();
	}
}
}
}
