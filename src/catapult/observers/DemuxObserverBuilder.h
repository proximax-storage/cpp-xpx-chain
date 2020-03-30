/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "AggregateObserverBuilder.h"
#include "ObserverTypes.h"
#include <functional>
#include <vector>

namespace catapult { namespace observers {

	/// A demultiplexing observer builder.
	class DemuxObserverBuilder {
	public:
		/// Adds an observer (\a pObserver) to the builder that is invoked only when matching notifications are processed.
		template<typename TNotification>
		DemuxObserverBuilder& add(NotificationObserverPointerT<TNotification>&& pObserver) {
			m_builder.add(TNotification::Notification_Type, std::make_unique<NotificationObserverAdapter<TNotification>>(std::move(pObserver)));
			return *this;
		}

		/// Builds a demultiplexing observer.
		AggregateNotificationObserverPointerT<model::Notification> build() {
			return m_builder.build();
		}

	private:
		template<typename TNotification>
		class NotificationObserverAdapter : public NotificationObserver {
		public:
			NotificationObserverAdapter(NotificationObserverPointerT<TNotification>&& pObserver)
				: m_pObserver(std::move(pObserver))
			{}

		public:
			const std::string& name() const override {
				return m_pObserver->name();
			}

			void notify(const model::Notification& notification, ObserverContext& context) const override {
				m_pObserver->notify(static_cast<const TNotification&>(notification), context);
			}

		private:
			NotificationObserverPointerT<TNotification> m_pObserver;
		};

	private:
		AggregateObserverBuilder<model::Notification> m_builder;
	};
}}
