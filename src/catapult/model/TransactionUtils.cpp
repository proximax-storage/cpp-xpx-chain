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

#include "TransactionUtils.h"
#include "Address.h"
#include "NotificationPublisher.h"
#include "NotificationSubscriber.h"
#include "ResolverContext.h"
#include "Transaction.h"

namespace catapult { namespace model {

	namespace {
		class AddressCollector : public NotificationSubscriber {
		public:
			explicit AddressCollector(NetworkIdentifier networkIdentifier) : m_networkIdentifier(networkIdentifier)
			{}

		public:
			void notify(const Notification& notification) override {
				if (Core_Register_Account_Address_Notification == notification.Type)
					switch (notification.getVersion()) {
					case 1:
						m_addresses.insert(static_cast<const AccountAddressNotification<1>&>(notification).Address);
						break;
					default:
						CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of AccountAddressNotification", notification.getVersion());
					}
				else if (Core_Register_Account_Public_Key_Notification == notification.Type)
					switch (notification.getVersion()) {
					case 1:
						m_addresses.insert(toAddress(static_cast<const AccountPublicKeyNotification<1>&>(notification).PublicKey));
						break;
					default:
						CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of AccountPublicKeyNotification", notification.getVersion());
					}
			}

		public:
			const UnresolvedAddressSet& addresses() const {
				return m_addresses;
			}

		private:
			UnresolvedAddress toAddress(const Key& publicKey) const {
				auto resolvedAddress = PublicKeyToAddress(publicKey, m_networkIdentifier);

				UnresolvedAddress unresolvedAddress;
				std::memcpy(unresolvedAddress.data(), resolvedAddress.data(), resolvedAddress.size());
				return unresolvedAddress;
			}

		private:
			NetworkIdentifier m_networkIdentifier;
			UnresolvedAddressSet m_addresses;
		};
	}

	UnresolvedAddressSet ExtractAddresses(const Transaction& transaction, const NotificationPublisher& notificationPublisher) {
		WeakEntityInfo weakInfo(transaction);
		AddressCollector sub(NetworkIdentifier(weakInfo.entity().Network()));
		notificationPublisher.publish(weakInfo, sub);
		return sub.addresses();
	}
}}
