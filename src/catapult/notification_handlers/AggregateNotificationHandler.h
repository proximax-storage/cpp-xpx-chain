/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NotificationHandler.h"
#include <vector>

namespace catapult { namespace notification_handlers {

	/// A strongly typed aggregate notification handler.
	template<typename TNotification, typename... TArgs>
	class AggregateNotificationHandlerT : public NotificationHandlerTT<TNotification, TArgs...> {
	public:
		/// Gets the names of all sub handlers.
		virtual std::vector<std::string> names() const = 0;
	};
}}
