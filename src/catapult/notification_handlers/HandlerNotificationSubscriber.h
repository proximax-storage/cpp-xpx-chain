/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NotificationHandlerTypes.h"
#include "catapult/model/NotificationSubscriber.h"

namespace catapult { namespace notification_handlers {

	/// A notification subscriber that handle notifications.
	template<typename THandler>
	class HandlerNotificationSubscriber : public model::NotificationSubscriber {
	public:
		/// Creates a handle notification subscriber around \a handler and \a context.
		explicit HandlerNotificationSubscriber(const THandler& handler, const HandlerContext& context)
				: m_handler(handler)
				, m_context(context)
		{}

	public:
		void notify(const model::Notification& notification) override {
			m_handler.handle(notification, m_context);
		}

	private:
		const THandler& m_handler;
		const HandlerContext& m_context;
	};
}}
