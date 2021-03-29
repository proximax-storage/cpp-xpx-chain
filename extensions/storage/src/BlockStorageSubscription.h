/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/io/BlockChangeSubscriber.h"
#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/notification_handlers/AggregateNotificationHandler.h"
#include "catapult/notification_handlers/NotificationHandlerTypes.h"
#include <memory>

namespace catapult { namespace storage {

	using HandlerPointer = std::shared_ptr<const notification_handlers::AggregateNotificationHandler>;

	/// Creates a block storage around \a pluginManager.
	std::unique_ptr<io::BlockChangeSubscriber> CreateBlockStorageSubscription(
			extensions::ProcessBootstrapper& bootstrapper, const HandlerPointer& pValidator);
}}
