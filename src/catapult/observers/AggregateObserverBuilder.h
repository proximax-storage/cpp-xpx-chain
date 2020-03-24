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
#include "ObserverTypes.h"
#include "catapult/utils/NamedObject.h"
#include <vector>

namespace catapult { namespace observers {

	/// A strongly typed aggregate notification observer builder.
	template<typename TNotification>
	class AggregateObserverBuilder {
	private:
		using NotificationObserverPointer = NotificationObserverPointerT<TNotification>;
		using NotificationObserverPointerMap = std::map<model::NotificationType, std::vector<NotificationObserverPointer>>;

	public:
		/// Adds \a pObserver to the builder and allows chaining.
		AggregateObserverBuilder& add(model::NotificationType type, NotificationObserverPointer&& pObserver) {
			m_observers[type].push_back(std::move(pObserver));
			return *this;
		}

		/// Builds a strongly typed notification observer.
		AggregateNotificationObserverPointerT<TNotification> build() {
			return std::make_unique<DefaultAggregateNotificationObserver>(std::move(m_observers));
		}

	private:
		class DefaultAggregateNotificationObserver : public AggregateNotificationObserverT<TNotification> {
		public:
			explicit DefaultAggregateNotificationObserver(NotificationObserverPointerMap&& observers)
					: m_observers(std::move(observers))
					, m_name(utils::ReduceNames(utils::ExtractNames(m_observers)))
			{}

		public:
			const std::string& name() const override {
				return m_name;
			}

			std::vector<std::string> names() const override {
				return utils::ExtractNames(m_observers);
			}

			void notify(const TNotification& notification, ObserverContext& context) const override {
				auto observerIter = m_observers.find(notification.Type);
				if (m_observers.end() == observerIter)
					return;

				const auto& observers = observerIter->second;
				if (NotifyMode::Commit == context.Mode)
					notify(observers.cbegin(), observers.cend(), notification, context);
				else
					notify(observers.crbegin(), observers.crend(), notification, context);
			}

		private:
			template<typename TIter>
			void notify(TIter begin, TIter end, const TNotification& notification, ObserverContext& context) const {
				for (auto iter = begin; end != iter; ++iter)
					(*iter)->notify(notification, context);
			}

		private:
			NotificationObserverPointerMap m_observers;
			std::string m_name;
		};

	private:
		NotificationObserverPointerMap m_observers;
	};
}}
