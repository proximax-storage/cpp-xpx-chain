/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"
#include <string>

namespace catapult { namespace notification_handlers {

	/// A strongly typed notification handler.
	template<typename TNotification, typename... TArgs>
	class NotificationHandlerTT {
	public:
		/// Notification type.
		using NotificationType = TNotification;

	public:
		virtual ~NotificationHandlerTT() = default;

	public:
		/// Gets the handler name.
		virtual const std::string& name() const = 0;

		/// Handle a single \a notification with contextual information \a args.
		virtual void handle(const TNotification& notification, TArgs&&... args) const = 0;
	};
}}
