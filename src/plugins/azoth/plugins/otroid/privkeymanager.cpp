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

#include "privkeymanager.h"
#include <QStandardItemModel>
#include <QMessageBox>
#include <QtDebug>

extern "C"
{
#include <libotr/privkey.h>
}

#include <interfaces/azoth/iproxyobject.h>
#include <interfaces/azoth/iaccount.h>
#include <interfaces/azoth/iprotocol.h>
#include "util.h"

namespace LeechCraft
{
namespace Azoth
{
namespace OTRoid
{
	PrivKeyManager::PrivKeyManager (const OtrlUserState state, IProxyObject *proxy)
	: UserState_ { state }
	, AzothProxy_ { proxy }
	, Model_ { new QStandardItemModel { this } }
	{
	}

	QAbstractItemModel* PrivKeyManager::GetModel () const
	{
		return Model_;
	}

	void PrivKeyManager::reloadAll ()
	{
		Model_->clear ();
		Model_->setHorizontalHeaderLabels ({ tr ("Account"), tr ("Private key") });

		QHash<QString, QString> accId2key;
		for (auto pkey = UserState_->privkey_root; pkey; pkey = pkey->next)
		{
			char fpHash [OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
			if (!otrl_privkey_fingerprint (UserState_,
						fpHash, pkey->accountname, pkey->protocol))
				continue;

			const auto& accId = QString::fromUtf8 (pkey->accountname);
			accId2key [accId] = QString::fromUtf8 (fpHash);
		}

		for (const auto accObj : AzothProxy_->GetAllAccounts ())
		{
			const auto acc = qobject_cast<IAccount*> (accObj);
			if (!acc)
				continue;

			const auto& hash = accId2key.value (acc->GetAccountID ());

			QList<QStandardItem*> row
			{
				new QStandardItem { acc->GetAccountName () },
				new QStandardItem { hash }
			};

			const auto proto = qobject_cast<IProtocol*> (acc->GetParentProtocol ());

			for (auto item : row)
			{
				item->setEditable (false);
				item->setData (acc->GetAccountID (), RoleAccId);
				item->setData (proto->GetProtocolID (), RoleProtoId);
			}

			row.value (ColumnKey)->setFont (QFont { "Monospace" });

			Model_->appendRow (row);
		}
	}

	void PrivKeyManager::removeRequested (const QString&, QModelIndexList indexes)
	{
		for (auto i = indexes.begin (); i != indexes.end (); )
		{
			auto index = *i;
			index = index.sibling (index.row (), ColumnKey);
			if (index.data ().toString ().isEmpty ())
				i = indexes.erase (i);
			else
				++i;
		}

		if (indexes.isEmpty ())
			return;

		if (QMessageBox::question (nullptr,
					tr ("Confirm private keys deletion"),
					tr ("Are you sure you want to delete %n private key(s)?", 0, indexes.size ()),
					QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;

		for (const auto& index : indexes)
		{
			const auto& accId = index.data (RoleAccId).toString ();
			const auto& protoId = index.data (RoleProtoId).toString ();

			const auto pkey = otrl_privkey_find (UserState_,
					accId.toUtf8 ().constData (),
					protoId.toUtf8 ().constData ());
			if (!pkey)
			{
				qWarning () << Q_FUNC_INFO
						<< "cannot find private key for"
						<< accId
						<< protoId;
				continue;
			}

			otrl_privkey_forget (pkey);
		}

		emit keysChanged ();
	}

	void PrivKeyManager::customButtonPressed (const QString&, const QByteArray& id, int)
	{
		if (id == "refresh")
			reloadAll ();
	}
}
}
}
