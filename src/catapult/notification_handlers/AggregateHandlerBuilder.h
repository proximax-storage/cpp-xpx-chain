/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NotificationHandlerTypes.h"
#include "catapult/utils/NamedObject.h"
#include <utility>
#include <vector>

namespace catapult { namespace notification_handlers {

	/// A strongly typed aggregate notification handler builder.
	template<typename TNotification, typename... TArgs>
	class AggregateHandlerBuilder {
	private:
		using NotificationHandlerPointer = std::unique_ptr<const NotificationHandlerTT<TNotification, TArgs...>>;
		using NotificationHandlerPointerMap = std::map<model::NotificationType, std::vector<NotificationHandlerPointer>>;
		using AggregateHandlerPointer = std::unique_ptr<const AggregateNotificationHandlerT<TNotification, TArgs...>>;

	public:
		/// Adds \a pHandler to the builder and allows chaining.
		AggregateHandlerBuilder& add(model::NotificationType type, NotificationHandlerPointer&& pHandler) {
			m_handlers[type].push_back(std::move(pHandler));
			return *this;
		}

		/// Builds a strongly typed notification handler that ignores suppressed failures according to \a isSuppressedFailure.
		AggregateHandlerPointer build() {
			return std::make_unique<DefaultAggregateNotificationHandler>(std::move(m_handlers));
		}

	private:
		class DefaultAggregateNotificationHandler : public AggregateNotificationHandlerT<TNotification, TArgs...> {
		public:
			explicit DefaultAggregateNotificationHandler(
					NotificationHandlerPointerMap&& handlers)
					: m_handlers(std::move(handlers))
					, m_name(utils::ReduceNames(utils::ExtractNames(m_handlers)))
			{}

		public:
			const std::string& name() const override {
				return m_name;
			}

			std::vector<std::string> names() const override {
				return utils::ExtractNames(m_handlers);
			}

			void handle(const TNotification& notification, TArgs&&... args) const override {
				auto handlerIter = m_handlers.find(notification.Type);
				if (m_handlers.end() == handlerIter)
					return;

				for (const auto& pHandler : handlerIter->second) {
					pHandler->handle(notification, std::forward<TArgs>(args)...);
				}
			}

		private:
			NotificationHandlerPointerMap m_handlers;
			std::string m_name;
		};

	private:
		NotificationHandlerPointerMap m_handlers;
	};
}}
