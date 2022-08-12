/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NotificationHandlerTypes.h"
#include "catapult/model/NotificationPublisher.h"

namespace catapult { namespace model { class TransactionRegistry; } }

namespace catapult { namespace notification_handlers {

	class NotificationHandlerAdapter : public EntityHandler {
	private:
		using NotificationHandlerPointer = std::unique_ptr<const NotificationHandler>;
		using NotificationPublisherPointer = std::unique_ptr<const model::NotificationPublisher>;

	public:
		/// Creates a new adapter around \a pHandler and \a pPublisher.
		NotificationHandlerAdapter(NotificationHandlerPointer&& pHandler, NotificationPublisherPointer&& pPublisher);

	public:
		const std::string& name() const override;

		void handle(const model::WeakEntityInfo& entityInfo, const HandlerContext&) const override;

	private:
		NotificationHandlerPointer m_pHandler;
		NotificationPublisherPointer m_pPublisher;
	};
}}
