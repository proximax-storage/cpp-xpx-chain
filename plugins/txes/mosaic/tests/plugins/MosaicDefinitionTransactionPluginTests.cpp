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

#include "src/config/MosaicConfiguration.h"
#include "src/model/MosaicDefinitionTransaction.h"
#include "src/model/MosaicNotifications.h"
#include "src/plugins/MosaicDefinitionTransactionPlugin.h"
#include "catapult/model/Address.h"
#include "catapult/plugins/PluginUtils.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS MosaicDefinitionTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(MosaicDefinition, std::shared_ptr<config::LocalNodeConfigurationHolder>, 3, 3,)

		constexpr UnresolvedMosaicId Currency_Mosaic_Id(1234);
		constexpr auto Transaction_Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 3);

		auto CreateMosaicConfiguration(Amount fee) {
			auto pluginConfig = config::MosaicConfiguration::Uninitialized();
			pluginConfig.MosaicRentalFeeSinkPublicKey = test::GenerateRandomData<Key_Size>();
			pluginConfig.MosaicRentalFee = fee;
			return pluginConfig;
		}

		auto CreateBlockChainConfiguration(config::MosaicConfiguration pluginConfig) {
			auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
			blockChainConfig.CurrencyMosaicId = MosaicId{Currency_Mosaic_Id.unwrap()};
			blockChainConfig.Network.PublicKey = test::GenerateRandomData<Key_Size>();
			blockChainConfig.Network.Identifier = model::NetworkIdentifier::Mijin_Test;
			blockChainConfig.SetPluginConfiguration(PLUGIN_NAME(mosaic), pluginConfig);
			return blockChainConfig;
		}

		auto GetSinkAddress(const config::MosaicConfiguration& pluginConfig, const model::BlockChainConfiguration& blockChainConfig) {
			auto address = PublicKeyToAddress(pluginConfig.MosaicRentalFeeSinkPublicKey, blockChainConfig.Network.Identifier);
			UnresolvedAddress sinkAddress;
			std::memcpy(sinkAddress.data(), address.data(), address.size());
			return sinkAddress;
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(
			TEST_CLASS,
			,
			,Entity_Type_Mosaic_Definition,
		std::make_shared<config::MockLocalNodeConfigurationHolder>(CreateBlockChainConfiguration(CreateMosaicConfiguration(Amount(0)))))

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pConfigHolder = std::make_shared<config::MockLocalNodeConfigurationHolder>(CreateBlockChainConfiguration(CreateMosaicConfiguration(Amount(0))));
		auto pPlugin = TTraits::CreatePlugin(pConfigHolder);

		typename TTraits::TransactionType transaction;
		transaction.Version = Transaction_Version;
		transaction.Size = 0;
		transaction.PropertiesHeader.Count = 2;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 2 * sizeof(MosaicProperty), realSize);
	}

	PLUGIN_TEST(CanExtractAccounts) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pluginConfig = CreateMosaicConfiguration(Amount(0));
		auto blockChainConfig = CreateBlockChainConfiguration(pluginConfig);
		auto pPlugin = TTraits::CreatePlugin(std::make_shared<config::MockLocalNodeConfigurationHolder>(blockChainConfig));

		typename TTraits::TransactionType transaction;
		transaction.Version = Transaction_Version;
		transaction.PropertiesHeader.Count = 0;
		test::FillWithRandomData(transaction.Signer);

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		EXPECT_EQ(6u, sub.numNotifications());
		EXPECT_EQ(0u, sub.numAddresses());
		EXPECT_EQ(1u, sub.numKeys());

		EXPECT_TRUE(sub.contains(pluginConfig.MosaicRentalFeeSinkPublicKey));
	}

	// region balance change

	namespace {
		template<typename TTraits, typename TAssertTransfers>
		void RunBalanceChangeObserverTest(bool isSignerExempt, TAssertTransfers assertTransfers) {
			// Arrange:
			mocks::MockNotificationSubscriber sub;
			auto pluginConfig = CreateMosaicConfiguration(Amount(987));
			auto blockChainConfig = CreateBlockChainConfiguration(pluginConfig);
			auto sinkAddress = GetSinkAddress(pluginConfig, blockChainConfig);

			// - prepare the transaction
			typename TTraits::TransactionType transaction;
			transaction.Version = Transaction_Version;
			transaction.PropertiesHeader.Count = 0;
			test::FillWithRandomData(transaction.Signer);
			if (isSignerExempt)
				transaction.Signer = blockChainConfig.Network.PublicKey;
			auto pPlugin = TTraits::CreatePlugin(std::make_shared<config::MockLocalNodeConfigurationHolder>(blockChainConfig));

			// Act:
			test::PublishTransaction(*pPlugin, transaction, sub);

			// Assert:
			assertTransfers(sub, transaction.Signer, sinkAddress);
		}
	}

	PLUGIN_TEST(RentalFeeIsExtractedFromNonNemesis) {
		// Arrange:
		RunBalanceChangeObserverTest<TTraits>(false, [](const auto& sub, const auto& signer, const auto& recipient) {
			// Assert:
			EXPECT_EQ(6u, sub.numNotifications());
			EXPECT_EQ(1u, sub.numTransfers());
			EXPECT_TRUE(sub.contains(signer, recipient, Currency_Mosaic_Id, Amount(987)));
		});
	}

	PLUGIN_TEST(RentalFeeIsExemptedFromNemesis) {
		// Arrange:
		RunBalanceChangeObserverTest<TTraits>(true, [](const auto& sub, const auto&, const auto&) {
			// Assert:
			EXPECT_EQ(4u, sub.numNotifications());
			EXPECT_EQ(0u, sub.numTransfers());
		});
	}

	// endregion

	// region registration

	namespace {
		constexpr Amount Default_Rental_Fee(543);

		template<typename TTraits>
		auto CreateTransactionWithProperties(uint8_t numProperties) {
			using TransactionType = typename TTraits::TransactionType;
			uint32_t entitySize = sizeof(TransactionType) + numProperties * sizeof(MosaicProperty);
			auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
			pTransaction->Version = Transaction_Version;
			pTransaction->Size = entitySize;
			pTransaction->PropertiesHeader.Count = numProperties;
			test::FillWithRandomData(pTransaction->Signer);
			return pTransaction;
		}

		template<typename TTransaction>
		void FillDefaultTransactionData(TTransaction& transaction) {
			transaction.MosaicNonce = MosaicNonce(12345);
			transaction.MosaicId = MosaicId(123);
			transaction.PropertiesHeader.Flags = static_cast<MosaicFlags>(2);
			transaction.PropertiesHeader.Divisibility = 7;
		}

		template<typename TTraits>
		struct MosaicDefinitionTransactionPluginTestContext {
		public:
			using TransactionType = typename TTraits::TransactionType;

		public:
			template<typename TTransactionPlugin>
			void PublishTransaction(const TTransactionPlugin& plugin, const TransactionType& transaction) {
				test::PublishTransaction(plugin, transaction, NonceIdSub);
				test::PublishTransaction(plugin, transaction, PropertiesSub);
				test::PublishTransaction(plugin, transaction, DefinitionSub);
				test::PublishTransaction(plugin, transaction, RentalFeeSub);
			}

			void AssertMosaicNonceNotification(const Key& signer) {
				ASSERT_EQ(1u, NonceIdSub.numMatchingNotifications());
				const auto& notification = NonceIdSub.matchingNotifications()[0];
				EXPECT_EQ(signer, notification.Signer);
				EXPECT_EQ(MosaicNonce(12345), notification.MosaicNonce);
				EXPECT_EQ(MosaicId(123), notification.MosaicId);
			}

			void AssertOptionalMosaicProperties(const MosaicPropertiesHeader& mosaicPropertiesHeader, const MosaicProperty& property) {
				ASSERT_EQ(1u, PropertiesSub.numMatchingNotifications());
				const auto& notification = PropertiesSub.matchingNotifications()[0];
				EXPECT_EQ(&mosaicPropertiesHeader, &notification.PropertiesHeader);
				EXPECT_EQ(&property, notification.PropertiesPtr);
			}

			void AssertNoOptionalMosaicProperties(const MosaicPropertiesHeader& mosaicPropertiesHeader) {
				ASSERT_EQ(1u, PropertiesSub.numMatchingNotifications());
				const auto& notification = PropertiesSub.matchingNotifications()[0];
				EXPECT_EQ(&mosaicPropertiesHeader, &notification.PropertiesHeader);
				EXPECT_FALSE(!!notification.PropertiesPtr);
			}

			void AssertMosaicDefinitionTransaction(const Key& signer, const MosaicProperties& expectedProperties) {
				ASSERT_EQ(1u, DefinitionSub.numMatchingNotifications());
				const auto& notification = DefinitionSub.matchingNotifications()[0];
				EXPECT_EQ(signer, notification.Signer);
				EXPECT_EQ(MosaicId(123), notification.MosaicId);
				EXPECT_EQ(expectedProperties, notification.Properties);
			}

			void AssertMosaicRentalFeeNotification(const Key& signer, const UnresolvedAddress& recipientSink) {
				ASSERT_EQ(1u, RentalFeeSub.numMatchingNotifications());
				const auto& notification = RentalFeeSub.matchingNotifications()[0];
				EXPECT_EQ(signer, notification.Sender);
				EXPECT_EQ(recipientSink, notification.Recipient);
				EXPECT_EQ(Currency_Mosaic_Id, notification.MosaicId);
				EXPECT_EQ(Default_Rental_Fee, notification.Amount);
			}

		public:
			mocks::MockTypedNotificationSubscriber<MosaicNonceNotification<1>> NonceIdSub;
			mocks::MockTypedNotificationSubscriber<MosaicPropertiesNotification<1>> PropertiesSub;
			mocks::MockTypedNotificationSubscriber<MosaicDefinitionNotification<1>> DefinitionSub;
			mocks::MockTypedNotificationSubscriber<MosaicRentalFeeNotification<1>> RentalFeeSub;
		};
	}

	PLUGIN_TEST(CanExtractDefinitionNotificationsWhenOptionalPropertiesArePresent) {
		// Arrange:
		MosaicDefinitionTransactionPluginTestContext<TTraits> testContext;
		auto pluginConfig = CreateMosaicConfiguration(Default_Rental_Fee);
		auto blockChainConfig = CreateBlockChainConfiguration(pluginConfig);
		auto sinkAddress = GetSinkAddress(pluginConfig, blockChainConfig);
		auto pPlugin = TTraits::CreatePlugin(std::make_shared<config::MockLocalNodeConfigurationHolder>(blockChainConfig));

		auto pTransaction = CreateTransactionWithProperties<TTraits>(1);
		FillDefaultTransactionData(*pTransaction);
		pTransaction->PropertiesPtr()[0] = { MosaicPropertyId::Duration, 5 };

		// Act:
		testContext.PublishTransaction(*pPlugin, *pTransaction);

		// Assert:
		const auto& signer = pTransaction->Signer;
		testContext.AssertMosaicNonceNotification(signer);
		testContext.AssertOptionalMosaicProperties(pTransaction->PropertiesHeader, *pTransaction->PropertiesPtr());
		testContext.AssertMosaicDefinitionTransaction(signer, MosaicProperties::FromValues({ { 2, 7, 5 } }));
		testContext.AssertMosaicRentalFeeNotification(signer, sinkAddress);
	}

	PLUGIN_TEST(CanExtractDefinitionNotificationsWhenNoOptionalPropertiesArePresent) {
		// Arrange:
		MosaicDefinitionTransactionPluginTestContext<TTraits> testContext;
		auto pluginConfig = CreateMosaicConfiguration(Default_Rental_Fee);
		auto blockChainConfig = CreateBlockChainConfiguration(pluginConfig);
		auto sinkAddress = GetSinkAddress(pluginConfig, blockChainConfig);
		auto pPlugin = TTraits::CreatePlugin(std::make_shared<config::MockLocalNodeConfigurationHolder>(blockChainConfig));

		auto pTransaction = CreateTransactionWithProperties<TTraits>(0);
		FillDefaultTransactionData(*pTransaction);

		// Act:
		testContext.PublishTransaction(*pPlugin, *pTransaction);

		// Assert:
		const auto& signer = pTransaction->Signer;
		testContext.AssertMosaicNonceNotification(signer);
		testContext.AssertNoOptionalMosaicProperties(pTransaction->PropertiesHeader);
		testContext.AssertMosaicDefinitionTransaction(signer, MosaicProperties::FromValues({ { 2, 7, 0 } }));
		testContext.AssertMosaicRentalFeeNotification(signer, sinkAddress);
	}

	// endregion
}}
