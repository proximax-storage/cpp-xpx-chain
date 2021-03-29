/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlerAdapter.h"
#include "HandlerNotificationSubscriber.h"

namespace catapult { namespace notification_handlers {

	NotificationHandlerAdapter::NotificationHandlerAdapter(
			NotificationHandlerPointer&& pHandler,
			NotificationPublisherPointer&& pPublisher)
			: m_pHandler(std::move(pHandler))
			, m_pPublisher(std::move(pPublisher))
	{}

	const std::string& NotificationHandlerAdapter::name() const {
		return m_pHandler->name();
	}

	void NotificationHandlerAdapter::handle(const model::WeakEntityInfo& entityInfo, const HandlerContext& context) const {
		HandlerNotificationSubscriber sub(*m_pHandler, context);
		m_pPublisher->publish(entityInfo, sub);
	}
}}
