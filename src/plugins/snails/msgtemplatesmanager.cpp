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

#include "msgtemplatesmanager.h"
#include <functional>
#include <QLocale>
#include <QtDebug>
#include <interfaces/itexteditor.h>
#include "message.h"
#include "account.h"

namespace LeechCraft
{
namespace Snails
{
	MsgTemplatesManager::MsgTemplatesManager (QObject *parent)
	: QObject { parent }
	{
	}

	QString MsgTemplatesManager::GetTemplate (ContentType contentType, MsgType msgType, Account *account) const
	{
		static const auto defaults = GetDefaults ();
		return defaults [contentType] [msgType];
	}

	namespace
	{
		auto GetFunctions ()
		{
			QHash<QString, std::function<QString (const Message*, QString)>> result;

			result ["ODATE"] = [] (const Message *msg, const QString&) { return msg->GetDate ().date ().toString (Qt::DefaultLocaleLongDate); };
			result ["OTIME"] = [] (const Message *msg, const QString&) { return msg->GetDate ().time ().toString (Qt::DefaultLocaleLongDate); };
			result ["ONAME"] = [] (const Message *msg, const QString&) { return msg->GetAddress (Message::Address::ReplyTo).first; };
			result ["OEMAIL"] = [] (const Message *msg, const QString&) { return msg->GetAddress (Message::Address::ReplyTo).second; };
			result ["ONAMEOREMAIL"] = [] (const Message *msg, const QString&)
						{
							const auto& addr = msg->GetAddress (Message::Address::ReplyTo);
							return addr.first.isEmpty () ? addr.second : addr.first;
						};
			result ["QUOTE"] = [] (const Message*, const QString& body) { return body; };

			return result;
		}

		static const QString OpenMarker = "${";
		static const QString CloseMarker = "}";

		QString PerformSubstitutions (QString text, const Message *msg, const QString& body)
		{
			static const auto functions = GetFunctions ();

			const auto openSize = OpenMarker.size ();

			auto pos = 0;
			while ((pos = text.indexOf (OpenMarker, pos)) != -1)
			{
				const auto closing = text.indexOf (CloseMarker, pos);
				if (closing == -1)
					break;

				const auto& variable = text.mid (pos + openSize, closing - pos - openSize);

				if (functions.contains (variable))
				{
					const auto& subst = functions [variable] (msg, body);
					text.replace (pos, closing - pos + 1, subst);
					pos += subst.size ();
				}
				else
					pos = closing + 1;
			}

			return text;
		}
	}

	QString MsgTemplatesManager::GetTemplatedText (ContentType type,
			MsgType msgType, const QString& body, const Message *msg) const
	{
		return body;
	}

	QMap<ContentType, QMap<MsgTemplatesManager::MsgType, QString>> MsgTemplatesManager::GetDefaults ()
	{
		return
		{
			{
				ContentType::PlainText,
				{
					{
						MsgType::New,
						tr (R"delim(Dear ${ONAME},

${CURSOR}

${SIGNATURE})delim")
					},
					{
						MsgType::Reply,
						tr (R"delim(On ${ODATE} at ${OTIME} user ${ONAME} wrote:
${QUOTE}

${CURSOR}

${SIGNATURE})delim")
					}
				}
			}
		};
	}
}
}
