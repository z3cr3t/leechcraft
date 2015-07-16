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

#include "filetransferin.h"
#include <QFile>
#include <tox/tox.h>
#include "toxthread.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	FileTransferIn::FileTransferIn (const QString& azothId,
			const QByteArray& pubkey,
			int friendNum,
			int fileNum,
			qint64 filesize,
			const QString& offeredName,
			const std::shared_ptr<ToxThread>& thread,
			QObject *parent)
	: FileTransferBase { azothId, pubkey, thread, parent }
	, FriendNum_ { friendNum }
	, FileNum_ { fileNum }
	, Filename_ { offeredName }
	, Filesize_ { filesize }
	{
	}

	QString FileTransferIn::GetName () const
	{
		return Filename_;
	}

	qint64 FileTransferIn::GetSize () const
	{
		return Filesize_;
	}

	TransferDirection FileTransferIn::GetDirection () const
	{
		return TDIn;
	}

	void FileTransferIn::Accept (const QString& dirName)
	{
		const auto& outName = dirName + '/' + Filename_;
		File_ = std::make_shared<QFile> (outName);
		if (!File_->open (QIODevice::WriteOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open"
					<< outName
					<< "for write"
					<< File_->errorString ();
			emit errorAppeared (TEFileAccessError, File_->errorString ());
			emit stateChanged (TSFinished);
			return;
		}

		Thread_->ScheduleFunction ([this] (Tox *tox)
				{
					TOX_ERR_FILE_CONTROL error {};
					if (!tox_file_control (tox, FriendNum_, FileNum_, TOX_FILE_CONTROL_RESUME, &error))
						throw MakeCommandCodeException ("tox_file_control", error);
				});
	}

	void FileTransferIn::Abort ()
	{
		Thread_->ScheduleFunction ([this] (Tox *tox)
				{
					TOX_ERR_FILE_CONTROL error {};
					tox_file_control (tox, FriendNum_, FileNum_, TOX_FILE_CONTROL_CANCEL, &error);
				});
	}

	void FileTransferIn::handleData (qint32 friendNum, qint8 fileNum, const QByteArray& data)
	{
		if (friendNum != FriendNum_ || fileNum != FileNum_)
			return;

		File_->write (data);
		emit transferProgress (File_->pos (), Filesize_);
	}

	void FileTransferIn::handleFileControl (qint32 friendNum,
			qint8 fileNum, qint8 type, const QByteArray&)
	{
		if (friendNum != FriendNum_ || fileNum != FileNum_)
			return;

		switch (type)
		{
		case TOX_FILECONTROL_FINISHED:
			Thread_->ScheduleFunction ([this] (Tox *tox)
					{ tox_file_send_control (tox, FriendNum_, 1, FileNum_, TOX_FILECONTROL_FINISHED, nullptr, 0); });
			emit stateChanged (TSFinished);
			break;
		case TOX_FILECONTROL_KILL:
			emit errorAppeared (TEAborted, tr ("Remote party aborted the transfer."));
			emit stateChanged (TSFinished);
			break;
		default:
			qWarning () << Q_FUNC_INFO
					<< "unknown filecontrol type"
					<< type;
			break;
		}
	}
}
}
}
