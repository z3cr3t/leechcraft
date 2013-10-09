/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "syncmanager.h"
#include <QStringList>
#include <QtDebug>
#include <QFileInfo>
#include <util/util.h>
#include "copymanager.h"
#include "../core.h"
#include "../localfileresolver.h"
#include "../util.h"

namespace LeechCraft
{
namespace LMP
{
	SyncManager::SyncManager (QObject *parent)
	: SyncManagerBase (parent)
	{
	}

	void SyncManager::AddFiles (ISyncPlugin *syncer, const QString& mount,
			const QStringList& files, const TranscodingParams& params)
	{
		std::for_each (files.begin (), files.end (),
				[this, syncer, &mount] (decltype (files.front ()) file)
					{ Source2Params_ [file] = { syncer, mount }; });

		SyncManagerBase::AddFiles (files, params);
	}

	void SyncManager::CreateSyncer (const QString& mount)
	{
		auto mgr = new CopyManager<CopyJob> (this);
		connect (mgr,
				SIGNAL (startedCopying (QString)),
				this,
				SLOT (handleStartedCopying (QString)));
		connect (mgr,
				SIGNAL (finishedCopying ()),
				this,
				SLOT (handleFinishedCopying ()));
		connect (mgr,
				SIGNAL (copyProgress (qint64, qint64)),
				this,
				SLOT (handleCopyProgress (qint64, qint64)));
		connect (mgr,
				SIGNAL (errorCopying (QString, QString)),
				this,
				SLOT (handleErrorCopying (QString, QString)));
		Mount2Copiers_ [mount] = mgr;
	}

	namespace
	{
		bool FixMask (QString& mask, const QString& transcoded)
		{
			MediaInfo info;
			try
			{
				info = Core::Instance ().GetLocalFileResolver ()->ResolveInfo (transcoded);
			}
			catch (const std::exception& e)
			{
				qWarning () << Q_FUNC_INFO
						<< e.what ();
				return false;
			}

			auto fix = [] (QString& str) -> void
			{
				str.replace ('/', '_');
				str.replace ('?', '_');
				str.replace ('*', '_');
			};
			fix (info.Album_);
			fix (info.Artist_);
			fix (info.Title_);

			mask = PerformSubstitutions (mask, info);
			const auto& ext = QFileInfo (transcoded).suffix ();
			if (!mask.endsWith (ext))
				mask += "." + ext;

			return true;
		}
	}

	void SyncManager::handleFileTranscoded (const QString& from,
			const QString& transcoded, QString mask)
	{
		SyncManagerBase::HandleFileTranscoded (from, transcoded);

		const auto& syncTo = Source2Params_.take (from);
		if (syncTo.MountPath_.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "dumb transcoded file detected"
					<< from
					<< transcoded;
			return;
		}

		emit uploadLog (tr ("File %1 successfully transcoded, adding to copy queue for the device %2...")
				.arg ("<em>" + QFileInfo (from).fileName () + "</em>")
				.arg ("<em>" + syncTo.MountPath_) + "</em>");

		if (!FixMask (mask, transcoded))
			return;

		if (!Mount2Copiers_.contains (syncTo.MountPath_))
			CreateSyncer (syncTo.MountPath_);
		const CopyJob copyJob
		{
			transcoded,
			from != transcoded,
			syncTo.Syncer_,
			from,
			syncTo.MountPath_,
			mask
		};
		Mount2Copiers_ [syncTo.MountPath_]->Copy (copyJob);
	}
}
}
