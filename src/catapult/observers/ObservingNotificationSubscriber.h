/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/NotificationSubscriber.h"

namespace catapult { namespace observers {

	class ObservingNotificationSubscriber : public model::NotificationSubscriber {
	public:
		explicit ObservingNotificationSubscriber(const NotificationObserver& observer, ObserverContext& context)
				: m_observer(observer)
				, m_context(context)
		{}

	public:
		void notify(const model::Notification& notification) override {
			if (!IsSet(notification.Type, model::NotificationChannel::Observer))
				return;

			m_observer.notify(notification, m_context);
		}

	private:
		const NotificationObserver& m_observer;
		ObserverContext& m_context;
	};
}}
