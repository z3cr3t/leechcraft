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

#include "abbrev.h"
#include <QIcon>
#include <util/util.h>
#include <interfaces/azoth/iproxyobject.h>
#include <interfaces/azoth/iclentry.h>
#include "abbrevsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Abbrev
{
	void Plugin::Init (ICoreProxy_ptr)
	{
		qRegisterMetaType<Abbreviation> ("LeechCraft::Azoth::Abbrev::Abbreviation");
		qRegisterMetaType<QList<Abbreviation>> ("QList<LeechCraft::Azoth::Abbrev::Abbreviation>");
		qRegisterMetaTypeStreamOperators<Abbreviation> ();
		qRegisterMetaTypeStreamOperators<QList<Abbreviation>> ();

		Util::InstallTranslator ("azoth_abbrev");

		Manager_ = std::make_shared<AbbrevsManager> ();

		Commands_.append ({
				{ "/abbrev" },
				[this] (ICLEntry*, const QString& text) -> bool
				{
					const auto& pattern = text.section (' ', 1, 1).trimmed ();
					const auto& expansion = text.section (' ', 2).trimmed ();
					Manager_->Add ({ pattern, expansion });
					return true;
				},
				tr ("Adds a new abbreviation to the list of abbreviations."),
				tr ("Usage: @/abbrev@ _pattern_ _text_\n\n"
					"Adds a new _pattern_ that expands to the given _text_, which can span "
					"multiple lines.\n\n"
					"@/listabbrevs@ lists all available abbreviations and @/unabbrev@ allows "
					"removing them.")
			});
		Commands_.append ({
				{ "/listabbrevs" },
				[this] (ICLEntry *entry, const QString&) -> bool
				{
					ListAbbrevs (entry);
					return true;
				},
				tr ("Lists all abbreviations that were previously added."),
				{}
			});
		Commands_.append ({
				{ "/unabbrev" },
				[this] (ICLEntry*, const QString& text) -> bool
				{
					RemoveAbbrev (text.section (' ', 1).trimmed ());
					return true;
				},
				tr ("Removes a previously added abbreviation."),
				tr ("Usage: @/unabbrev@ <_pattern_|_index_>\n\n"
					"Removes a previously added abbrevation either by its _pattern_ or by its "
					"_index_ in the list returned by @/listabbrevs@.")
			});
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Azoth.Abbrev";
	}

	void Plugin::Release ()
	{
		Manager_.reset ();
	}

	QString Plugin::GetName () const
	{
		return "Azoth Abbrev";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Provides support for automatically expanding abbreviations for Azoth.");
	}

	QIcon Plugin::GetIcon () const
	{
		return {};
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Plugins.Azoth.Plugins.IGeneralPlugin";
		return result;
	}

	StaticCommands_t Plugin::GetStaticCommands (ICLEntry*)
	{
		return Commands_;
	}

	void Plugin::ListAbbrevs (ICLEntry *entry)
	{
		QStringList abbrevs;
		for (const auto& abbrev : Manager_->List ())
			abbrevs << QString::fromUtf8 ("%1 → %2")
					.arg (abbrev.Pattern_)
					.arg (abbrev.Expansion_);

		const auto& text = tr ("%n abbreviation(s):", 0, abbrevs.size ()) +
				"<ol><li>" + abbrevs.join ("</li><li>") + "</li></ol>";

		const auto entryObj = entry->GetQObject ();
		const auto msgObj = AzothProxy_->CreateCoreMessage (text,
				QDateTime::currentDateTime (),
				IMessage::Type::ServiceMessage,
				IMessage::Direction::In,
				entryObj,
				entryObj);
		const auto msg = qobject_cast<IMessage*> (msgObj);
		msg->Store ();
	}

	void Plugin::RemoveAbbrev (const QString& text)
	{
		bool ok = false;
		const auto idx = text.toInt (&ok);
		if (ok)
		{
			Manager_->Remove (idx - 1);
			return;
		}

		const auto& list = Manager_->List ();
		const auto pos = std::find_if (list.begin (), list.end (),
				[&text] (const Abbreviation& abbrev) { return abbrev.Pattern_ == text; });
		if (pos == list.end ())
			throw CommandException { tr ("Unable to find abbreviation %1.")
					.arg ("<em>" + text + "</em>") };

		Manager_->Remove (std::distance (list.begin (), pos));
	}

	void Plugin::initPlugin (QObject *proxyObj)
	{
		AzothProxy_ = qobject_cast<IProxyObject*> (proxyObj);
	}

	void Plugin::hookMessageWillCreated (LeechCraft::IHookProxy_ptr proxy,
			QObject*, QObject *entryObj, int, QString)
	{
		const auto& text = proxy->GetValue ("text").toString ();

		try
		{
			const auto& newText = Manager_->Process (text);
			if (text != newText)
				proxy->SetValue ("text", newText);
		}
		catch (const CommandException& e)
		{
			const auto msgObj = AzothProxy_->CreateCoreMessage (e.GetError (),
					QDateTime::currentDateTime (),
					IMessage::Type::ServiceMessage,
					IMessage::Direction::In,
					entryObj,
					entryObj);
			const auto msg = qobject_cast<IMessage*> (msgObj);
			msg->Store ();
		}
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_azoth_abbrev, LeechCraft::Azoth::Abbrev::Plugin);

