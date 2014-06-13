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

#pragma once

#include <memory>
#include <QObject>
#include <QHash>
#include "message.h"
#include "progresslistener.h"

class QMutex;
class QAbstractItemModel;
class QStandardItemModel;
class QStandardItem;
class QModelIndex;

namespace LeechCraft
{
namespace Snails
{
	class AccountThread;
	class AccountThreadWorker;
	class AccountFolderManager;
	class MailModel;

	class Account : public QObject
	{
		Q_OBJECT

		friend class AccountThreadWorker;
		AccountThread * const Thread_;
		QMutex * const AccMutex_;

		QByteArray ID_;

		QString AccName_;
		QString UserName_;
		QString UserEmail_;

		QString Login_;
		bool UseSASL_;
		bool SASLRequired_;

	public:
		enum class SecurityType
		{
			TLS,
			SSL,
			No
		};
	private:
		bool UseTLS_;
		bool UseSSL_;
		bool InSecurityRequired_;

		SecurityType OutSecurity_;
		bool OutSecurityRequired_;

		bool SMTPNeedsAuth_;
		bool APOP_;
		bool APOPFail_;

		QString InHost_;
		int InPort_;

		QString OutHost_;
		int OutPort_;

		QString OutLogin_;
	public:
		enum class Direction
		{
			In,
			Out
		};

		enum class InType
		{
			IMAP,
			POP3,
			Maildir
		};

		enum class OutType
		{
			SMTP,
			Sendmail
		};

		enum FetchFlag
		{
			FetchAll = 0x01,
			FetchNew = 0x02
		};

		Q_DECLARE_FLAGS (FetchFlags, FetchFlag);
	private:
		InType InType_;
		OutType OutType_;

		AccountFolderManager *FolderManager_;
		QStandardItemModel *FoldersModel_;

		MailModel * const MailModel_;

		enum FoldersRole
		{
			Path = Qt::UserRole + 1
		};
	public:
		Account (QObject* = 0);

		QByteArray GetID () const;
		QString GetName () const;
		QString GetServer () const;
		QString GetType () const;

		AccountFolderManager* GetFolderManager () const;
		QAbstractItemModel* GetMailModel () const;
		QAbstractItemModel* GetFoldersModel () const;

		void ShowFolder (const QModelIndex&);
		void Synchronize (FetchFlags);
		void Synchronize (const QStringList&);

		void FetchWholeMessage (Message_ptr);
		void SendMessage (Message_ptr);
		void FetchAttachment (Message_ptr,
				const QString&, const QString&);

		void Update (const Message_ptr&);

		QByteArray Serialize () const;
		void Deserialize (const QByteArray&);

		void OpenConfigDialog ();

		bool IsNull () const;

		QString GetInUsername ();
		QString GetOutUsername ();
	private:
		QMutex* GetMutex () const;

		QString BuildInURL ();
		QString BuildOutURL ();
		QString GetPassImpl (Direction);
		QByteArray GetStoreID (Direction) const;
	private slots:
		void buildInURL (QString*);
		void buildOutURL (QString*);
		void getPassword (QString*, Direction = Direction::In);
		void handleMsgHeaders (QList<Message_ptr>);
		void handleGotUpdatedMessages (QList<Message_ptr>);
		void handleGotOtherMessages (QList<QByteArray>, QStringList);
		void handleGotFolders (QList<QStringList>);
		void handleFoldersUpdated ();
		void handleMessageBodyFetched (Message_ptr);
	signals:
		void mailChanged ();
		void gotProgressListener (ProgressListener_g_ptr);
		void accountChanged ();
		void messageBodyFetched (Message_ptr);
	};

	typedef std::shared_ptr<Account> Account_ptr;
}
}

Q_DECLARE_METATYPE (LeechCraft::Snails::Account::FetchFlags);
Q_DECLARE_METATYPE (LeechCraft::Snails::Account_ptr);
