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

#pragma once

#include <boost/optional.hpp>
#include <QObject>
#include <interfaces/core/icoreproxy.h>

class QUrl;

namespace LeechCraft
{
struct Entity;

namespace LMP
{
	struct MediaInfo;
	class LocalCollection;
	class LocalFileResolver;
	class PlaylistManager;
	class SyncManager;
	class SyncUnmountableManager;
	class CloudUploadManager;
	class Player;
	class PreviewHandler;
	class ProgressManager;
	class RadioManager;

	class Core : public QObject
	{
		Q_OBJECT

		ICoreProxy_ptr Proxy_;

		LocalFileResolver *Resolver_;
		LocalCollection *Collection_;
		PlaylistManager *PLManager_;

		SyncManager *SyncManager_;
		SyncUnmountableManager *SyncUnmountableManager_;
		CloudUploadManager *CloudUpMgr_;

		ProgressManager *ProgressManager_;

		RadioManager *RadioManager_;

		Player *Player_;
		PreviewHandler *PreviewMgr_;

		QObjectList SyncPlugins_;
		QObjectList CloudPlugins_;

		Core ();
		Core (const Core&) = delete;
		Core (Core&&) = delete;
		Core& operator= (const Core&) = delete;
		Core& operator= (Core&&) = delete;
	public:
		static Core& Instance ();

		void SetProxy (ICoreProxy_ptr);
		ICoreProxy_ptr GetProxy ();

		void SendEntity (const Entity&);

		void PostInit ();
		void InitWithOtherPlugins ();

		void AddPlugin (QObject*);
		QObjectList GetSyncPlugins () const;
		QObjectList GetCloudStoragePlugins () const;

		LocalFileResolver* GetLocalFileResolver () const;
		LocalCollection* GetLocalCollection () const;
		PlaylistManager* GetPlaylistManager () const;
		SyncManager* GetSyncManager () const;
		SyncUnmountableManager* GetSyncUnmountableManager () const;
		CloudUploadManager* GetCloudUploadManager () const;
		ProgressManager* GetProgressManager () const;
		RadioManager* GetRadioManager () const;

		Player* GetPlayer () const;
		PreviewHandler* GetPreviewHandler () const;

		boost::optional<MediaInfo> TryURLResolve (const QUrl&) const;
	public slots:
		void rescan ();
	signals:
		void gotEntity (const LeechCraft::Entity&);

		void cloudStoragePluginsChanged ();

		void artistBrowseRequested (const QString&);
	};
}
}
