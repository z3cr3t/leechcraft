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

#include "foldersmodel.h"
#include <algorithm>
#include <QtDebug>

namespace LeechCraft
{
namespace Snails
{
	struct FolderDescr
	{
		QString Name_;

		std::weak_ptr<FolderDescr> Parent_;
		QList<FolderDescr_ptr> Children_;

		FolderDescr () = default;
		FolderDescr (const QString& name, const std::weak_ptr<FolderDescr>& parent);

		QList<FolderDescr_ptr>::iterator Find (const QString& name);
	};

	FolderDescr::FolderDescr (const QString& name, const std::weak_ptr<FolderDescr>& parent)
	: Name_ { name }
	, Parent_ { parent }
	{
	}

	QList<FolderDescr_ptr>::iterator FolderDescr::Find (const QString& name)
	{
		return std::find_if (Children_.begin (), Children_.end (),
				[&name] (const FolderDescr_ptr& descr) { return descr->Name_ == name; });
	}

	FoldersModel::FoldersModel (QObject *parent)
	: QAbstractItemModel { parent }
	, Headers_ { tr ("Messages") }
	, RootFolder_ { new FolderDescr {} }
	{
	}

	QVariant FoldersModel::headerData (int section, Qt::Orientation orient, int role) const
	{
		if (orient != Qt::Horizontal || role != Qt::DisplayRole)
			return {};

		return Headers_.value (section);
	}

	int FoldersModel::columnCount (const QModelIndex&) const
	{
		return Headers_.size ();
	}

	QVariant FoldersModel::data (const QModelIndex& index, int role) const
	{
		const auto folder = static_cast<FolderDescr*> (index.internalPointer ());

		switch (role)
		{
		case Qt::DisplayRole:
			break;
		case Role::FolderPath:
		{
			QStringList path { folder->Name_ };
			auto wItem = folder->Parent_;
			while (wItem.lock () != RootFolder_)
			{
				const auto& item = wItem.lock ();
				path.prepend (item->Name_);
				wItem = item->Parent_;
			}
			return path;
		}
		default:
			return {};
		}

		return folder->Name_;
	}

	QModelIndex FoldersModel::index (int row, int column, const QModelIndex& parent) const
	{
		if (!hasIndex (row, column, parent))
			return {};

		const auto folder = parent.isValid () ?
				static_cast<FolderDescr*> (parent.internalPointer ()) :
				RootFolder_.get ();

		return createIndex (row, column, folder->Children_.value (row).get ());
	}

	QModelIndex FoldersModel::parent (const QModelIndex& index) const
	{
		const auto folder = static_cast<FolderDescr*> (index.internalPointer ());
		const auto parentFolder = folder->Parent_.lock ().get ();
		if (parentFolder == RootFolder_.get ())
			return {};

		const auto& parentParent = parentFolder->Parent_.lock ();
		const auto parentPos = std::find_if (parentParent->Children_.begin (),
				parentParent->Children_.end (),
				[&parentFolder] (const std::shared_ptr<FolderDescr>& folder)
				{
					return folder.get () == parentFolder;
				});
		if (parentPos == parentParent->Children_.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "could not determine parent's folder";
			return {};
		}

		const auto row = std::distance (parentParent->Children_.begin (), parentPos);

		return createIndex (row, 0, parentFolder);
	}

	int FoldersModel::rowCount (const QModelIndex& parent) const
	{
		const auto folder = parent.isValid () ?
				static_cast<FolderDescr*> (parent.internalPointer ()) :
				RootFolder_.get ();
		return folder->Children_.size ();
	}

	void FoldersModel::SetFolders (const QList<QStringList>& folders)
	{
		if (const auto rc = RootFolder_->Children_.size ())
		{
			beginRemoveRows ({}, 0, rc - 1);
			RootFolder_->Children_.clear ();
			endRemoveRows ();
		}

		auto newRoot = std::make_shared<FolderDescr> ();
		for (const auto& folder : folders)
		{
			auto currentRoot = newRoot;
			for (const auto& component : folder)
			{
				const auto componentDescrPos = currentRoot->Find (component);
				if (componentDescrPos != currentRoot->Children_.end ())
				{
					currentRoot = *componentDescrPos;
					continue;
				}

				const auto& componentDescr = std::make_shared<FolderDescr> (component, currentRoot);
				currentRoot->Children_.append (componentDescr);
				currentRoot = componentDescr;
			}
		}

		if (const auto newRc = newRoot->Children_.size ())
		{
			beginInsertRows ({}, 0, newRc - 1);
			RootFolder_ = newRoot;
			endInsertRows ();
		}
	}
}
}
