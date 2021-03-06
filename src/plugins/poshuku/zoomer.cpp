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

#include "zoomer.h"
#include <QWheelEvent>
#include <QAction>

namespace LeechCraft
{
namespace Poshuku
{
	Zoomer::Zoomer (const ZoomGetter_f& getter, const ZoomSetter_f& setter,
			QObject *parent, const QList<qreal>& zooms)
	: QObject { parent }
	, Zooms_ { zooms }
	, Getter_ { getter }
	, Setter_ { setter }
	{
	}

	void Zoomer::SetActionsTriple (QAction *in, QAction *out, QAction *reset)
	{
		connect (in,
				SIGNAL (triggered ()),
				this,
				SLOT (zoomIn ()));
		connect (out,
				SIGNAL (triggered ()),
				this,
				SLOT (zoomOut ()));
		connect (reset,
				SIGNAL (triggered ()),
				this,
				SLOT (zoomReset ()));
	}

	void Zoomer::InstallScrollFilter (QObject *obj, const std::function<bool (QWheelEvent*)>& cond)
	{
		class ScrollEF : public QObject
		{
			const std::function<bool (QWheelEvent*)> Cond_;
			Zoomer * const Zoomer_;
		public:
			ScrollEF (const std::function<bool (QWheelEvent*)>& cond,
					Zoomer *zoomer, QObject *parent = nullptr)
			: QObject { parent }
			, Cond_ { cond }
			, Zoomer_ { zoomer }
			{
			}

			bool eventFilter (QObject*, QEvent *event) override
			{
				if (event->type () != QEvent::Wheel)
					return false;

				const auto we = static_cast<QWheelEvent*> (event);
				if (!Cond_ (we))
					return false;

				qreal degrees = we->delta () / 8;
				qreal delta = degrees / 150;
				Zoomer_->Setter_ (Zoomer_->Getter_ () + delta);

				return true;
			}
		};

		obj->installEventFilter (new ScrollEF { cond, this, obj });
	}

	int Zoomer::LevelForZoom (qreal zoom) const
	{
		int i = Zooms_.indexOf (zoom);

		if (i >= 0)
			return i;

		for (i = 0; i < Zooms_.size (); ++i)
			if (zoom <= Zooms_ [i])
				break;

		if (i == Zooms_.size ())
			return i - 1;

		if (i == 0)
			return i;

		if (zoom - Zooms_ [i - 1] > Zooms_ [i] - zoom)
			return i;
		else
			return i - 1;
	}

	void Zoomer::SetZoom (qreal zoom)
	{
		Setter_ (zoom);
		emit zoomChanged ();
	}

	void Zoomer::zoomIn ()
	{
		int i = LevelForZoom (Getter_ ());

		if (i < Zooms_.size () - 1)
			SetZoom (Zooms_ [i + 1]);
	}

	void Zoomer::zoomOut ()
	{
		int i = LevelForZoom (Getter_ ());

		if (i > 0)
			SetZoom (Zooms_ [i - 1]);
	}

	void Zoomer::zoomReset ()
	{
		SetZoom (1);
	}
}
}
