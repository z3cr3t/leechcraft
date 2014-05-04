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

#include "azoth.h"
#include <QIcon>
#include <QDockWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QMenu>
#include <QStringListModel>

#ifdef ENABLE_MEDIACALLS
#include <QAudioDeviceInfo>
#endif

#include <interfaces/entitytesthandleresult.h>
#include <interfaces/imwproxy.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/irootwindowsmanager.h>
#include <interfaces/core/iiconthememanager.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <util/sys/resourceloader.h>
#include <util/util.h>
#include <util/shortcuts/shortcutmanager.h>
#include "interfaces/azoth/imucjoinwidget.h"
#include "core.h"
#include "mainwidget.h"
#include "chattabsmanager.h"
#include "chattab.h"
#include "xmlsettingsmanager.h"
#include "transferjobmanager.h"
#include "servicediscoverywidget.h"
#include "microblogstab.h"
#include "accountslistwidget.h"
#include "consolewidget.h"
#include "searchwidget.h"
#include "chatstyleoptionmanager.h"
#include "colorlisteditorwidget.h"
#include "customstatusesmanager.h"
#include "accountactionsmanager.h"
#include "serverhistorywidget.h"
#include "actionsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Translator_.reset (Util::InstallTranslator ("azoth"));

		ChatTab::SetParentMultiTabs (this);
		ServiceDiscoveryWidget::SetParentMultiTabs (this);
		SearchWidget::SetParentMultiTabs (this);

		Core::Instance ().SetProxy (proxy);

		connect (Core::Instance ().GetActionsManager (),
				SIGNAL (gotServerHistoryTab (ServerHistoryWidget*)),
				this,
				SLOT (handleServerHistoryTab (ServerHistoryWidget*)));

		InitShortcuts ();
		InitAccActsMgr ();
		InitSettings ();
		InitSignals ();
		InitTabClasses ();
	}

	void Plugin::SecondInit ()
	{
		InitMW ();

		XmlSettingsDialog_->SetDataSource ("SmileIcons",
				Core::Instance ().GetSmilesOptionsModel ());

		auto setSOM = [this] (const QByteArray& propName)
		{
			auto mgr = Core::Instance ().GetChatStylesOptionsManager (propName);
			XmlSettingsDialog_->SetDataSource (propName, mgr->GetStyleModel ());
			XmlSettingsDialog_->SetDataSource (propName + "Variant", mgr->GetVariantModel ());
		};
		setSOM ("ChatWindowStyle");
		setSOM ("MUCWindowStyle");

		Core::Instance ().GetShortcutManager ()->AnnounceGlobalShorcuts ();
	}

	void Plugin::Release ()
	{
		Core::Instance ().Release ();
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Azoth";
	}

	QString Plugin::GetName () const
	{
		return "Azoth";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Extensible IM client for LeechCraft.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/azoth/resources/images/azoth.svg");
		return icon;
	}

	QStringList Plugin::Provides () const
	{
		return QStringList (GetUniqueID ());
	}

	QSet<QByteArray> Plugin::GetExpectedPluginClasses () const
	{
		QSet<QByteArray> classes;
		classes << "org.LeechCraft.Plugins.Azoth.Plugins.IGeneralPlugin";
		classes << "org.LeechCraft.Plugins.Azoth.Plugins.IProtocolPlugin";
		classes << "org.LeechCraft.Plugins.Azoth.Plugins.IResourceSourcePlugin";
		return classes;
	}

	void Plugin::AddPlugin (QObject *object)
	{
		Core::Instance ().AddPlugin (object);
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XmlSettingsDialog_;
	}

	QAbstractItemModel* Plugin::GetRepresentation () const
	{
		return Core::Instance ().GetTransferJobManager ()->GetSummaryModel ();
	}

	QList<QAction*> Plugin::GetActions (ActionsEmbedPlace aep) const
	{
		QList<QAction*> result;
		switch (aep)
		{
		case ActionsEmbedPlace::TrayMenu:
			result << MW_->GetChangeStatusMenu ()->menuAction ();
			break;
		default:
			break;
		}
		return result;
	}

	QMap<QString, QList<QAction*>> Plugin::GetMenuActions () const
	{
		QMap<QString, QList<QAction*>> result;
		result ["Azoth"] << MW_->GetMenuActions ();
		return result;
	}

	EntityTestHandleResult Plugin::CouldHandle (const Entity& e) const
	{
		return Core::Instance ().CouldHandle (e) ?
				EntityTestHandleResult (EntityTestHandleResult::PIdeal) :
				EntityTestHandleResult ();
	}

	void Plugin::Handle (Entity e)
	{
		Core::Instance ().Handle (e);
	}

	TabClasses_t Plugin::GetTabClasses () const
	{
		return TabClasses_;
	}

	void Plugin::TabOpenRequested (const QByteArray& tabClass)
	{
		if (tabClass == "MUCTab")
			Core::Instance ().handleMucJoinRequested ();
		else if (tabClass == "SD")
			handleSDWidget (new ServiceDiscoveryWidget);
		else if (tabClass == "Search")
		{
			SearchWidget *search = new SearchWidget;
			connect (search,
					SIGNAL (removeTab (QWidget*)),
					this,
					SIGNAL (removeTab (QWidget*)));
			emit addNewTab (tr ("Search"), search);
			emit raiseTab (search);
		}
	}

	void Plugin::RecoverTabs (const QList<TabRecoverInfo>& infos)
	{
		for (const auto& recInfo : infos)
		{
			QDataStream str (recInfo.Data_);
			QByteArray context;
			str >> context;

			if (context == "chattab" || context == "chattab2")
			{
				ChatTabsManager::RestoreChatInfo info;
				info.Props_ = recInfo.DynProperties_;
				str >> info.EntryID_
					>> info.Variant_;

				if (context == "chattab2")
					str >> info.MsgText_;

				Core::Instance ().GetChatTabsManager ()->EnqueueRestoreInfos ({ info });
			}
			else if (context == "muctab2")
			{
				QString entryId;
				QVariantMap data;
				QByteArray accountId;
				QString text;
				str >> entryId
					>> data
					>> accountId
					>> text;

				ChatTabsManager::RestoreChatInfo info;
				info.Props_ = recInfo.DynProperties_;
				info.EntryID_ = entryId;
				info.MsgText_ = text;

				Core::Instance ().GetChatTabsManager ()->EnqueueRestoreInfos ({ info });

				if (!Core::Instance ().GetEntry (entryId))
				{
					auto acc = Core::Instance ().GetAccount (accountId);
					if (!acc)
					{
						qWarning () << Q_FUNC_INFO
								<< "no account for ID"
								<< accountId;
						continue;
					}

					auto proto = qobject_cast<IProtocol*> (acc->GetParentProtocol ());
					auto widgetObj = proto->GetMUCJoinWidget ();
					auto widget = qobject_cast<IMUCJoinWidget*> (widgetObj);
					if (!widget)
						continue;

					widget->SetIdentifyingData (data);
					widget->Join (acc->GetQObject ());
					widgetObj->deleteLater ();
				}
			}
			else
				qWarning () << Q_FUNC_INFO
						<< "unknown context"
						<< context;
		}
	}

	void Plugin::SetShortcut (const QString& id, const QKeySequences_t& seqs)
	{
		Core::Instance ().GetShortcutManager ()->SetShortcut (id, seqs);
	}

	QMap<QString, ActionInfo> Plugin::GetActionInfo () const
	{
		return Core::Instance ().GetShortcutManager ()->GetActionInfo ();
	}

	QList<ANFieldData> Plugin::GetANFields () const
	{
		return Core::Instance ().GetANFields ();
	}

	void Plugin::InitShortcuts ()
	{
		auto proxy = Core::Instance ().GetProxy ();

		auto sm = Core::Instance ().GetShortcutManager ();
		sm->SetObject (this);

		sm->RegisterActionInfo ("org.LeechCraft.Azoth.ClearChat",
				ActionInfo (tr ("Clear chat window"),
						QString ("Ctrl+L"),
						proxy->GetIconThemeManager ()->GetIcon ("edit-clear-history")));
		sm->RegisterActionInfo ("org.LeechCraft.Azoth.ScrollHistoryBack",
				ActionInfo (tr ("Prepend messages from history"),
						QKeySequence::StandardKey::Back,
						proxy->GetIconThemeManager ()->GetIcon ("go-previous")));
		sm->RegisterActionInfo ("org.LeechCraft.Azoth.QuoteSelected",
				ActionInfo (tr ("Quote selected in chat tab"),
						QString ("Ctrl+Q"),
						proxy->GetIconThemeManager ()->GetIcon ("mail-reply-sender")));

		sm->RegisterActionInfo ("org.LeechCraft.Azoth.LeaveMUC",
				ActionInfo (tr ("Leave"),
						QString (),
						proxy->GetIconThemeManager ()->GetIcon ("irc-close-channel")));
		sm->RegisterActionInfo ("org.LeechCraft.Azoth.MUCUsers",
				ActionInfo (tr ("Show MUC users list"),
						QString ("Ctrl+M"),
						proxy->GetIconThemeManager ()->GetIcon ("irc-close-channel")));
		sm->RegisterActionInfo ("org.LeechCraft.Azoth.OpenLastLink",
				ActionInfo (tr ("Open last link in chat"),
						QString ("Ctrl+O"),
						proxy->GetIconThemeManager ()->GetIcon ("document-open-remote")));

		sm->RegisterGlobalShortcut ("org.LeechCraft.Azoth.ShowNextUnread",
				&Core::Instance (), SLOT (handleShowNextUnread ()),
				{
					tr ("Show next unread message (global shortcut)"),
					QString ("Ctrl+Alt+Shift+M"),
					proxy->GetIconThemeManager ()->GetIcon ("mail-unread-new")
				});

		sm->RegisterActionInfo ("org.Azoth.TextEdit.DeleteWord",
				{
					tr ("Delete the word before the cursor"),
					QKeySequence {},
					{}
				}
			);
		sm->RegisterActionInfo ("org.Azoth.TextEdit.DeleteBOL",
				{
					tr ("Delete from cursor to the beginning of line"),
					QKeySequence { "Ctrl+U" },
					{}
				}
			);
		sm->RegisterActionInfo ("org.Azoth.TextEdit.DeleteEOL",
				{
					tr ("Delete from cursor to the end of line"),
					QKeySequence { "Ctrl+K" },
					{}
				}
			);
	}

	void Plugin::InitAccActsMgr ()
	{
		auto accActsMgr = new AccountActionsManager ();
		MW_ = new MainWidget (accActsMgr);
		accActsMgr->SetMainWidget (MW_);
		connect (accActsMgr,
				SIGNAL (gotConsoleWidget (ConsoleWidget*)),
				this,
				SLOT (handleConsoleWidget (ConsoleWidget*)));
		connect (accActsMgr,
				SIGNAL (gotSDWidget (ServiceDiscoveryWidget*)),
				this,
				SLOT (handleSDWidget (ServiceDiscoveryWidget*)));
		connect (accActsMgr,
				SIGNAL (gotMicroblogsTab (MicroblogsTab*)),
				this,
				SLOT (handleMicroblogsTab (MicroblogsTab*)));
		connect (accActsMgr,
				SIGNAL (gotServerHistoryTab (ServerHistoryWidget*)),
				this,
				SLOT (handleServerHistoryTab (ServerHistoryWidget*)));
	}

	void Plugin::InitSettings ()
	{
		XmlSettingsDialog_.reset (new Util::XmlSettingsDialog ());
		XmlSettingsDialog_->RegisterObject (&XmlSettingsManager::Instance (),
				"azothsettings.xml");

		connect (XmlSettingsDialog_.get (),
				SIGNAL (moreThisStuffRequested (const QString&)),
				this,
				SLOT (handleMoreThisStuff (const QString&)));

		XmlSettingsDialog_->SetDataSource ("StatusIcons",
				Core::Instance ().GetResourceLoader (Core::RLTStatusIconLoader)->
					GetSubElemModel ());
		XmlSettingsDialog_->SetDataSource ("ClientIcons",
				Core::Instance ().GetResourceLoader (Core::RLTClientIconLoader)->
					GetSubElemModel ());
		XmlSettingsDialog_->SetDataSource ("AffIcons",
				Core::Instance ().GetResourceLoader (Core::RLTAffIconLoader)->
					GetSubElemModel ());
		XmlSettingsDialog_->SetDataSource ("MoodIcons",
				Core::Instance ().GetResourceLoader (Core::RLTMoodIconLoader)->
					GetSubElemModel ());
		XmlSettingsDialog_->SetDataSource ("ActivityIcons",
				Core::Instance ().GetResourceLoader (Core::RLTActivityIconLoader)->
					GetSubElemModel ());
		XmlSettingsDialog_->SetDataSource ("SystemIcons",
				Core::Instance ().GetResourceLoader (Core::RLTSystemIconLoader)->
					GetSubElemModel ());

		QList<QByteArray> iconsPropList;
		iconsPropList << "StatusIcons"
				<< "ClientIcon"
				<< "AffIcons"
				<< "MoodIcons"
				<< "ActivityIcons"
				<< "SystemIcons";
		XmlSettingsManager::Instance ().RegisterObject (iconsPropList,
				&Core::Instance (),
				"flushIconCaches");

#ifdef ENABLE_MEDIACALLS
		QStringList audioIns (tr ("Default input device"));
		Q_FOREACH (const QAudioDeviceInfo& info,
				QAudioDeviceInfo::availableDevices (QAudio::AudioInput))
			audioIns << info.deviceName ();
		XmlSettingsDialog_->SetDataSource ("InputAudioDevice", new QStringListModel (audioIns));

		QStringList audioOuts (tr ("Default output device"));
		Q_FOREACH (const QAudioDeviceInfo& info,
				QAudioDeviceInfo::availableDevices (QAudio::AudioOutput))
			audioOuts << info.deviceName ();
		XmlSettingsDialog_->SetDataSource ("OutputAudioDevice", new QStringListModel (audioOuts));
#endif

		auto accountsList = new AccountsListWidget;
		XmlSettingsDialog_->SetCustomWidget ("AccountsWidget", accountsList);
		connect (accountsList,
				SIGNAL (accountVisibilityChanged (IAccount*)),
				MW_,
				SLOT (handleAccountVisibilityChanged ()));
		connect (accountsList,
				SIGNAL (accountVisibilityChanged (IAccount*)),
				&Core::Instance (),
				SLOT (saveAccountVisibility (IAccount*)));

		XmlSettingsDialog_->SetCustomWidget ("ColorListEditor", new ColorListEditorWidget);

		XmlSettingsDialog_->SetDataSource ("CustomStatusesView",
				Core::Instance ().GetCustomStatusesManager ()->GetModel ());
	}

	void Plugin::InitMW ()
	{
		QDockWidget *dw = new QDockWidget ();
		dw->setWidget (MW_);
		dw->setWindowTitle ("Azoth");
		dw->setWindowIcon (GetIcon ());
		dw->toggleViewAction ()->setIcon (GetIcon ());

		const int dockArea = XmlSettingsManager::Instance ()
				.Property ("MWDockArea", Qt::RightDockWidgetArea).toInt ();
		const bool floating = XmlSettingsManager::Instance ()
				.Property ("MWFloating", false).toBool ();

		auto rootWM = Core::Instance ().GetProxy ()->GetRootWindowsManager ();
		rootWM->GetMWProxy (0)->AddDockWidget (static_cast<Qt::DockWidgetArea> (dockArea), dw);
		rootWM->GetMWProxy (0)->SetViewActionShortcut (dw, QString ("Ctrl+J,A"));

		dw->setFloating (floating);
		connect (dw,
				SIGNAL (dockLocationChanged (Qt::DockWidgetArea)),
				this,
				SLOT (handleMWLocation (Qt::DockWidgetArea)));
		connect (dw,
				SIGNAL (topLevelChanged (bool)),
				this,
				SLOT (handleMWFloating (bool)));
	}

	void Plugin::InitSignals ()
	{
		connect (&Core::Instance (),
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SIGNAL (gotEntity (const LeechCraft::Entity&)));
		connect (&Core::Instance (),
				SIGNAL (delegateEntity (const LeechCraft::Entity&, int*, QObject**)),
				this,
				SIGNAL (delegateEntity (const LeechCraft::Entity&, int*, QObject**)));
		connect (&Core::Instance (),
				SIGNAL (gotSDWidget (ServiceDiscoveryWidget*)),
				this,
				SLOT (handleSDWidget (ServiceDiscoveryWidget*)));

		connect (Core::Instance ().GetChatTabsManager (),
				SIGNAL (addNewTab (const QString&, QWidget*)),
				this,
				SIGNAL (addNewTab (const QString&, QWidget*)));
		connect (Core::Instance ().GetChatTabsManager (),
				SIGNAL (changeTabName (QWidget*, const QString&)),
				this,
				SIGNAL (changeTabName (QWidget*, const QString&)));
		connect (Core::Instance ().GetChatTabsManager (),
				SIGNAL (changeTabIcon (QWidget*, const QIcon&)),
				this,
				SIGNAL (changeTabIcon (QWidget*, const QIcon&)));
		connect (Core::Instance ().GetChatTabsManager (),
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)));
		connect (Core::Instance ().GetChatTabsManager (),
				SIGNAL (raiseTab (QWidget*)),
				this,
				SIGNAL (raiseTab (QWidget*)));
	}

	void Plugin::InitTabClasses ()
	{
		TabClassInfo chatTab =
		{
			"LeechCraft.Azoth.ChatTab",
			tr ("Chat"),
			tr ("A tab with a chat session"),
			QIcon ("lcicons:/plugins/azoth/resources/images/chattabclass.svg"),
			0,
			TFEmpty
		};
		ChatTab::SetChatTabClassInfo (chatTab);

		TabClassInfo mucTab =
		{
			"LeechCraft.Azoth.MUCTab",
			tr ("MUC"),
			tr ("A multiuser conference"),
			QIcon ("lcicons:/plugins/azoth/resources/images/muctabclass.svg"),
			0,
			TFEmpty
		};
		ChatTab::SetMUCTabClassInfo (mucTab);

		TabClassInfo searchTab =
		{
			"Search",
			tr ("Search"),
			tr ("A search tab allows one to search within IM services"),
			QIcon ("lcicons:/plugins/azoth/resources/images/searchtab.svg"),
			55,
			TFOpenableByRequest
		};
		TabClassInfo sdTab =
		{
			"SD",
			tr ("Service discovery"),
			tr ("A service discovery tab that allows one to discover "
				"capabilities of remote entries"),
			QIcon ("lcicons:/plugins/azoth/resources/images/sdtab.svg"),
			55,
			TFOpenableByRequest
		};
		TabClassInfo consoleTab =
		{
			"ConsoleTab",
			tr ("IM console"),
			tr ("Protocol console, for example, XML console for a XMPP "
				"client protocol"),
			QIcon ("lcicons:/plugins/azoth/resources/images/console.svg"),
			0,
			TFEmpty
		};

		TabClassInfo microblogsTab =
		{
			"MicroblogsTab",
			tr ("Microblogs"),
			tr ("Microblogs where protocol/account supports that"),
			QIcon (),
			0,
			TFEmpty
		};
		MicroblogsTab::SetTabData (this, microblogsTab);

		ServerHistoryTC_ =
		{
			"ServerHistoryTab",
			tr ("Server history"),
			tr ("Server history browser for protocols and accounts supporting this feature"),
			{},
			0,
			TFEmpty
		};

		TabClasses_ << chatTab;
		TabClasses_ << mucTab;
		TabClasses_ << searchTab;
		TabClasses_ << sdTab;
		TabClasses_ << consoleTab;
		TabClasses_ << microblogsTab;
		TabClasses_ << ServerHistoryTC_;
	}

	void Plugin::handleSDWidget (ServiceDiscoveryWidget *sd)
	{
		connect (sd,
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)));
		emit addNewTab (tr ("Service discovery"), sd);
		emit raiseTab (sd);
	}

	void Plugin::handleMicroblogsTab (MicroblogsTab *tab)
	{
		connect (tab,
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)));
		emit addNewTab (tr ("Microblogs"), tab);
		emit raiseTab (tab);
	}

	void Plugin::handleServerHistoryTab (ServerHistoryWidget *widget)
	{
		connect (widget,
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)));
		widget->SetTabInfo (this, ServerHistoryTC_);

		emit addNewTab (ServerHistoryTC_.VisibleName_, widget);
		emit raiseTab (widget);
	}

	void Plugin::handleTasksTreeSelectionCurrentRowChanged (const QModelIndex& index, const QModelIndex&)
	{
		QModelIndex si = Core::Instance ().GetProxy ()->MapToSource (index);
		TransferJobManager *mgr = Core::Instance ().GetTransferJobManager ();
		mgr->SelectionChanged (si.model () == mgr->GetSummaryModel () ? si : QModelIndex ());
	}

	void Plugin::handleMWLocation (Qt::DockWidgetArea area)
	{
		XmlSettingsManager::Instance ().setProperty ("MWDockArea", area);
	}

	void Plugin::handleMWFloating (bool floating)
	{
		XmlSettingsManager::Instance ().setProperty ("MWFloating", floating);
	}

	void Plugin::handleMoreThisStuff (const QString& id)
	{
		QMap<QString, QStringList> id2tags;
		id2tags ["StatusIcons"] << "azoth" << "status icons";
		id2tags ["MoodIcons"] << "azoth" << "mood icons";
		id2tags ["Smiles"] << "azoth" << "emoticons";
		id2tags ["ClientIcons"] << "azoth" << "client icons";
		id2tags ["AffIcons"] << "azoth" << "affiliation icons";
		id2tags ["ActivityIcons"] << "azoth" << "activity icons";
		id2tags ["SystemIcons"] << "azoth" << "system icons";
		id2tags ["ChatWindowStyles"] << "azoth" << "chat styles";

		const QStringList& tags = id2tags [id];
		if (tags.isEmpty ())
			return;

		Entity e = Util::MakeEntity ("ListPackages",
				QString (),
				FromUserInitiated,
				"x-leechcraft/package-manager-action");
		e.Additional_ ["Tags"] = tags;

		emit gotEntity (e);
	}

	void Plugin::handleConsoleWidget (ConsoleWidget *cw)
	{
		cw->SetParentMultiTabs (this);
		connect (cw,
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)),
				Qt::UniqueConnection);
		emit addNewTab (cw->GetTitle (), cw);
		emit raiseTab (cw);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_azoth, LeechCraft::Azoth::Plugin);
