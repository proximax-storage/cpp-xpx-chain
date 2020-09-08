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

namespace catapult { namespace model {

	namespace {
		class AddressCollector : public NotificationSubscriber {
		public:
			explicit AddressCollector(NetworkIdentifier networkIdentifier, const ExtractorContext& extractorContext)
			: m_networkIdentifier(networkIdentifier)
			, m_extractorContext(extractorContext)
			{}

		public:
			void notify(const Notification& notification) override {
				if (Core_Register_Account_Address_v1_Notification == notification.Type) {
					auto addresses = m_extractorContext.extract(
							static_cast<const AccountAddressNotification <1>&>(notification).Address);
					m_addresses.insert(addresses.begin(), addresses.end());

				} else if (Core_Register_Account_Public_Key_v1_Notification == notification.Type) {
					auto keys = m_extractorContext.extract(
							static_cast<const AccountPublicKeyNotification <1>&>(notification).PublicKey);
					for (const auto& key : keys)
						m_addresses.insert(toAddress(key));
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
			const ExtractorContext& m_extractorContext;
		};
	}

	UnresolvedAddressSet ExtractAddresses(const Transaction& transaction, const Hash256& hash, const Height& height,
			const NotificationPublisher& notificationPublisher, const ExtractorContext& extractorContext) {
		WeakEntityInfo weakInfo(transaction, hash, height);
		AddressCollector sub(NetworkIdentifier(weakInfo.entity().Network()), extractorContext);
		notificationPublisher.publish(weakInfo, sub);
		return sub.addresses();
	}
}}
