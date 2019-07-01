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

#include "catapult/model/TransactionUtils.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "catapult/model/Address.h"
#include "catapult/model/NotificationPublisher.h"
#include "catapult/model/NotificationSubscriber.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS TransactionUtilsTests

	namespace {
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;

		class MockNotificationPublisher : public NotificationPublisher {
		public:
			enum class Mode { Address, Public_Key, Other };

		public:
			explicit MockNotificationPublisher(Mode mode, VersionType notificationVersion = 1)
				: m_mode(mode)
				, m_notificationVersion(notificationVersion)
			{}

		public:
			void publish(const WeakEntityInfo& entityInfo, NotificationSubscriber& sub) const override {
				const auto& transaction = entityInfo.cast<mocks::MockTransaction>().entity();

				if (Mode::Address == m_mode) {
					auto senderAddress = PublicKeyToAddress(transaction.Signer, Network_Identifier);
					auto recipientAddress = PublicKeyToAddress(transaction.Recipient, Network_Identifier);
					switch (m_notificationVersion) {
					case 1:
						sub.notify(AccountAddressNotification<1>(extensions::CopyToUnresolvedAddress(senderAddress)));
						sub.notify(AccountAddressNotification<1>(extensions::CopyToUnresolvedAddress(recipientAddress)));
						break;
					default:
						CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of AccountAddressNotification", m_notificationVersion);
					}
				} else if (Mode::Public_Key == m_mode) {
					switch (m_notificationVersion) {
					case 1:
						sub.notify(AccountPublicKeyNotification<1>(transaction.Signer));
						sub.notify(AccountPublicKeyNotification<1>(transaction.Recipient));
						break;
					default:
						CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of AccountPublicKeyNotification", m_notificationVersion);
					}
				} else {
					VersionSet supportedVersions{ 0 };
					switch (m_notificationVersion) {
					case 1:
						sub.notify(EntityNotification<1>(transaction.Network(), supportedVersions, transaction.EntityVersion()));
						break;
					default:
						CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of EntityNotification", m_notificationVersion);
					}
				}
			}

		private:
			Mode m_mode;
			VersionType m_notificationVersion;
		};

		void RunExtractAddressesTest(MockNotificationPublisher::Mode mode) {
			// Arrange:
			auto pTransaction = mocks::CreateMockTransactionWithSignerAndRecipient(
					test::GenerateRandomData<Key_Size>(),
					test::GenerateRandomData<Key_Size>());
			auto senderAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(pTransaction->Signer, Network_Identifier));
			auto recipientAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(pTransaction->Recipient, Network_Identifier));

			MockNotificationPublisher notificationPublisher(mode);

			// Act:
			auto addresses = ExtractAddresses(*pTransaction, notificationPublisher);

			// Assert:
			EXPECT_EQ(2u, addresses.size());
			EXPECT_CONTAINS(addresses, senderAddress);
			EXPECT_CONTAINS(addresses, recipientAddress);
		}
	}

	TEST(TEST_CLASS, ExtractAddressesExtractsAddressesFromAddressNotifications) {
		// Assert:
		RunExtractAddressesTest(MockNotificationPublisher::Mode::Address);
	}

	TEST(TEST_CLASS, ExtractAddressesExtractsAddressesFromPublicKeyNotifications) {
		// Assert:
		RunExtractAddressesTest(MockNotificationPublisher::Mode::Public_Key);
	}

	TEST(TEST_CLASS, ExtractAddressesDoesNotExtractAddressesFromOtherNotifications) {
		// Arrange:
		auto pTransaction = mocks::CreateMockTransactionWithSignerAndRecipient(
				test::GenerateRandomData<Key_Size>(),
				test::GenerateRandomData<Key_Size>());

		MockNotificationPublisher notificationPublisher(MockNotificationPublisher::Mode::Other);

		// Act:
		auto addresses = ExtractAddresses(*pTransaction, notificationPublisher);

		// Assert:
		EXPECT_TRUE(addresses.empty());
	}
}}
