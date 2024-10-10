/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "AggregateNotificationHandler.h"
#include "EntityHandler.h"
#include "FunctionalNotificationHandler.h"
#include "catapult/model/VerifiableEntity.h"
#include "catapult/model/WeakEntityInfo.h"
#include "catapult/functions.h"
#include "catapult/plugins/PluginUtils.h"
#include <memory>
#include <vector>

namespace catapult { namespace notification_handlers {

	struct HandlerContext;

	template<typename... TArgs>
	class DemuxHandlerBuilderT;

	template<typename TNotification>
	using NotificationHandlerT = catapult::notification_handlers::NotificationHandlerTT<TNotification, const HandlerContext&>;

	template<typename TNotification>
	using NotificationHandlerPointerT = std::unique_ptr<const NotificationHandlerT<TNotification>>;

	template<typename TNotification>
	using FunctionalNotificationHandlerT =
			catapult::notification_handlers::FunctionalNotificationHandlerTT<TNotification, const HandlerContext&>;

	using EntityHandler = EntityHandlerT<const HandlerContext&>;
	using NotificationHandler = NotificationHandlerT<model::Notification>;

	using AggregateNotificationHandler = AggregateNotificationHandlerT<model::Notification, const HandlerContext&>;
	using DemuxHandlerBuilder = DemuxHandlerBuilderT<const HandlerContext&>;

	using AggregateNotificationHandlerPointer = std::unique_ptr<const notification_handlers::AggregateNotificationHandler>;

/// Declares a stateful handler with \a NAME for notifications of type \a NOTIFICATION_TYPE.
#define DECLARE_HANDLER(NAME, NOTIFICATION_TYPE) \
	NotificationHandlerPointerT<NOTIFICATION_TYPE> Create##NAME##Handler

/// Makes a functional stateful handler with \a NAME around \a HANDLER.
/// \note This macro requires a notification_handlers::Notification alias.
#define MAKE_HANDLER(NAME, HANDLER) \
	std::make_unique<FunctionalNotificationHandlerT<notification_handlers::Notification>>(#NAME "Handler", HANDLER);

/// Makes a functional stateful handler with \a NAME around \a HANDLER for notifications of type \a NOTIFICATION_TYPE.
#define MAKE_HANDLER_WITH_TYPE(NAME, NOTIFICATION_TYPE, HANDLER) \
	std::make_unique<FunctionalNotificationHandlerT<NOTIFICATION_TYPE>>(#NAME "Handler", HANDLER);
}}
