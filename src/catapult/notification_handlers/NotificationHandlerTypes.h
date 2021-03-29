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
	class AggregateEntityHandlerT;

	template<typename... TArgs>
	class DemuxHandlerBuilderT;

	/// A vector of handlers.
	template<typename... TArgs>
	using HandlerVectorT = std::vector<std::unique_ptr<const EntityHandlerT<TArgs...>>>;

	/// A handler function.
	using HandlerFunction = std::function<void (const model::WeakEntityInfo&)>;

	/// A vector of handler functions.
	using HandlerFunctions = std::vector<HandlerFunction>;

	template<typename TNotification>
	using NotificationHandlerT = catapult::notification_handlers::NotificationHandlerTT<TNotification, const HandlerContext&>;

	template<typename TNotification>
	using NotificationHandlerPointerT = std::unique_ptr<const NotificationHandlerT<TNotification>>;

	template<typename TNotification>
	using FunctionalNotificationHandlerT =
			catapult::notification_handlers::FunctionalNotificationHandlerTT<TNotification, const HandlerContext&>;

	using EntityHandler = EntityHandlerT<const HandlerContext&>;
	using NotificationHandler = NotificationHandlerT<model::Notification>;

	using AggregateEntityHandler = AggregateEntityHandlerT<const HandlerContext&>;
	using AggregateNotificationHandler = AggregateNotificationHandlerT<model::Notification, const HandlerContext&>;
	using DemuxHandlerBuilder = DemuxHandlerBuilderT<const HandlerContext&>;

/// Declares a stateful handler with \a NAME for notifications of type \a NOTIFICATION_TYPE.
#define DECLARE_HANDLER(NAME, NOTIFICATION_TYPE) \
	stateful::NotificationHandlerPointerT<NOTIFICATION_TYPE> Create##NAME##Handler

/// Makes a functional stateful handler with \a NAME around \a HANDLER.
/// \note This macro requires a notification_handlers::Notification alias.
#define MAKE_HANDLER(NAME, HANDLER) \
	std::make_unique<stateful::FunctionalNotificationHandlerT<notification_handlers::Notification>>(#NAME "Handler", HANDLER);

/// Defines a functional stateful handler with \a NAME around \a HANDLER.
/// \note This macro requires a notification_handlers::Notification alias.
#define DEFINE_HANDLER(NAME, HANDLER) \
	DECLARE_HANDLER(NAME, notification_handlers::Notification)() { \
		return MAKE_Handler(NAME, HANDLER); \
	}

/// Defines a functional stateful handler with \a NAME around \a HANDLER for notifications of type \a NOTIFICATION_TYPE.
#define DEFINE_HANDLER_WITH_TYPE(NAME, NOTIFICATION_TYPE, HANDLER) \
	DECLARE_STATEFUL_HANDLER(NAME, NOTIFICATION_TYPE)() { \
		return std::make_unique<stateful::FunctionalNotificationHandlerT<NOTIFICATION_TYPE>>(#NAME "Handler", HANDLER); \
	}
}}
