/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Vladislav Tyulbashev
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

#include <interfaces/core/icoreproxy.h>
#include <QIcon>
#include <QShortcut>
#include <util/shortcuts/shortcutmanager.h>
#include "vlc.h"

namespace LeechCraft
{
namespace vlc
{	
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
		Manager_ = new Util::ShortcutManager (proxy);
		Manager_->SetObject (this);
		
		Manager_->RegisterActionInfo ("org.vlc.navigate_left", 
									ActionInfo (tr ("Key for navigate left in DVD menu"), 
									QKeySequence (Qt::Key_Left), 
									Proxy_->GetIcon ("arrow-left")));
		
		Manager_->RegisterActionInfo ("org.vlc.navigate_right",
									ActionInfo (tr ("Key for navigate right in DVD menu"), 
									QKeySequence (Qt::Key_Right), 
									Proxy_->GetIcon ("arrow-right")));
		
		Manager_->RegisterActionInfo ("org.vlc.navigate_up",
									ActionInfo (tr ("Key for navigate up in DVD menu"), 
									QKeySequence (Qt::Key_Up),
									Proxy_->GetIcon ("arrow-up")));
		
		Manager_->RegisterActionInfo ("org.vlc.navigate_down",
									ActionInfo (tr ("Key for navigate down in DVD menu"), 
									QKeySequence (Qt::Key_Down),
									Proxy_->GetIcon ("arrow-down")));
		
		Manager_->RegisterActionInfo ("org.vlc.navigate_enter",
									ActionInfo (tr ("Key for activate current in DVD menu"), 
									QKeySequence (Qt::Key_Enter), 
									Proxy_->GetIcon ("key-enter")));
		
		Manager_->RegisterActionInfo ("org.vlc.toggle_fullscreen",
									ActionInfo (tr ("Key for toggle fullscreen"),
									QKeySequence (Qt::Key_F),
									Proxy_->GetIcon ("view-fullscreen")));
		
		Manager_->RegisterActionInfo ("org.vlc.toggle_play",
									ActionInfo (tr ("Key for switch play/pause"),
									QKeySequence (Qt::Key_Space),
									Proxy_->GetIcon ("media-playback-start")));
		
		Manager_->RegisterActionInfo ("org.vlc.volume_increase",
									ActionInfo (tr ("Key for increasing volume"),
									QKeySequence (Qt::Key_Plus),
									Proxy_->GetIcon ("audio-volume-high")));
		
		Manager_->RegisterActionInfo ("org.vlc.volume_decrease",
									ActionInfo (tr ("Key for decreasing volume"),
									QKeySequence (Qt::Key_Minus),
									Proxy_->GetIcon ("audio-volume-low")));
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.vlc";
	}

	void Plugin::Release ()
	{
		for (int i = 0; i < Tabs_.size (); i++)
			delete Tabs_ [i];
	}

	QString Plugin::GetName () const
	{
		return "vlc";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Video player, based on VLC");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}
	
	void Plugin::TabOpenRequested (const QByteArray& tabClass) 
	{
		VlcWidget *widget = new VlcWidget (Manager_);
		Tabs_.push_back (widget);
		emit addNewTab ("VLC", widget);
		emit raiseTab (widget);
		connect (widget, 
				SIGNAL (deleteMe (QWidget*)), 
				this, 
				SIGNAL (removeTab (QWidget*)));
	}
	
	LeechCraft::TabClasses_t Plugin::GetTabClasses () const 
	{
		return { VlcWidget::GetTabInfo () };
	}
	
	QMap<QString, ActionInfo> Plugin::GetActionInfo () const
	{
		return Manager_->GetActionInfo ();
	}
	
	void Plugin::SetShortcut (const QString &id, const QKeySequences_t &shortcuts)
	{
		Manager_->SetShortcut (id, shortcuts);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_vlc, LeechCraft::vlc::Plugin);
