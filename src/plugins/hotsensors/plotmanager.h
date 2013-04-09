/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#pragma once

#include <memory>
#include <QObject>
#include "structures.h"

class QDeclarativeImageProvider;
class QAbstractItemModel;
class QStandardItemModel;

namespace LeechCraft
{
namespace HotSensors
{
	class SensorsManager;
	class SensorsGraphProvider;

	class PlotManager : public QObject
	{
		Q_OBJECT

		std::weak_ptr<SensorsManager> SensorsMgr_;
		QStandardItemModel *Model_;
		SensorsGraphProvider *GraphProvider_;

		int UpdateCounter_;
	public:
		PlotManager (std::weak_ptr<SensorsManager>, QObject* = 0);

		QAbstractItemModel* GetModel () const;
		QDeclarativeImageProvider* GetImageProvider () const;
	public slots:
		void handleHistoryUpdated (const ReadingsHistory_t&);
	};
}
}
