/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "tests/test/core/NotificationTestUtils.h"
#include <vector>

namespace catapult { namespace mocks {

	/// Mock notification observer that captures information about observed notifications and contexts.
	template<typename TNotification>
	class MockNotificationObserverT : public observers::NotificationObserverT<TNotification> {
	public:
		/// Creates a mock observer with a default name.
		MockNotificationObserverT() : MockNotificationObserverT("MockNotificationObserverT")
		{}

		/// Creates a mock observer with \a name.
		explicit MockNotificationObserverT(const std::string& name) : m_name(name)
		{}

	public:
		const std::string& name() const override {
			return m_name;
		}

		void notify(const TNotification& notification, observers::ObserverContext& context) const override {
			m_notificationHashes.push_back(test::CalculateNotificationHash(notification));
			m_notificationTypes.push_back(notification.Type);

			m_contexts.push_back(context);
			m_contextPointers.push_back(&context);
		}

	public:
		/// Returns collected notification hashes.
		const auto& notificationHashes() const {
			return m_notificationHashes;
		}

		/// Returns collected notification types.
		const auto& notificationTypes() const {
			return m_notificationTypes;
		}

		/// Returns collected account keys.
		const auto& accountKeys() const {
			return m_accountKeys;
		}

		/// Returns collected contexts.
		const auto& contexts() const {
			return m_contexts;
		}

		/// Returns collected context pointers.
		const auto& contextPointers() const {
			return m_contextPointers;
		}

	private:
		std::string m_name;
		mutable std::vector<Hash256> m_notificationHashes;
		mutable std::vector<model::NotificationType> m_notificationTypes;
		mutable std::vector<Key> m_accountKeys;
		mutable std::vector<observers::ObserverContext> m_contexts;
		mutable std::vector<const observers::ObserverContext*> m_contextPointers;
	};

	/// Mock notification observer that captures information about observed notifications, contexts and public keys.
	class MockNotificationObserver : public MockNotificationObserverT<model::Notification> {
	private:
		using Base = MockNotificationObserverT<model::Notification>;

	public:
		using Base::Base;

	public:
		void notify(const model::Notification& notification, observers::ObserverContext& context) const override {
			Base::notify(notification, context);

			if (model::Core_Register_Account_Public_Key_v1_Notification == notification.Type) {
				m_accountKeys.push_back(static_cast<const model::AccountPublicKeyNotification<1>&>(notification).PublicKey);
			}
		}

	public:
		/// Returns collected account keys.
		const auto& accountKeys() const {
			return m_accountKeys;
		}

	private:
		mutable std::vector<Key> m_accountKeys;
	};
}}
