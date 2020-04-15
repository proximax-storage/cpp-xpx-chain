/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"

namespace catapult { namespace mocks {

	template<model::NotificationType notificationType>
	struct MockNotification : public model::Notification {
	public:
		static constexpr auto Notification_Type = notificationType;

	public:
		explicit MockNotification()
			: Notification(notificationType, sizeof(MockNotification))
		{}
	};
}}
