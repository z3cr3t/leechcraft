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

#include <QDateTime>
#include <QUrl>
#include <QStringList>
#include <QtPlugin>

namespace Media
{
	/** @brief Enum describing if and how an event is attended by user.
	 *
	 * @sa EventInfo, IEventsProvider
	 */
	enum class EventAttendType
	{
		/** @brief The user won't attend this event.
		 */
		None,

		/** @brief The user is interested and maybe will attend the event.
		 */
		Maybe,

		/** @brief The user surely will attend the event.
		 */
		Surely
	};

	/** @brief A structure describing an event like a gig or a festival.
	 *
	 * @sa EventInfo, IEventsProvider
	 */
	struct EventInfo
	{
		/** @brief The internal ID of the event.
		 */
		qint64 ID_;

		/** @brief The name of the event.
		 */
		QString Name_;

		/** @brief The description of the event.
		 */
		QString Description_;

		/** @brief The date the event will happen.
		 */
		QDateTime Date_;

		/** @brief The URL of a page describing the event in more detail.
		 */
		QUrl URL_;

		/** @brief A thumb image associated with this event.
		 */
		QUrl SmallImage_;

		/** @brief A big, preferably poster-size image of this event.
		 */
		QUrl BigImage_;

		/** @brief The list of all artists present on this event.
		 *
		 * This list should include the headliner as well.
		 */
		QStringList Artists_;

		/** @brief The name of the headliner of this event.
		 *
		 * The name of the headliner of this event.
		 */
		QString Headliner_;

		/** @brief The associated tags like musical genre of bands.
		 */
		QStringList Tags_;

		/** @brief The current number of attendees or -1 if not known.
		 */
		int Attendees_;

		/** @brief The name of the club or other place this event will
		 * be in.
		 */
		QString PlaceName_;

		/** @brief Latitude of the place.
		 *
		 * The geographical latitude of the place this event will be in,
		 * or -1 if not known.
		 */
		double Latitude_;

		/** @brief Longitude of the place.
		 *
		 * The geographical longitude of the place this event will be in,
		 * or -1 if not known.
		 */
		double Longitude_;

		/** @brief The city this event will happen in.
		 */
		QString City_;

		/** @brief The address of the place this event will happen in.
		 */
		QString Address_;

		/** @brief Whether this event can be attended.
		 */
		bool CanBeAttended_;

		/** @brief Current attendance status by the user.
		 */
		EventAttendType AttendType_;
	};

	/** @brief A list of events.
	 */
	typedef QList<EventInfo> EventInfos_t;

	/** @brief Interface for plugins that can provide events.
	 *
	 * Plugins that can provide nearby or recommended events based on
	 * user's location or musical taste should implement this interface.
	 *
	 * Fetching recommended events is asynchronous in nature, so one
	 * should request updating the list of recommended events via
	 * UpdateRecommendedEvents() and then listen for the
	 * gotRecommendedEvents() signal.
	 */
	class Q_DECL_EXPORT IEventsProvider
	{
	public:
		virtual ~IEventsProvider () {}

		/** @brief Returns the service name.
		 *
		 * This string returns a human-readable string with the service
		 * name, like "Last.FM".
		 *
		 * @return The human-readable service name.
		 */
		virtual QString GetServiceName () const = 0;

		/** @brief Requests re-fetching the list of recommended events.
		 *
		 * The gotRecommendedEvents() signal will be emitted after new
		 * recommended events are fetched.
		 *
		 * @sa gotRecommendedEvents().
		 */
		virtual void UpdateRecommendedEvents () = 0;

		/** @brief Updates the event attendance status, if possible.
		 *
		 * This function marks the event attendance status as status, if
		 * the service supports it. The event is identified by its ID.
		 *
		 * @param[in] id The ID of the event (EventInfo::ID_).
		 * @param[in] status The new event attendance status.
		 */
		virtual void AttendEvent (qint64 id, EventAttendType status) = 0;
	protected:
		/** @brief Emitted when a list of recommended events is fetched.
		 *
		 * @param[out] events The current list of recommended events.
		 */
		virtual void gotRecommendedEvents (const EventInfos_t& events) = 0;
	};
}

Q_DECLARE_INTERFACE (Media::IEventsProvider, "org.LeechCraft.Media.IEventsProvider/1.0");
