/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/notification_handlers/NotificationHandlerTypes.h"
#include "tests/test/core/EntityTestUtils.h"
#include <vector>

namespace catapult { namespace mocks {

	/// Mock aggregate notification handler.
	class MockAggregateNotificationHandler : public notification_handlers::AggregateNotificationHandler {
	public:
		void handle(const model::Notification& notification, const notification_handlers::HandlerContext&) const override {
		}

		std::string& name() const override {
		}

		std::vector<std::string> names() const override {
		}
	};
}}
