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

#include "accountdatabase.h"
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>
#include <util/db/dblock.h>
#include <util/sll/qtutil.h>
#include "account.h"

bool operator< (const QStringList& left, const QStringList& right)
{
	if (left.size () != right.size ())
		return left.size () < right.size ();

	for (int i = 0; i < left.size (); ++i)
		if (left.at (i) != right.at (i))
			return left.at (i) < right.at (i);

	return false;
}

namespace LeechCraft
{
namespace Snails
{
	AccountDatabase::AccountDatabase (const QDir& dir, Account *acc, QObject *parent)
	: QObject { parent }
	, DB_ { std::make_shared<QSqlDatabase> (QSqlDatabase::addDatabase ("QSQLITE", "SnailsStorage_" + acc->GetID ())) }
	{
		if (!DB_->isValid ())
		{
			Util::DBLock::DumpError (DB_->lastError ());
			throw std::runtime_error ("Unable to add database connection.");
		}

		DB_->setDatabaseName (dir.filePath ("msgs.db"));
		if (!DB_->open ())
		{
			Util::DBLock::DumpError (DB_->lastError ());
			throw std::runtime_error (qPrintable (QString ("Could not initialize database: %1")
						.arg (DB_->lastError ().text ())));
		}

		InitTables ();
		PrepareQueries ();
		LoadKnownFolders ();
	}

	QList<QByteArray> AccountDatabase::GetIDs (const QStringList& folder)
	{
		QueryGetIds_.bindValue (":path", folder.join ("/"));
		Util::DBLock::Execute (QueryGetIds_);

		QList<QByteArray> result;
		while (QueryGetIds_.next ())
			result << QueryGetIds_.value (0).toByteArray ();
		QueryGetIds_.finish ();
		return result;
	}

	namespace
	{
		int GetCount (QSqlQuery& query, const QStringList& folder)
		{
			query.bindValue (":path", folder.join ("/"));
			Util::DBLock::Execute (query);
			if (!query.next ())
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to navigate to result";
				throw std::runtime_error ("Query execution failed.");
			}

			const auto result = query.value (0).toInt ();
			query.finish ();
			return result;
		}
	}

	int AccountDatabase::GetMessageCount (const QStringList& folder)
	{
		return GetCount (QueryGetCount_, folder);
	}

	int AccountDatabase::GetUnreadMessageCount (const QStringList& folder)
	{
		return GetCount (QueryGetUnreadCount_, folder);
	}

	int AccountDatabase::GetMessageCount ()
	{
		Util::DBLock::Execute (QueryGetTotalCount_);
		if (!QueryGetTotalCount_.next ())
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to navigate to result";
			throw std::runtime_error ("Query execution failed.");
		}

		const auto result = QueryGetTotalCount_.value (0).toInt ();
		QueryGetTotalCount_.finish ();
		return result;
	}

	bool AccountDatabase::HasMessage (const QByteArray& msgId, const QStringList& folder)
	{
		QueryGetMsgTableIdByFolder_.bindValue (":msgId", msgId);
		QueryGetMsgTableIdByFolder_.bindValue (":path", folder.join ("/"));
		Util::DBLock::Execute (QueryGetMsgTableIdByFolder_);
		if (!QueryGetMsgTableIdByFolder_.next ())
			return false;

		QueryGetMsgTableIdByFolder_.finish ();
		return true;
	}

	void AccountDatabase::AddMessage (const Message_ptr& msg)
	{
		for (const auto& folder : msg->GetFolders ())
			AddFolder (folder);

		Util::DBLock lock { *DB_ };
		lock.Init ();

		for (const auto& folder : msg->GetFolders ())
		{
			if (HasMessage (msg->GetFolderID (), folder))
				continue;

			const auto msgTableId = AddMessageUnfoldered (msg);
			AddMessageToFolder (msgTableId, GetFolder (folder), msg->GetFolderID ());
		}

		lock.Good ();
	}

	void AccountDatabase::RemoveMessage (const QByteArray& msgId, const QStringList& folder,
			const std::function<void ()>& continuation)
	{
		Util::DBLock lock { *DB_ };
		lock.Init ();

		QueryRemoveMessage_.bindValue (":msgId", msgId);
		QueryRemoveMessage_.bindValue (":path", folder.join ("/"));
		Util::DBLock::Execute (QueryRemoveMessage_);

		if (continuation)
			continuation ();

		lock.Good ();
	}

	int AccountDatabase::AddMessageUnfoldered (const Message_ptr& msg)
	{
		QueryAddMsgUnfoldered_.bindValue (":isRead", msg->IsRead ());
		QueryAddMsgUnfoldered_.bindValue (":uniqueId", msg->GetMessageID ());
		Util::DBLock::Execute (QueryAddMsgUnfoldered_);

		const auto& idVar = QueryAddMsgUnfoldered_.lastInsertId ();
		if (!idVar.isValid ())
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown insert ID";
			throw std::runtime_error ("Unable to obtain last insert ID.");
		}
		return idVar.toInt ();
	}

	void AccountDatabase::AddMessageToFolder (int msgTableId, int folderTableId, const QByteArray& msgId)
	{
		QueryAddMsgToFolder_.bindValue (":msgId", msgId);
		QueryAddMsgToFolder_.bindValue (":msgTableId", msgTableId);
		QueryAddMsgToFolder_.bindValue (":folderId", folderTableId);
		Util::DBLock::Execute (QueryAddMsgToFolder_);
	}

	void AccountDatabase::InitTables ()
	{
		QHash<QString, QStringList> table2queries;
		table2queries ["messages"] <<
				R"d(
					CREATE TABLE messages (
					Id INTEGER PRIMARY KEY AUTOINCREMENT,
					UniqueId TEXT UNIQUE,
					IsRead BOOL NOT NULL
					);
				)d";
		table2queries ["folders"] <<
				R"d(
					CREATE TABLE folders (
					Id INTEGER PRIMARY KEY AUTOINCREMENT,
					FolderPath TEXT UNIQUE NOT NULL
					)
				)d";
		table2queries ["msg2folder"] <<
				R"d(
					CREATE TABLE msg2folder (
					Id INTEGER PRIMARY KEY AUTOINCREMENT,
					MsgId INTEGER NOT NULL REFERENCES messages (Id) ON DELETE CASCADE,
					FolderId INTEGER NOT NULL REFERENCES folders (Id) ON DELETE CASCADE,
					FolderMessageId TEXT NOT NULL
					)
				)d";

		QSqlQuery query { *DB_ };
		for (const auto& pair : Util::Stlize (table2queries))
			if (!DB_->tables ().contains (pair.first))
				for (const auto& queryStr : pair.second)
					if (!query.exec (queryStr))
					{
						Util::DBLock::DumpError (query);
						throw std::runtime_error ("Query execution failed for storage creation.");
					}

		query.exec ("PRAGMA foreign_keys = ON;");
		query.exec ("PRAGMA synchronous = OFF;");
	}

	void AccountDatabase::PrepareQueries ()
	{
		QueryGetIds_ = QSqlQuery { *DB_ };
		QueryGetIds_.prepare (R"d(
					SELECT msg2folder.FolderMessageId FROM msg2folder, folders
					WHERE folders.FolderPath = :path
					AND folders.Id = msg2folder.FolderId
				)d");

		QueryGetCount_ = QSqlQuery { *DB_ };
		QueryGetCount_.prepare (R"d(
					SELECT COUNT(1) FROM msg2folder, folders
					WHERE folders.FolderPath = :path
					AND folders.Id = msg2folder.FolderId
				)d");

		QueryGetTotalCount_ = QSqlQuery { *DB_ };
		QueryGetTotalCount_.prepare ("SELECT COUNT(1) FROM messages");

		QueryRemoveMessage_ = QSqlQuery { *DB_ };
		QueryRemoveMessage_.prepare (R"d(
					DELETE FROM msg2folder
					WHERE Id =
						(SELECT msg2folder.Id
							FROM msg2folder, folders
							WHERE msg2folder.FolderMessageId = :msgId
							AND folders.FolderPath = :path
							AND msg2folder.FolderId = folders.Id)
				)d");

		QueryAddFolder_ = QSqlQuery { *DB_ };
		QueryAddFolder_.prepare ("INSERT INTO folders (FolderPath) VALUES (:path);");

		QueryGetMsgTableIdByFolder_ = QSqlQuery { *DB_ };
		QueryGetMsgTableIdByFolder_.prepare (R"d(
					SELECT msg2folder.MsgId FROM msg2folder, folders
					WHERE msg2folder.FolderId = folders.Id
					AND folders.FolderPath = :path
					AND msg2folder.FolderMessageId = :msgId
				)d");

		QueryAddMsgUnfoldered_ = QSqlQuery { *DB_ };
		QueryAddMsgUnfoldered_.prepare (R"d(
					INSERT OR REPLACE INTO messages
					(UniqueId, IsRead)
					VALUES
					(:uniqueId, :isRead)
				)d");

		QueryAddMsgToFolder_ = QSqlQuery { *DB_ };
		QueryAddMsgToFolder_.prepare (R"d(
					INSERT INTO msg2folder
					(MsgId, FolderId, FolderMessageId)
					VALUES
					(:msgTableId, :folderId, :msgId)
				)d");
	}

	int AccountDatabase::AddFolder (const QStringList& folder)
	{
		if (KnownFolders_.contains (folder))
			return KnownFolders_.value (folder);

		QueryAddFolder_.bindValue (":path", folder.join ("/"));
		Util::DBLock::Execute (QueryAddFolder_);

		const auto& idVar = QueryAddFolder_.lastInsertId ();
		if (!idVar.isValid ())
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown insert ID";
			throw std::runtime_error ("Unable to obtain last insert ID.");
		}

		const auto id = idVar.toInt ();
		KnownFolders_ [folder] = id;
		return id;
	}

	int AccountDatabase::GetFolder (const QStringList& folder) const
	{
		if (!KnownFolders_.contains (folder))
			throw std::runtime_error ("Unknown folder");

		return KnownFolders_.value (folder);
	}

	void AccountDatabase::LoadKnownFolders ()
	{
		QSqlQuery query { *DB_ };
		query.prepare ("SELECT Id, FolderPath FROM folders");
		Util::DBLock::Execute (query);

		while (query.next ())
		{
			const auto id = query.value (0).toInt ();
			const auto& path = query.value (1).toString ().split ('/');

			KnownFolders_ [path] = id;
		}
	}
}
}
