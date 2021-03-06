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

#include "maillistview.h"

#if QT_VERSION < 0x050000
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#else
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#endif

#include <QStandardItemModel>
#include <QtDebug>
#include <util/sys/paths.h>
#include <util/gui/unhoverdeletemixin.h>
#include <util/qml/colorthemeproxy.h>
#include <util/models/rolenamesmixin.h>

namespace LeechCraft
{
namespace GmailNotifier
{
	namespace
	{
		class MailListModel : public Util::RoleNamesMixin<QStandardItemModel>
		{
		public:
			enum Roles
			{
				Subject = Qt::UserRole + 1,
				Summary,
				ModifiedDate,
				AuthorName,
				AuthorEmail
			};

			MailListModel (QObject *parent)
			: RoleNamesMixin<QStandardItemModel> (parent)
			{
				QHash<int, QByteArray> roleNames;
				roleNames [Subject] = "subject";
				roleNames [Summary] = "summary";
				roleNames [ModifiedDate] = "modifiedDate";
				roleNames [AuthorName] = "authorName";
				roleNames [AuthorEmail] = "authorEmail";
				setRoleNames (roleNames);
			}
		};
	}

	MailListView::MailListView (const ConvInfos_t& infos, ICoreProxy_ptr proxy, QWidget *parent)
#if QT_VERSION < 0x050000
	: QDeclarativeView (parent)
#else
	: QQuickWidget (parent)
#endif
	, Model_ (new MailListModel (this))
	{
		new Util::UnhoverDeleteMixin (this);

		const auto& file = Util::GetSysPath (Util::SysPath::QML, "gmailnotifier", "MailListView.qml");
		if (file.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "file not found";
			deleteLater ();
			return;
		}

		const auto& now = QDateTime::currentDateTime ();
		for (const auto& info : infos)
		{
			auto item = new QStandardItem;
			item->setData (info.Title_, MailListModel::Roles::Subject);
			item->setData (info.AuthorEmail_, MailListModel::Roles::AuthorEmail);
			item->setData (info.AuthorName_, MailListModel::Roles::AuthorName);
			item->setData (info.Summary_, MailListModel::Roles::Summary);

			QString dateString;
			if (info.Modified_.secsTo (now) < 12 * 60 * 60)
				dateString = info.Modified_.time ().toString ("hh:mm");
			else if (now.date ().day () == info.Modified_.date ().day () - 1)
				dateString = tr ("Yesterday, %1").arg (info.Modified_.time ().toString ());
			else if (now.date ().year () == info.Modified_.date ().year ())
				dateString = info.Modified_.date ().toString ("d MMM");
			else
				dateString = info.Modified_.date ().toString (Qt::DefaultLocaleShortDate);
			item->setData (dateString, MailListModel::Roles::ModifiedDate);
			Model_->appendRow (item);
		}

		setStyleSheet ("background: transparent");
		setWindowFlags (Qt::ToolTip);
		setAttribute (Qt::WA_TranslucentBackground);

		rootContext ()->setContextProperty ("mailListModel", Model_);
		rootContext ()->setContextProperty ("colorProxy",
				new Util::ColorThemeProxy (proxy->GetColorThemeManager (), this));
		setSource (QUrl::fromLocalFile (file));
	}
}
}
