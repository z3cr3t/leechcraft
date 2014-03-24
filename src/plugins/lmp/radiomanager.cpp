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

#include "radiomanager.h"
#include <QStandardItemModel>
#include <QInputDialog>
#include <QTimer>
#include <QMimeData>
#include <QtDebug>
#include <interfaces/media/iradiostationprovider.h>
#include <interfaces/media/iaudiopile.h>
#include <interfaces/media/imodifiableradiostation.h>
#include <interfaces/core/ipluginsmanager.h>
#include "core.h"
#include "player.h"
#include "previewhandler.h"
#include "xmlsettingsmanager.h"
#include "radiocustomstreams.h"

namespace LeechCraft
{
namespace LMP
{
	namespace
	{
		enum RadioWidgetRole
		{
			PileObject = Media::RadioItemRole::MaxRadioRole + 1
		};

		namespace
		{
			template<typename T>
			void Serialize (QMimeData *mimeData, const QString& name, const T& t)
			{
				QByteArray infosData;
				QDataStream ostr (&infosData, QIODevice::WriteOnly);
				ostr << t;

				mimeData->setData (name, infosData);
			}
		}

		class RadioModel : public QStandardItemModel
		{
			RadioManager * const Manager_;
		public:
			RadioModel (RadioManager *manager)
			: QStandardItemModel { manager }
			, Manager_ { manager }
			{
				setSupportedDragActions (Qt::CopyAction | Qt::MoveAction);
			}

			QStringList mimeTypes () const
			{
				return { "text/uri-list" };
			}

			QMimeData* mimeData (const QModelIndexList& indexes) const
			{
				QList<QUrl> urls;
				QList<MediaInfo> infos;

				for (const auto& index : indexes)
					for (const auto& info : Manager_->GetSources (index))
					{
						urls << info.Other_ ["URL"].toUrl ();
						infos << MediaInfo::FromAudioInfo (info);
					}

				urls.removeAll ({});

				if (urls.isEmpty ())
					return nullptr;

				auto result = new QMimeData;
				result->setUrls (urls);

				Serialize (result, "x-leechcraft-lmp/media-info-list", infos);

				return result;
			}
		};
	}

	RadioManager::RadioManager (QObject *parent)
	: QObject (parent)
	, StationsModel_ (new RadioModel (this))
	, AutoRefreshTimer_ (new QTimer (this))
	{
		XmlSettingsManager::Instance ().RegisterObject ({ "AutoRefreshRadios",
					"RadioRefreshTimeout" },
				this, "handleRefreshSettingsChanged");
		handleRefreshSettingsChanged ();

		connect (AutoRefreshTimer_,
				SIGNAL (timeout ()),
				this,
				SLOT (refreshAll ()));
	}

	void RadioManager::InitProviders ()
	{
		InitProvider (new RadioCustomStreams (this));

		auto pm = Core::Instance ().GetProxy ()->GetPluginsManager ();
		auto pileObjs = pm->GetAllCastableRoots<Media::IAudioPile*> ();
		for (auto pileObj : pileObjs)
		{
			auto pile = qobject_cast<Media::IAudioPile*> (pileObj);

			auto item = new QStandardItem (tr ("Search in %1")
					.arg (pile->GetServiceName ()));
			item->setIcon (pile->GetServiceIcon ());
			item->setEditable (false);
			item->setData (QVariant::fromValue (pileObj), RadioWidgetRole::PileObject);

			StationsModel_->appendRow (item);
		}

		auto providerObjs = pm->GetAllCastableRoots<Media::IRadioStationProvider*> ();
		for (auto provObj : providerObjs)
			InitProvider (provObj);
	}

	QAbstractItemModel* RadioManager::GetModel () const
	{
		return StationsModel_;
	}

	namespace
	{
		QStandardItem* GetRootItem (QStandardItem *item)
		{
			auto root = item;
			while (auto parent = root->parent ())
				root = parent;
			return root;
		}
	}

	void RadioManager::Refresh (const QModelIndex& index)
	{
		const auto item = StationsModel_->itemFromIndex (index);
		if (item->data (RadioWidgetRole::PileObject).value<QObject*> ())
			return;

		const auto root = GetRootItem (item);
		if (!Root2Prov_.contains (root))
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown provider for index"
					<< index;
			return;
		}

		Root2Prov_ [root]->RefreshItems ({ item });
	}

	void RadioManager::AddUrl (const QModelIndex& index, const QUrl& url, const QString& name)
	{
		const auto item = StationsModel_->itemFromIndex (index);
		const auto root = GetRootItem (item);
		if (!Root2Prov_.contains (root))
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown provider for index"
					<< index;
			return;
		}

		const auto radio = Root2Prov_ [root]->GetRadioStation (item, {});
		if (!radio)
		{
			qWarning () << Q_FUNC_INFO
					<< "got a null radio station from provider";
			return;
		}

		auto modifiable = qobject_cast<Media::IModifiableRadioStation*> (radio->GetQObject ());
		if (!modifiable)
		{
			qWarning () << Q_FUNC_INFO
					<< radio->GetRadioName ()
					<< "is not modifiable";
			return;
		}

		modifiable->AddItem (url, name);
	}

	void RadioManager::RemoveUrl (const QModelIndex& index)
	{
		const auto item = StationsModel_->itemFromIndex (index);
		const auto root = GetRootItem (item);
		if (!Root2Prov_.contains (root))
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown provider for index"
					<< index;
			return;
		}

		const auto radio = Root2Prov_ [root]->GetRadioStation (item, {});
		if (!radio)
		{
			qWarning () << Q_FUNC_INFO
					<< "got a null radio station from provider";
			return;
		}

		auto modifiable = qobject_cast<Media::IModifiableRadioStation*> (radio->GetQObject ());
		if (!modifiable)
		{
			qWarning () << Q_FUNC_INFO
					<< radio->GetRadioName ()
					<< "is not modifiable";
			return;
		}

		modifiable->RemoveItem (index);
	}

	void RadioManager::Handle (const QModelIndex& index, Player *player)
	{
		const auto item = StationsModel_->itemFromIndex (index);

		if (const auto pileObj = item->data (RadioWidgetRole::PileObject).value<QObject*> ())
			return HandlePile (item, pileObj);

		const auto root = GetRootItem (item);
		if (!Root2Prov_.contains (root))
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown provider for index"
					<< index;
			return;
		}

		QString param;

		const auto intRadioType = item->data (Media::RadioItemRole::ItemType).toInt ();
		switch (static_cast<Media::RadioType> (intRadioType))
		{
		case Media::RadioType::None:
			return;
		case Media::RadioType::Predefined:
		case Media::RadioType::CustomAddableStreams:
		case Media::RadioType::RadioAction:
			break;
		case Media::RadioType::SimilarArtists:
			param = QInputDialog::getText (0,
					tr ("Similar artists radio"),
					tr ("Enter artist name for which to tune the similar artists radio station:"));
			if (param.isEmpty ())
				return;
			break;
		case Media::RadioType::GlobalTag:
			param = QInputDialog::getText (0,
					tr ("Global tag radio"),
					tr ("Enter global tag name:"));
			if (param.isEmpty ())
				return;
			break;
		case Media::RadioType::TracksList:
		case Media::RadioType::SingleTrack:
		{
			QList<AudioSource> sources;
			for (const auto& info : GetSources (index))
			{
				const auto& url = info.Other_ ["URL"].toUrl ();
				player->PrepareURLInfo (url, MediaInfo::FromAudioInfo (info));
				sources << url;
			}
			player->Enqueue (sources, Player::EnqueueNone);
			return;
		}
		}

		if (auto station = Root2Prov_ [root]->GetRadioStation (item, param))
			player->SetRadioStation (station);
	}

	void RadioManager::HandleWokeUp ()
	{
		if (XmlSettingsManager::Instance ().property ("RefreshRadioOnWakeup").toBool ())
			QTimer::singleShot (15000,
					this,
					SLOT (refreshAll ()));
	}

	QList<Media::AudioInfo> RadioManager::GetSources (const QModelIndex& index) const
	{
		const auto intRadioType = index.data (Media::RadioItemRole::ItemType).toInt ();
		switch (static_cast<Media::RadioType> (intRadioType))
		{
		case Media::RadioType::TracksList:
		case Media::RadioType::SingleTrack:
		{
			QList<Media::AudioInfo> result;

			const auto& infosVar = index.data (Media::RadioItemRole::TracksInfos);
			for (const auto& info : infosVar.value<QList<Media::AudioInfo>> ())
			{
				const auto& url = info.Other_ ["URL"].toUrl ();
				if (!url.isValid ())
				{
					qWarning () << Q_FUNC_INFO
							<< "ignoring invalid URL"
							<< info.Other_;
					continue;
				}

				result << info;
			}

			return result;
		}
		default:
			return {};
		}
	}

	void RadioManager::InitProvider (QObject *provObj)
	{
		auto prov = qobject_cast<Media::IRadioStationProvider*> (provObj);
		for (auto item : prov->GetRadioListItems ())
		{
			StationsModel_->appendRow (item);
			Root2Prov_ [item] = prov;
		}
	}

	void RadioManager::HandlePile (QStandardItem*, QObject *pileObj)
	{
		const auto& query = QInputDialog::getText (0,
				tr ("Audio search"),
				tr ("Enter the string to search for:"));
		if (query.isEmpty ())
			return;

		Media::AudioSearchRequest req;
		req.FreeForm_ = query;

		const auto pending = qobject_cast<Media::IAudioPile*> (pileObj)->Search (req);
		Core::Instance ().GetPreviewHandler ()->HandlePending (pending);
	}

	void RadioManager::refreshAll ()
	{
		for (auto prov : Root2Prov_)
			prov->RefreshItems (prov->GetRadioListItems ());
	}

	void RadioManager::handleRefreshSettingsChanged ()
	{
		AutoRefreshTimer_->stop ();

		const auto interval = XmlSettingsManager::Instance ()
				.property ("RadioRefreshTimeout").toInt ();
		AutoRefreshTimer_->setInterval (interval * 60 * 60 * 1000);

		if (XmlSettingsManager::Instance ().property ("AutoRefreshRadios").toBool ())
			AutoRefreshTimer_->start ();
	}
}
}
