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

#include "utilproxy.h"
#include <util/util.h>
#include <util/xpc/util.h>
#include <util/sys/paths.h>
#include <interfaces/structures.h>

namespace LeechCraft
{
namespace Qrosp
{
	UtilProxy::UtilProxy (QObject *parent)
	: QObject (parent)
	{
	}

	QString UtilProxy::GetUserText (const LeechCraft::Entity& entity) const
	{
		return Util::GetUserText (entity);
	}

	QString UtilProxy::MakePrettySize (qint64 size) const
	{
		return Util::MakePrettySize (size);
	}

	QString UtilProxy::MakeTimeFromLong (ulong time) const
	{
		return Util::MakePrettySize (time);
	}

	QTranslator* UtilProxy::InstallTranslator (const QString& base,
			const QString& prefix, const QString& appname) const
	{
		return Util::InstallTranslator (base, prefix, appname);
	}

	QString UtilProxy::GetLocaleName () const
	{
		return Util::GetLocaleName ();
	}

	QString UtilProxy::GetLanguage () const
	{
		return Util::GetLanguage ();
	}

	QDir UtilProxy::CreateIfNotExists (const QString& path) const
	{
		return Util::CreateIfNotExists (path);
	}

	QString UtilProxy::GetTemporaryName (const QString& pattern) const
	{
		return Util::GetTemporaryName (pattern);
	}

	QObject* UtilProxy::MakeEntity (const QVariant& entity,
			const QString& location, LeechCraft::TaskParameters tp, const QString& mime) const
	{
		return new EntityWrapper (Util::MakeEntity (entity, location, tp,  mime));
	}

	namespace
	{
		Priority Str2Priority (QString str)
		{
			str = str.toLower ();
			if (str == "log")
				return PLog_;
			else if (str == "info")
				return PInfo_;
			else if (str == "warning")
				return PWarning_;
			else if (str == "critical")
				return PCritical_;
			else
			{
				qWarning () << Q_FUNC_INFO
						<< "unknown priority"
						<< str;
				return PInfo_;
			}
		}
	}

	QObject* UtilProxy::MakeNotification (const QString& header,
			const QString& text, QString priorityStr) const
	{
		return new EntityWrapper (Util::MakeNotification (header, text, Str2Priority (priorityStr)));
	}
}
}
