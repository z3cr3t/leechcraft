/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2012  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "localcollection.h"
#include <functional>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QtConcurrentMap>
#include <QtDebug>
#include "localcollectionstorage.h"
#include "core.h"
#include "util.h"
#include "localfileresolver.h"
#include "player.h"

namespace LeechCraft
{
namespace LMP
{
	namespace
	{
		template<LocalCollection::Role T>
		struct Role2Type;

		template<>
		struct Role2Type<LocalCollection::Role::ArtistName> { typedef QString ValueType; };

		template<>
		struct Role2Type<LocalCollection::Role::AlbumName> { typedef QString ValueType; };

		template<>
		struct Role2Type<LocalCollection::Role::AlbumYear> { typedef int ValueType; };

		template<>
		struct Role2Type<LocalCollection::Role::TrackNumber> { typedef int ValueType; };

		template<>
		struct Role2Type<LocalCollection::Role::TrackTitle> { typedef QString ValueType; };

		template<>
		struct Role2Type<LocalCollection::Role::TrackPath> { typedef QString ValueType; };

		template<>
		struct Role2Type<LocalCollection::Role::Node> { typedef int ValueType; };

		template<LocalCollection::Role Role = LocalCollection::Role::Node, LocalCollection::Role... Rest>
		bool RoleCompare (const QModelIndex& left, const QModelIndex& right)
		{
			if (Role == LocalCollection::Role::Node)
				return false;

			const auto& lData = left.data (Role).value<typename Role2Type<Role>::ValueType> ();
			const auto& rData = right.data (Role).value<typename Role2Type<Role>::ValueType> ();
			if (lData != rData)
				return lData < rData;
			else
				return RoleCompare<Rest...> (left, right);
		}

		class CollectionSorter : public QSortFilterProxyModel
		{
		public:
			CollectionSorter (QObject *parent)
			: QSortFilterProxyModel (parent)
			{
			}
		protected:
			bool lessThan (const QModelIndex& left, const QModelIndex& right) const
			{
				const auto type = left.data (LocalCollection::Role::Node).toInt ();
				switch (type)
				{
				case LocalCollection::NodeType::Artist:
					return RoleCompare<LocalCollection::Role::ArtistName> (left, right);
				case LocalCollection::NodeType::Album:
					return RoleCompare<LocalCollection::Role::AlbumYear,
								LocalCollection::Role::AlbumName> (left, right);
				case LocalCollection::NodeType::Track:
					return RoleCompare<LocalCollection::Role::TrackNumber,
								LocalCollection::Role::TrackTitle,
								LocalCollection::Role::TrackPath> (left, right);
				default:
					return QSortFilterProxyModel::lessThan (left, right);
				}
			}
		};
	}

	LocalCollection::LocalCollection (QObject *parent)
	: QObject (parent)
	, Storage_ (new LocalCollectionStorage (this))
	, CollectionModel_ (new QStandardItemModel (this))
	, Sorter_ (new CollectionSorter (this))
	, Watcher_ (new QFutureWatcher<MediaInfo> (this))
	{
		connect (Watcher_,
				SIGNAL (finished ()),
				this,
				SLOT (handleScanFinished ()));

		Artists_ = Storage_->Load ();
		Q_FOREACH (const auto& artist, Artists_)
			Q_FOREACH (auto album, artist.Albums_)
				Q_FOREACH (const auto& track, album->Tracks_)
					PresentPaths_ << track.FilePath_;

		AppendToModel (Artists_);

		Sorter_->setSourceModel (CollectionModel_);
		Sorter_->setDynamicSortFilter (true);
		Sorter_->sort (0);
	}

	QAbstractItemModel* LocalCollection::GetCollectionModel () const
	{
		return Sorter_;
	}

	void LocalCollection::Enqueue (const QModelIndex& index, Player *player)
	{
		player->Enqueue (CollectPaths (Sorter_->mapToSource (index)));
	}

	void LocalCollection::Scan (const QString& path)
	{
		auto resolver = Core::Instance ().GetLocalFileResolver ();
		auto paths = QSet<QString>::fromList (RecIterate (path));
		paths.subtract (PresentPaths_);
		if (paths.isEmpty ())
			return;

		PresentPaths_ += paths;
		auto worker = [resolver] (const QString& path) { return resolver->ResolveInfo (path); };
		QFuture<MediaInfo> future = QtConcurrent::mapped (paths,
				std::function<MediaInfo (const QString&)> (worker));
		Watcher_->setFuture (future);
	}

	QStringList LocalCollection::CollectPaths (const QModelIndex& index)
	{
		const auto type = index.data (Role::Node).toInt ();
		if (type == NodeType::Track)
			return QStringList (index.data (Role::TrackPath).toString ());

		QStringList paths;
		for (int i = 0; i < CollectionModel_->rowCount (index); ++i)
			paths += CollectPaths (CollectionModel_->index (i, 0, index));
		return paths;
	}

	namespace
	{
		template<typename T, typename U, typename Init, typename Parent>
		QStandardItem* GetItem (T& c, U idx, Init f, Parent parent)
		{
			if (c.contains (idx))
				return c [idx];

			auto item = new QStandardItem ();
			f (item);
			parent->appendRow (item);
			c [idx] = item;
			return item;
		}
	}

	void LocalCollection::AppendToModel (const Collection::Artists_t& artists)
	{
		Q_FOREACH (const auto& artist, artists)
		{
			auto artistItem = GetItem (Artist2Item_,
					artist.ID_,
					[&artist] (QStandardItem *item)
					{
						item->setText (artist.Name_);
						item->setData (artist.Name_, Role::ArtistName);
						item->setData (NodeType::Artist, Role::Node);
					},
					CollectionModel_);
			Q_FOREACH (auto album, artist.Albums_)
			{
				auto albumItem = GetItem (Album2Item_,
						album->ID_,
						[album] (QStandardItem *item)
						{
							item->setText (QString ("%1 - %2")
									.arg (album->Year_)
									.arg (album->Name_));
							item->setData (album->Year_, Role::AlbumYear);
							item->setData (album->Name_, Role::AlbumName);
							item->setData (NodeType::Album, Role::Node);
						},
						artistItem);

				Q_FOREACH (const auto& track, album->Tracks_)
				{
					const QString& name = QString ("%1 - %2")
							.arg (track.Number_)
							.arg (track.Name_);
					auto item = new QStandardItem (name);
					item->setData (track.Number_, Role::TrackNumber);
					item->setData (track.Name_, Role::TrackTitle);
					item->setData (track.FilePath_, Role::TrackPath);
					item->setData (NodeType::Track, Role::Node);
					albumItem->appendRow (item);
				}
			}
		}
	}

	void LocalCollection::handleScanFinished ()
	{
		auto future = Watcher_->future ();
		QList<MediaInfo> infos;
		std::copy (future.begin (), future.end (), std::back_inserter (infos));

		auto newArts = Storage_->AddToCollection (infos);
		AppendToModel (newArts);
	}
}
}
