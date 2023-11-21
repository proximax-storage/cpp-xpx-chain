/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "AggregateHandlerBuilder.h"
#include "NotificationHandlerTypes.h"
#include <functional>
#include <utility>
#include <vector>

namespace catapult { namespace notification_handlers {

	/// A demultiplexing handler builder.
	template<typename... TArgs>
	class DemuxHandlerBuilderT {
	private:
		template<typename TNotification>
		using NotificationHandlerPointerT = std::unique_ptr<const NotificationHandlerTT<TNotification, TArgs...>>;
		using AggregateHandlerPointer = std::unique_ptr<const AggregateNotificationHandlerT<model::Notification, TArgs...>>;

	public:
		/// Adds a handler (\a pHandler) to the builder that is invoked only when matching notifications are processed.
		template<typename TNotification>
		DemuxHandlerBuilderT& add(NotificationHandlerPointerT<TNotification>&& pHandler) {
			m_builder.add(TNotification::Notification_Type, std::make_unique<NotificationHandlerAdapter<TNotification>>(std::move(pHandler)));
			return *this;
		}

		/// Builds a demultiplexing handler.
		AggregateHandlerPointer build() {
			return m_builder.build();
		}

	private:
		template<typename TNotification>
		class NotificationHandlerAdapter : public NotificationHandlerTT<model::Notification, TArgs...> {
		public:
			NotificationHandlerAdapter(NotificationHandlerPointerT<TNotification>&& pHandler)
				: m_pHandler(std::move(pHandler))
			{}

		public:
			const std::string& name() const override {
				return m_pHandler->name();
			}

			void handle(const model::Notification& notification, TArgs&&... args) const override {
				return m_pHandler->handle(static_cast<const TNotification&>(notification), std::forward<TArgs>(args)...);
			}

		private:
			NotificationHandlerPointerT<TNotification> m_pHandler;
		};

	private:
		AggregateHandlerBuilder<model::Notification, TArgs...> m_builder;
	};
}}
