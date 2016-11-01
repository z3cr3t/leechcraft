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

#include "itemhandlerradio.h"
#include <QLabel>
#include <QGridLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QtDebug>
#include "../radiogroup.h"

namespace LeechCraft
{
	ItemHandlerRadio::ItemHandlerRadio ()
	{
	}

	ItemHandlerRadio::~ItemHandlerRadio ()
	{
	}

	bool ItemHandlerRadio::CanHandle (const QDomElement& element) const
	{
		return element.attribute ("type") == "radio";
	}

	void ItemHandlerRadio::Handle (const QDomElement& item, QWidget *pwidget)
	{
		QGridLayout *lay = qobject_cast<QGridLayout*> (pwidget->layout ());
		RadioGroup *group = new RadioGroup (XSD_->GetWidget ());
		group->setObjectName (item.attribute ("property"));

		QStringList searchTerms;
		QDomElement option = item.firstChildElement ("option");
		while (!option.isNull ())
		{
			QRadioButton *button = new QRadioButton (XSD_->GetLabel (option));
			searchTerms << button->text ();
			XSD_->SetTooltip (button, option);
			button->setObjectName (option.attribute ("name"));
			group->AddButton (button,
					option.hasAttribute ("default") &&
					option.attribute ("default") == "true");
			option = option.nextSiblingElement ("option");
		}

		QVariant value = XSD_->GetValue (item);

		connect (group,
				SIGNAL (valueChanged ()),
				this,
				SLOT (updatePreferences ()));

		QGroupBox *box = new QGroupBox (XSD_->GetLabel (item));
		QVBoxLayout *layout = new QVBoxLayout ();
		box->setLayout (layout);
		layout->addWidget (group);

		searchTerms << box->title ();
		group->setProperty ("ItemHandler", QVariant::fromValue<QObject*> (this));
		group->setProperty ("SearchTerms", searchTerms);

		lay->addWidget (box, lay->rowCount (), 0);
	}

	void ItemHandlerRadio::SetValue (QWidget *widget, const QVariant& value) const
	{
		RadioGroup *radiogroup = qobject_cast<RadioGroup*> (widget);
		if (!radiogroup)
		{
			qWarning () << Q_FUNC_INFO
				<< "not a RadioGroup"
				<< widget;
			return;
		}
		radiogroup->SetValue (value.toString ());
	}

	QVariant ItemHandlerRadio::GetObjectValue (QObject *object) const
	{
		RadioGroup *radiogroup = qobject_cast<RadioGroup*> (object);
		if (!radiogroup)
		{
			qWarning () << Q_FUNC_INFO
				<< "not a RadioGroup"
				<< object;
			return QVariant ();
		}
		return radiogroup->GetValue ();
	}
}
