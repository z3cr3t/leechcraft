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

#include "webkitview.h"
#include <QIcon>
#include <QDir>
#include <qwebsettings.h>

#if QT_VERSION < 0x050000
#include <qwebkitversion.h>
#else
#include <qtwebkitversion.h>
#endif

#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <util/sys/paths.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/poshuku/iproxyobject.h>
#include "customwebview.h"
#include "customwebpage.h"
#include "webpluginfactory.h"
#include "linkhistory.h"
#include "xmlsettingsmanager.h"
#include "settingsglobalhandler.h"
#include "interceptadaptor.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace WebKitView
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;

		QWebHistoryInterface::setDefaultInterface (new LinkHistory);

		XSD_ = std::make_shared<Util::XmlSettingsDialog> ();
		XSD_->RegisterObject (&XmlSettingsManager::Instance (), "poshukuwebkitviewsettings.xml");

		const auto sgh = new SettingsGlobalHandler { this };
		connect (XSD_.get (),
				SIGNAL (pushButtonClicked (QString)),
				sgh,
				SLOT (handleSettingsClicked (QString)));

		try
		{
			const auto& path = Util::GetUserDir (Util::UserDir::Cache, "poshuku/favicons").absolutePath ();
			QWebSettings::setIconDatabasePath (path);
		}
		catch (const std::runtime_error& e)
		{
			qWarning () << Q_FUNC_INFO
					<< e.what ();
		}

		try
		{
			const auto& path = Util::CreateIfNotExists ("poshuku/offlinestorage").absolutePath ();
			QWebSettings::setOfflineStoragePath (path);
		}
		catch (const std::runtime_error& e)
		{
			qWarning () << Q_FUNC_INFO
					<< e.what ();
		}

		try
		{
			const auto& path = Util::GetUserDir (Util::UserDir::Cache, "poshuku/offlinewebappcache").absolutePath ();
			QWebSettings::setOfflineWebApplicationCachePath (path);
		}
		catch (const std::runtime_error& e)
		{
			qWarning () << Q_FUNC_INFO
					<< e.what ();
		}

		Interceptor_ = std::make_shared<InterceptAdaptor> ();
	}

	void Plugin::SecondInit ()
	{
		WebPluginFactory_ = new WebPluginFactory { Proxy_->GetPluginsManager () };
	}

	void Plugin::Release ()
	{
		Interceptor_.reset ();
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Poshuku.WebKitView";
	}

	QString Plugin::GetName () const
	{
		return "Poshuku WebKitView";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Provides QtWebKit-based backend for Poshuku.");
	}

	QIcon Plugin::GetIcon () const
	{
		return {};
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Poshuku.Plugins/1.0";
		result << "org.LeechCraft.Core.Plugins/1.0";
		return result;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	QString Plugin::GetDiagInfoString () const
	{
		return QString ("Built with QtWebKit %1, running with QtWebKit %2")
#ifdef QTWEBKIT_VERSION_STR
				.arg (QTWEBKIT_VERSION_STR)
#else
				.arg ("unknown (QTWEBKIT_VERSION_STR is not defined)")
#endif
				.arg (qWebKitVersion ());
	}

	IWebView* Plugin::CreateWebView ()
	{
		const auto view = new CustomWebView { Proxy_, PoshukuProxy_ };

		connect (view,
				SIGNAL (webViewCreated (IWebView*, bool)),
				this,
				SIGNAL (webViewCreated (IWebView*, bool)));

		if (WebPluginFactory_)
			view->page ()->setPluginFactory (WebPluginFactory_);
		else
			qWarning () << Q_FUNC_INFO
					<< "web plugin factory isn't initialized yet";

		return view;
	}

	QIcon Plugin::GetIconForUrl (const QUrl& url) const
	{
		const auto& specific = QWebSettings::iconForUrl (url);
		if (!specific.isNull ())
			return specific;

		QUrl test;
		test.setScheme (url.scheme ());
		test.setHost (url.host ());
		return QWebSettings::iconForUrl (test);
	}

	QIcon Plugin::GetDefaultUrlIcon () const
	{
		return QWebSettings::webGraphic (QWebSettings::DefaultFrameIconGraphic);
	}

	void Plugin::AddInterceptor (const Interceptor_t& interceptor)
	{
		Interceptor_->AddInterceptor (interceptor);
	}

	void Plugin::hookNAMCreateRequest (IHookProxy_ptr proxy,
			QNetworkAccessManager *manager,
			QNetworkAccessManager::Operation *op,
			QIODevice **dev)
	{
		Interceptor_->HandleNAM (proxy, manager, op, dev);
	}

	void Plugin::initPlugin (QObject *proxyObj)
	{
		PoshukuProxy_ = qobject_cast<IProxyObject*> (proxyObj);
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_poshuku_webkitview, LeechCraft::Poshuku::WebKitView::Plugin);
