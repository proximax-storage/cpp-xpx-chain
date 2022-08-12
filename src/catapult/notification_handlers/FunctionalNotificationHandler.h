/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NotificationHandler.h"
#include <functional>

namespace catapult { namespace notification_handlers {

	/// A notification handler implementation that wraps a function.
	template<typename TNotification, typename... TArgs>
	class FunctionalNotificationHandlerTT : public NotificationHandlerTT<TNotification, TArgs...> {
	private:
		using FunctionType = std::function<void (const TNotification&, TArgs&&...)>;

	public:
		/// Creates a functional notification handler around \a func with \a name.
		explicit FunctionalNotificationHandlerTT(const std::string& name, const FunctionType& func)
				: m_name(name)
				, m_func(func)
		{}

	public:
		const std::string& name() const override {
			return m_name;
		}

		void handle(const TNotification& notification, TArgs&&... args) const override {
			return m_func(notification, std::forward<TArgs>(args)...);
		}

	private:
		std::string m_name;
		FunctionType m_func;
	};
}}
