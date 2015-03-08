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

#include <QObject>
#include <QLinkedList>
#include <interfaces/iinfo.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/iactionsexporter.h>
#include "batteryhistory.h"
#include "batteryinfo.h"
#include "platform/poweractions/platform.h"

namespace LeechCraft
{
namespace Liznoo
{
	class BatteryHistoryDialog;
	class PlatformLayer;

	namespace Screen
	{
		class ScreenPlatform;
	}

	namespace PowerActions
	{
		class Platform;
	}

	namespace Battery
	{
		class BatteryPlatform;
	}

	class Plugin : public QObject
				 , public IInfo
				 , public IHaveSettings
				 , public IEntityHandler
				 , public IActionsExporter
	{
		Q_OBJECT
		Q_INTERFACES (IInfo IHaveSettings IEntityHandler IActionsExporter)

		LC_PLUGIN_METADATA ("org.LeechCraft.Liznoo")

		ICoreProxy_ptr Proxy_;

		Util::XmlSettingsDialog_ptr XSD_;

		std::shared_ptr<PlatformLayer> PL_;
		Screen::ScreenPlatform *SPL_ = nullptr;
		std::shared_ptr<PowerActions::Platform> PowerActPlatform_;
		std::shared_ptr<Battery::BatteryPlatform> BatteryPlatform_;

		QMap<QString, QAction*> Battery2Action_;
		QMap<QString, BatteryInfo> Battery2LastInfo_;
		QMap<QString, BatteryHistoryDialog*> Battery2Dialog_;
		QMap<QString, QLinkedList<BatteryHistory>> Battery2History_;

		QAction *Suspend_;
		QAction *Hibernate_;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		QByteArray GetUniqueID () const;
		void Release ();
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;

		Util::XmlSettingsDialog_ptr GetSettingsDialog () const;

		EntityTestHandleResult CouldHandle (const Entity& entity) const;
		void Handle (Entity entity);

		QList<QAction*> GetActions (ActionsEmbedPlace) const;
		QMap<QString, QList<QAction*>> GetMenuActions () const;
	private:
		void UpdateAction (const BatteryInfo&);
		void CheckNotifications (const BatteryInfo&);

		void ChangeState (PowerActions::Platform::State);
	private slots:
		void handleBatteryInfo (Liznoo::BatteryInfo);
		void handleUpdateHistory ();
		void handleHistoryTriggered ();
		void handleBatteryDialogDestroyed ();

		void handleSuspendRequested ();
		void handleHibernateRequested ();

		void handlePushButton (const QString&);
	signals:
		void gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace);
	};
}
}

