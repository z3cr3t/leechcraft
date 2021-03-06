/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Slava Barinov <rayslava@gmail.com>
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

#include "tweet.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Woodpecker
{
	Tweet::Tweet (QObject *parent)
	: QObject (parent)
	, Author_ (new TwitterUser (parent))
	{
	}

	Tweet::Tweet (const QString& text, const qulonglong id, TwitterUser* author, QObject *parent)
	: QObject (parent)
	, Id_ (id)
	, Author_ (author)
	{
		SetText (text);
	}

	Tweet::Tweet (const Tweet& original)
	: Id_ (original.GetId ())
	, Author_ (original.GetAuthor ())
	, Created_ (original.GetDateTime ())
	{
		SetText (original.GetText ());
	}

	bool Tweet::operator== (const Tweet& other) const
	{
		return Id_ == other.GetId ();
	}

	bool Tweet::operator!= (const Tweet& other) const
	{
		return !(*this == other);
	}

	bool Tweet::operator< (const Tweet& other) const
	{
		return Id_ < other.GetId ();
	}

	bool Tweet::operator> (const Tweet& other) const
	{
		return Id_ > other.GetId ();
	}

	void Tweet::SetText (const QString& text)
	{
		/* HTML links matching */
		QRegExp rx ("(\\s|^)((http|https)://[a-z0-9]+([-.]{1}[a-z0-9]+)*.[a-z]{2,5}(([0-9]{1,5})?/?.*))(\\s|,|$)");
		rx.setMinimal (true);

		Text_ = text;

		/* Some regexp multiple match support magic for links highlighting.
		 * Borrowed from Qt support forum */
		QString html = text;
		int pos = 0;
		while ((pos = rx.indexIn (html, pos)) != -1)
		{
			if (rx.cap (2).startsWith ("http"))
			{
				QString before = rx.cap (2);
				if (before.endsWith ("."))
					before.chop (1);
				QString after = " <a href=\"" + before + "\">" + before + "</a>";
				html.replace (pos, before.length () + 1, after);
				pos += after.length ();
			}
			else
				pos += rx.matchedLength ();
		}

		/* Twitter username matching */
		QRegExp usernameRx ("(\\s|^)(@[\\w\\d_]+)(\\s|,|$|:)");
		usernameRx.setMinimal (true);

		/* Some regexp multiple match support magic for links highlighting.
		 * Borrowed from Qt support forum */
		pos = 0;
		while ((pos = usernameRx.indexIn (html, pos)) != -1)
		{
			if (usernameRx.cap (2).startsWith ("@"))
			{
				QString before = usernameRx.cap (2);
				if (before.endsWith ("."))
					before.chop (1);
				QString after = " <a href=\"twitter://user/" + before + "\">" + before + "</a> ";
				html.replace (pos, before.length () + 1, after);
				pos += after.length () - 1;			// -1 is needed to match next username starting with space
			}
			else
				pos += usernameRx.matchedLength ();
		}

		/* Twitter tag matching */
		QRegExp tagRx ("(\\s|^)(#[\\w\\d_]+)(\\s|,|$|:|\\.)");
		tagRx.setMinimal (true);

		/* Some regexp multiple match support magic for links highlighting.
		 * Borrowed from Qt support forum */
		pos = 0;
		while ((pos = tagRx.indexIn (html, pos)) != -1)
		{
			if (tagRx.cap (2).startsWith ("#"))
			{
				QString before = tagRx.cap (2);
				if (before.endsWith ("."))
					before.chop (1);
				QString after = " <a href=\"twitter://search/" + before + "\">" + before + "</a> ";
				html.replace (pos, before.length () + 1, after);
				pos += after.length ();
			}
			else
				pos += tagRx.matchedLength ();
		}

		Document_.setHtml (html);
	}

	TwitterUser * Tweet::GetAuthor () const
	{
		return Author_;
	}

	QDateTime Tweet::GetDateTime () const
	{
		return Created_;
	}

	void Tweet::SetDateTime (const QDateTime& datetime)
	{
		Created_ = datetime;
	}

	QTextDocument* Tweet::GetDocument ()
	{
		return &Document_;
	}

	QString Tweet::GetText () const
	{
		return Text_;
	}

	qulonglong Tweet::GetId () const
	{
		return Id_;
	}
}
}
}
