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
#include "catapult/model/NotificationSubscriber.h"
#include "tests/test/core/NotificationTestUtils.h"
#include <vector>

namespace catapult { namespace mocks {

	// region MockNotificationSubscriber

	/// A mock NotificationSubscriber implementation that stores account and balance transfer notifications.
	class MockNotificationSubscriber : public model::NotificationSubscriber {
	public:
		void notify(const model::Notification& notification) override {
			m_notificationTypes.push_back(notification.Type);

			// use if/else instead of switch to work around VS warning
			if (model::Core_Register_Account_Address_v1_Notification == notification.Type)
				m_addresses.push_back(test::CastToDerivedNotification<model::AccountAddressNotification<1>>(notification).Address);
			else if (model::Core_Register_Account_Public_Key_v1_Notification == notification.Type)
				m_keys.push_back(test::CastToDerivedNotification<model::AccountPublicKeyNotification<1>>(notification).PublicKey);
			else if (model::Core_Balance_Transfer_v1_Notification == notification.Type)
				addTransfer(test::CastToDerivedNotification<model::BalanceTransferNotification<1>>(notification));
		}

	public:
		/// Gets the number of notifications.
		size_t numNotifications() const {
			return m_notificationTypes.size();
		}

		/// Returns collected notification types.
		const auto& notificationTypes() const {
			return m_notificationTypes;
		}

	public:
		/// Gets the number of visited addresses.
		size_t numAddresses() const {
			return m_addresses.size();
		}

		/// Gets the number of visited keys.
		size_t numKeys() const {
			return m_keys.size();
		}

		/// Returns \c true if \a address was visited.
		bool contains(const UnresolvedAddress& address) const {
			return m_addresses.cend() != std::find(m_addresses.cbegin(), m_addresses.cend(), address);
		}

		/// Returns \c true if \a publicKey was visited.
		bool contains(const Key& publicKey) const {
			return m_keys.cend() != std::find(m_keys.cbegin(), m_keys.cend(), publicKey);
		}

	private:
		void addTransfer(const model::BalanceTransferNotification<1>& notification) {
			CATAPULT_LOG(debug) << "visited " << notification.MosaicId << " transfer of " << notification.Amount << " units";
			m_transfers.push_back(Transfer(notification));
		}

	public:
		/// Gets the number of visited transfers.
		size_t numTransfers() const {
			return m_transfers.size();
		}

		/// Returns \c true if a transfer of \a amount units of \a mosaicId from \a sender to \a recipient was visited.
		bool contains(const Key& sender, const UnresolvedAddress& recipient, UnresolvedMosaicId mosaicId, Amount amount) const {
			auto targetTransfer = Transfer(sender, recipient, mosaicId, amount);
			return std::any_of(m_transfers.cbegin(), m_transfers.cend(), [&targetTransfer](const auto& transfer) {
				return targetTransfer.Sender == transfer.Sender
						&& targetTransfer.Recipient == transfer.Recipient
						&& targetTransfer.MosaicId == transfer.MosaicId
						&& targetTransfer.Amount == transfer.Amount;
			});
		}

	private:
		struct Transfer {
		public:
			explicit Transfer(const model::BalanceTransferNotification<1>& notification)
					: Transfer(notification.Sender, notification.Recipient, notification.MosaicId, catapult::Amount(notification.Amount))
			{}

			explicit Transfer(const Key& sender, const UnresolvedAddress& recipient, UnresolvedMosaicId mosaicId, Amount amount)
					: Sender(sender)
					, Recipient(recipient)
					, MosaicId(mosaicId)
					, Amount(amount)
			{}

		public:
			Key Sender;
			UnresolvedAddress Recipient;
			UnresolvedMosaicId MosaicId;
			catapult::Amount Amount;
		};

	private:
		std::vector<model::NotificationType> m_notificationTypes;
		std::vector<UnresolvedAddress> m_addresses;
		std::vector<Key> m_keys;
		std::vector<Transfer> m_transfers;
	};

	// endregion

	// region MockTypedNotificationSubscriber

	/// A mock NotificationSubscriber implementation that stores notifications of a specific type.
	template<typename TNotification>
	class MockTypedNotificationSubscriber : public model::NotificationSubscriber {
	public:
		/// Creates a new notification subscriber for capturing notifications of \a type.
		explicit MockTypedNotificationSubscriber(model::NotificationType type)
				: m_type(type)
				, m_numNotifications(0)
		{}

		/// Creates a new notification subscriber.
		MockTypedNotificationSubscriber() : MockTypedNotificationSubscriber(TNotification::Notification_Type)
		{}

	public:
		void notify(const model::Notification& notification) override {
			++m_numNotifications;
			if (model::AreEqualExcludingChannel(m_type, notification.Type))
				m_matchingNotifications.push_back(test::CastToDerivedNotification<TNotification>(notification));
		}

	public:
		/// Gets the number of notifications.
		size_t numNotifications() const {
			return m_numNotifications;
		}

		/// Gets the number of matching notifications.
		size_t numMatchingNotifications() const {
			return m_matchingNotifications.size();
		}

		/// Gets the matching notifications.
		const auto& matchingNotifications() const {
			return m_matchingNotifications;
		}

	private:
		model::NotificationType m_type;
		size_t m_numNotifications;
		std::vector<TNotification> m_matchingNotifications;
	};

	// endregion
}}
