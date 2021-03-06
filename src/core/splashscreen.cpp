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

#include "splashscreen.h"
#include <QStyle>
#include <QStyleOptionProgressBar>
#include <util/sll/slotclosure.h>
#include "loadprocessbase.h"

namespace LeechCraft
{
	void SplashScreen::RegisterLoadProcess (LoadProcessBase *proc)
	{
		Processes_ << proc;
		repaint ();

		connect (proc,
				SIGNAL (changed ()),
				this,
				SLOT (repaint ()));

		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[this, proc]
			{
				Processes_.removeOne (proc);
				repaint ();
			},
			proc,
			SIGNAL (destroyed (QObject*)),
			this
		};
	}

	void SplashScreen::drawContents (QPainter *painter)
	{
		QSplashScreen::drawContents (painter);

		const auto margin = 10;
		int ypos = margin;
		const auto height = 1.3 * fontMetrics ().height ();
		for (const auto proc : Processes_)
		{
			QStyleOptionProgressBar opt;
			opt.initFrom (this);

			opt.rect.setY (ypos);
			opt.rect.setHeight (height);
			opt.rect.setWidth (width ());
			ypos += height;

			auto& p = opt.palette;
			p.setColor (QPalette::Base, Qt::transparent);
			p.setColor (QPalette::Window, Qt::transparent);
			p.setColor (QPalette::Highlight, "#FF3B00");
			p.setColor (QPalette::Text, "#FF3B00");
			p.setColor (QPalette::HighlightedText, "#1B181F");

			opt.minimum = proc->GetMin ();
			opt.maximum = proc->GetMax ();
			opt.progress = proc->GetValue ();
			opt.text = proc->GetTitle () + " " + tr ("(%1 of %2)").arg (opt.progress).arg (opt.maximum);
			opt.textVisible = true;
			opt.textAlignment = Qt::AlignCenter;

			style ()->drawControl (QStyle::CE_ProgressBar, &opt, painter, this);
		}
	}
}
