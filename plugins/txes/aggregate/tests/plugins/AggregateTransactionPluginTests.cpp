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

#include "src/config/AggregateConfiguration.h"
#include "src/plugins/AggregateTransactionPlugin.h"
#include "src/model/AggregateNotifications.h"
#include "src/model/AggregateTransaction.h"
#include "catapult/model/Address.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/AggregateTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS AggregateTransactionPluginTests

	// region AggregateTransactionWrapper

	namespace {
		constexpr auto Entity_Type = static_cast<EntityType>(9876);

		template<typename TDescriptor>
		struct AggregateTransactionWrapper {
			model::UniqueEntityPtr<AggregateTransaction<TDescriptor>> pTransaction;
			std::vector<const mocks::EmbeddedMockTransaction*> SubTransactions;
			std::vector<Key> SubTransactionSigners;
			std::vector<Key> SubTransactionRecipients;
			std::vector<Key> Cosigners;
			std::vector<RawSignature> CosignerSignatures;
		};

		template<typename TDescriptor>
		AggregateTransactionWrapper<TDescriptor> CreateAggregateTransaction(uint8_t numTransactions, uint8_t numCosignatures, VersionType version) {
			using TransactionType = AggregateTransaction<TDescriptor>;
			uint32_t entitySize = sizeof(TransactionType)
					+ numTransactions * sizeof(mocks::EmbeddedMockTransaction)
					+ numCosignatures * sizeof(typename TDescriptor::CosignatureType);

			AggregateTransactionWrapper<TDescriptor> wrapper;
			auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
			pTransaction->Version = MakeVersion(NetworkIdentifier::Mijin_Test, version);
			pTransaction->Size = entitySize;
			pTransaction->PayloadSize = numTransactions * sizeof(mocks::EmbeddedMockTransaction);
			test::FillWithRandomData(pTransaction->Signer);
			test::FillWithRandomData(pTransaction->Signature);

			auto* pSubTransaction = static_cast<mocks::EmbeddedMockTransaction*>(pTransaction->TransactionsPtr());
			for (uint8_t i = 0; i < numTransactions; ++i) {
				pSubTransaction->Size = sizeof(mocks::EmbeddedMockTransaction);
				pSubTransaction->Data.Size = 0;
				pSubTransaction->Type = mocks::EmbeddedMockTransaction::Entity_Type;
				pSubTransaction->Version = model::MakeVersion(static_cast<model::NetworkIdentifier>(100 + i), (i + 1) * 2);
				test::FillWithRandomData(pSubTransaction->Signer);
				test::FillWithRandomData(pSubTransaction->Recipient);

				wrapper.SubTransactions.push_back(pSubTransaction);
				wrapper.SubTransactionSigners.push_back(pSubTransaction->Signer);
				wrapper.SubTransactionRecipients.push_back(pSubTransaction->Recipient);
				++pSubTransaction;
			}

			auto* pCosignature = pTransaction->CosignaturesPtr();
			for (auto i = 0u; i < numCosignatures; ++i) {
				test::FillWithRandomData(pCosignature->Signer);
				test::FillWithRandomData(pCosignature->Signature);

				wrapper.Cosigners.push_back(pCosignature->Signer);
				wrapper.CosignerSignatures.push_back(pCosignature->GetRawSignature());
				++pCosignature;
			}

			wrapper.pTransaction = std::move(pTransaction);
			return wrapper;
		}

		auto CreateConfigHolder(const utils::TimeSpan& time) {
			auto blockchainConfig = model::NetworkConfiguration::Uninitialized();
			auto aggregateConfig = config::AggregateConfiguration::Uninitialized();
			aggregateConfig.MaxBondedTransactionLifetime = time;
			blockchainConfig.SetPluginConfiguration(aggregateConfig);
			return config::CreateMockConfigurationHolder(blockchainConfig);
		}


		std::unique_ptr<model::TransactionPlugin> CreateTransactionPluginV1(const model::TransactionRegistry& registry, utils::TimeSpan time = utils::TimeSpan{}) {
			return CreateAggregateTransactionV1Plugin(registry, Entity_Type, CreateConfigHolder(time));
		}
		std::unique_ptr<model::TransactionPlugin> CreateTransactionPluginV2(const model::TransactionRegistry& registry, utils::TimeSpan time = utils::TimeSpan{}) {
			return CreateAggregateTransactionV2Plugin(registry, Entity_Type, CreateConfigHolder(time));
		}
	}

	// endregion

	// region basic

	TEST(TEST_CLASS, CanCreatePluginV1) {
		// Act:
		TransactionRegistry registry;
		auto pPlugin = CreateTransactionPluginV1(registry);

		// Assert:
		EXPECT_EQ(Entity_Type, pPlugin->type());
	}

	TEST(TEST_CLASS, PluginV1SupportsTopLevel) {
		// Arrange:
		TransactionRegistry registry;
		auto pPlugin = CreateTransactionPluginV1(registry);

		// Act + Assert:
		EXPECT_TRUE(pPlugin->supportsTopLevel());
	}

	TEST(TEST_CLASS, PluginV1DoesNotSupportEmbedding) {
		// Arrange:
		TransactionRegistry registry;
		auto pPlugin = CreateTransactionPluginV1(registry);

		// Act + Assert:
		EXPECT_FALSE(pPlugin->supportsEmbedding());
		EXPECT_THROW(pPlugin->embeddedPlugin(), catapult_runtime_error);
	}

	TEST(TEST_CLASS, CanCreatePluginV2) {
		// Act:
		TransactionRegistry registry;
		auto pPlugin = CreateTransactionPluginV1(registry);

		// Assert:
		EXPECT_EQ(Entity_Type, pPlugin->type());
	}

	TEST(TEST_CLASS, PluginV2SupportsTopLevel) {
		// Arrange:
		TransactionRegistry registry;
		auto pPlugin = CreateTransactionPluginV1(registry);

		// Act + Assert:
		EXPECT_TRUE(pPlugin->supportsTopLevel());
	}

	TEST(TEST_CLASS, PluginV2DoesNotSupportEmbedding) {
		// Arrange:
		TransactionRegistry registry;
		auto pPlugin = CreateTransactionPluginV1(registry);

		// Act + Assert:
		EXPECT_FALSE(pPlugin->supportsEmbedding());
		EXPECT_THROW(pPlugin->embeddedPlugin(), catapult_runtime_error);
	}

	// endregion

	// region attributes

	TEST(TEST_CLASS, V1AttributesReturnsCorrectValuesWhenConfiguredWithDefaultMaxLifetime) {
		// Arrange:
		TransactionRegistry registry;
		auto pPlugin = CreateTransactionPluginV1(registry);

		// Act:
		auto attributes = pPlugin->attributes(Height{0});

		// Assert:
		EXPECT_EQ(2u, attributes.MinVersion);
		EXPECT_EQ(3u, attributes.MaxVersion);

		// - zero denotes default lifetime should be used
		EXPECT_EQ(utils::TimeSpan(), attributes.MaxLifetime);
	}

	TEST(TEST_CLASS, V1AttributesReturnsCorrectValuesWhenConfiguredWithCustomMaxLifetime) {
		// Arrange:
		TransactionRegistry registry;
		auto pPlugin = CreateTransactionPluginV1(registry, utils::TimeSpan::FromMinutes(1234));

		// Act:
		auto attributes = pPlugin->attributes(Height{0});

		// Assert:
		EXPECT_EQ(2u, attributes.MinVersion);
		EXPECT_EQ(3u, attributes.MaxVersion);

		EXPECT_EQ(utils::TimeSpan::FromMinutes(1234), attributes.MaxLifetime);
	}

	TEST(TEST_CLASS, V2AttributesReturnsCorrectValuesWhenConfiguredWithDefaultMaxLifetime) {
		// Arrange:
		TransactionRegistry registry;
		auto pPlugin = CreateTransactionPluginV1(registry);

		// Act:
		auto attributes = pPlugin->attributes(Height{0});

		// Assert:
		EXPECT_EQ(2u, attributes.MinVersion);
		EXPECT_EQ(3u, attributes.MaxVersion);

		// - zero denotes default lifetime should be used
		EXPECT_EQ(utils::TimeSpan(), attributes.MaxLifetime);
	}

	TEST(TEST_CLASS, V2AttributesReturnsCorrectValuesWhenConfiguredWithCustomMaxLifetime) {
		// Arrange:
		TransactionRegistry registry;
		auto pPlugin = CreateTransactionPluginV1(registry, utils::TimeSpan::FromMinutes(1234));

		// Act:
		auto attributes = pPlugin->attributes(Height{0});

		// Assert:
		EXPECT_EQ(2u, attributes.MinVersion);
		EXPECT_EQ(3u, attributes.MaxVersion);

		EXPECT_EQ(utils::TimeSpan::FromMinutes(1234), attributes.MaxLifetime);
	}

	// endregion
namespace {
	struct V1PluginFactory
	{
		using CosignaturesNotification = AggregateCosignaturesNotification<1>;
		using EmbeddedNotification = AggregateEmbeddedTransactionNotification<1>;
		using SignatureNotificationType = SignatureNotification<1>;
		static std::unique_ptr<model::TransactionPlugin> Create(const model::TransactionRegistry& registry, utils::TimeSpan time = utils::TimeSpan{})
		{
			return CreateTransactionPluginV1(registry, time);
		}
	};

	struct V2PluginFactory
	{
		using CosignaturesNotification = AggregateCosignaturesNotification<3>;
		using EmbeddedNotification = AggregateEmbeddedTransactionNotification<2>;
		using SignatureNotificationType = SignatureNotification<2>;
		static std::unique_ptr<model::TransactionPlugin> Create(const model::TransactionRegistry& registry, utils::TimeSpan time = utils::TimeSpan{})
		{
			return CreateTransactionPluginV2(registry, time);
		}
	};

}
#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TDescriptor, typename TPluginFactory>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version); \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<model::AggregateTransactionRawDescriptor, V1PluginFactory>(2); } \
	TEST(TEST_CLASS, TEST_NAME##_v3) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<model::AggregateTransactionRawDescriptor, V1PluginFactory>(3); } \
    TEST(TEST_CLASS, TEST_NAME##_v1_2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<model::AggregateTransactionExtendedDescriptor, V2PluginFactory>(1); } \
    template<typename TDescriptor, typename TPluginFactory>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version)

	// region size

	TRAITS_BASED_TEST(CanCalculateSizeWhenAllSubTransactionsAreSupported) {
		// Arrange:
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(3, 4, version);

		// Act:
		auto realSize = pPlugin->calculateRealSize(*wrapper.pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(AggregateTransaction<TDescriptor>) + 3 * sizeof(mocks::EmbeddedMockTransaction) + 4 * sizeof(typename TDescriptor::CosignatureType), realSize);
	}

	TRAITS_BASED_TEST(CannotCalculateSizeWhenAnySubTransactionIsNotSupported) {
		// Arrange:
		TransactionRegistry registry;
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(3, 4, version);

		// Act:
		auto realSize = pPlugin->calculateRealSize(*wrapper.pTransaction);

		// Assert:
		EXPECT_EQ(std::numeric_limits<uint64_t>::max(), realSize);
	}

	// endregion

	// region publish - basic

	TRAITS_BASED_TEST(CanRaiseCorrectNumberOfNotificationsFromEmptyAggregate) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(0, 0, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, model::WeakEntityInfoT<model::Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);




		auto i = 0u;
		if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionRawDescriptor>) {
			// Assert for V1:
			// - 1 AggregateTransactionHashNotification (version 3 and higher)
			// - 1 AggregateCosignaturesNotification<2> (version 3 and higher)
			// - 1 AggregateTransactionTypeNotification
			// - 1 AggregateCosignaturesNotification<1>

			ASSERT_EQ(2u + (version > 2u ? 2 : 0), sub.numNotifications());

			if (version > 2u) {
				EXPECT_EQ(Aggregate_Hash_v1_Notification, sub.notificationTypes()[i++]);
				EXPECT_EQ(Aggregate_Cosignatures_v2_Notification, sub.notificationTypes()[i++]);
			}
			// - aggregate transaction type notification must be the first raised notification
			EXPECT_EQ(Aggregate_Type_v1_Notification, sub.notificationTypes()[i++]);
			// - aggregate cosignatures notification must be the second raised notification
			EXPECT_EQ(Aggregate_Cosignatures_v1_Notification, sub.notificationTypes()[i++]);
		}
		else {
			ASSERT_EQ(3u, sub.numNotifications());
			// Assert V2:
			// - 1 AggregateTransactionHashNotification (version 3 and higher)
			// - 1 AggregateCosignaturesNotification<2> (version 3 and higher)
			// - 1 AggregateTransactionTypeNotification
			// - 1 AggregateCosignaturesNotification<1>
			EXPECT_EQ(Aggregate_Type_v1_Notification, sub.notificationTypes()[i++]);
			EXPECT_EQ(Aggregate_Hash_v1_Notification, sub.notificationTypes()[i++]);
			EXPECT_EQ(Aggregate_Cosignatures_v3_Notification, sub.notificationTypes()[i++]);
		}

	}

	TRAITS_BASED_TEST(CanRaiseCorrectNumberOfNotificationsFromAggregate) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(2, 3, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, model::WeakEntityInfoT<model::Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

		if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionRawDescriptor>) {
			// Assert:
			// - 1 AggregateTransactionHashNotification (version 3 and higher)
			// - 1 AggregateCosignaturesNotification<2> (version 3 and higher)
			// - 1 AggregateTransactionTypeNotification
			// - 1 AggregateCosignaturesNotification<1>
			// - 3 SignatureNotification (one per cosigner)
			// - 2 SourceChangeNotification (one per embedded-mock)
			// - 4 AccountPublicKeyNotification (two per embedded-mock; one signer and one recipient each)
			// - 2 EntityNotification (one per embedded-mock)
			// - 2 AggregateEmbeddedTransactionNotification (one per embedded-mock)

			ASSERT_EQ(1u + 1 + 3 + 2 + 4 + 2 + 2 + (version > 2u ? 2 : 0), sub.numNotifications());

			auto i = 0u;
			if (version > 2u) {
				EXPECT_EQ(Aggregate_Hash_v1_Notification, sub.notificationTypes()[i++]);
				EXPECT_EQ(Aggregate_Cosignatures_v2_Notification, sub.notificationTypes()[i++]);
			}
			// - aggregate transaction type notification must be the first raised notification
			EXPECT_EQ(Aggregate_Type_v1_Notification, sub.notificationTypes()[i++]);
			// - aggregate cosignatures notification must be the second raised notification
			EXPECT_EQ(Aggregate_Cosignatures_v1_Notification, sub.notificationTypes()[i++]);

			// - source change notification must be the first raised sub-transaction notification
			for (auto k = 0u; k < 2u; ++k) {
				auto message = "sub-transaction at " + std::to_string(k);
				EXPECT_EQ(Core_Source_Change_v1_Notification, sub.notificationTypes()[i++]) << message;
				EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]) << message;
				EXPECT_EQ(Core_Entity_v1_Notification, sub.notificationTypes()[i++]) << message;
				EXPECT_EQ(Aggregate_EmbeddedTransaction_v1_Notification, sub.notificationTypes()[i++]) << message;
				EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]) << message;
			}

			// - signature notifications are raised last (and with wrong source) for performance reasons
			EXPECT_TRUE(Core_Signature_v1_Notification == sub.notificationTypes()[i++]);
			EXPECT_TRUE(Core_Signature_v1_Notification == sub.notificationTypes()[i++]);
			EXPECT_TRUE(Core_Signature_v1_Notification == sub.notificationTypes()[i++]);
		}
		else{
			// Assert:
			// - 1 AggregateTransactionTypeNotification
			// - 1 AggregateTransactionHashNotification
			// - 1 AggregateCosignaturesNotification<3>
			// - 3 SignatureNotification (one per cosigner)
			// - 2 SourceChangeNotification (one per embedded-mock)
			// - 4 AccountPublicKeyNotification (two per embedded-mock; one signer and one recipient each)
			// - 2 EntityNotification (one per embedded-mock)
			// - 2 AggregateEmbeddedTransactionNotification<2> (one per embedded-mock)

			ASSERT_EQ(1u + 1 + 1 + 3 + 2 + 4 + 2 + 2, sub.numNotifications());

			auto i = 0u;
			// - aggregate transaction type notification must be the first raised notification
			EXPECT_EQ(Aggregate_Type_v1_Notification, sub.notificationTypes()[i++]);

			EXPECT_EQ(Aggregate_Hash_v1_Notification, sub.notificationTypes()[i++]);
			// - aggregate cosignatures notification must be the second raised notification
			EXPECT_EQ(Aggregate_Cosignatures_v3_Notification, sub.notificationTypes()[i++]);

			// - source change notification must be the first raised sub-transaction notification
			for (auto k = 0u; k < 2u; ++k) {
				auto message = "sub-transaction at " + std::to_string(k);
				EXPECT_EQ(Core_Source_Change_v1_Notification, sub.notificationTypes()[i++]) << message;
				EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]) << message;
				EXPECT_EQ(Core_Entity_v1_Notification, sub.notificationTypes()[i++]) << message;
				EXPECT_EQ(Aggregate_EmbeddedTransaction_v2_Notification, sub.notificationTypes()[i++]) << message;
				EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]) << message;
			}

			// - signature notifications are raised last (and with wrong source) for performance reasons
			EXPECT_TRUE(Core_Signature_v2_Notification == sub.notificationTypes()[i++]);
			EXPECT_TRUE(Core_Signature_v2_Notification == sub.notificationTypes()[i++]);
			EXPECT_TRUE(Core_Signature_v2_Notification == sub.notificationTypes()[i++]);
		}

	}

	// endregion

	// region publish - account (including nested)

	namespace {
		template<typename TDescriptor>
		void AssertContainsAllKeys(const mocks::MockNotificationSubscriber& sub, const AggregateTransactionWrapper<TDescriptor>& wrapper) {
			auto i = 0u;
			for (const auto& key : wrapper.SubTransactionSigners) {
				EXPECT_TRUE(sub.contains(key)) << "sub transaction signer " << i;
				++i;
			}

			for (const auto& key : wrapper.SubTransactionRecipients) {
				EXPECT_TRUE(sub.contains(key)) << "sub transaction recipient " << i;
				++i;
			}

			// Sanity:
			EXPECT_EQ(i, sub.numKeys());
		}
	}

	TRAITS_BASED_TEST(CanRaiseAccountNotificationsFromAggregate) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(2, 3, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, model::WeakEntityInfoT<model::Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

		// Assert: 2 sub signer and 2 sub recipient notifications are raised
		EXPECT_EQ(0u, sub.numAddresses());
		EXPECT_EQ(4u, sub.numKeys());
		AssertContainsAllKeys(sub, wrapper);
	}

	// endregion

	// region publish - source change notification

	TRAITS_BASED_TEST(CanRaiseSourceChangeNotificationsFromAggregate) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<SourceChangeNotification<1>> sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(2, 3, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, model::WeakEntityInfoT<model::Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

		// Assert: one notification is raised for each embedded transaction
		ASSERT_EQ(2u, sub.numMatchingNotifications());
		for (auto i = 0u; i < 2; ++i) {
			auto message = "transaction at " + std::to_string(i);
			const auto& notification = sub.matchingNotifications()[i];

			EXPECT_EQ(0u, notification.PrimaryId) << message;
			EXPECT_EQ(1u, notification.SecondaryId) << message;
			EXPECT_EQ(SourceChangeNotification<1>::SourceChangeType::Relative, notification.PrimaryChangeType) << message;
			EXPECT_EQ(SourceChangeNotification<1>::SourceChangeType::Relative, notification.SecondaryChangeType) << message;
		}
	}

	// endregion

	// region publish - entity notification

	TRAITS_BASED_TEST(CanRaiseEntityNotificationsFromAggregate) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<EntityNotification<1>> sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(2, 3, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, model::WeakEntityInfoT<model::Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

		// Assert: one notification is raised for each embedded transaction
		ASSERT_EQ(2u, sub.numMatchingNotifications());
		for (auto i = 0u; i < 2; ++i) {
			auto message = "transaction at " + std::to_string(i);
			const auto& notification = sub.matchingNotifications()[i];

			// - min/max version comes from MockTransactionPlugin created in CreateDefaultTransactionRegistry
			EXPECT_EQ(static_cast<NetworkIdentifier>(100 + i), notification.NetworkIdentifier) << message;
			EXPECT_EQ(mocks::EmbeddedMockTransaction::Entity_Type, notification.EntityType) << message;
			EXPECT_EQ((i + 1) * 2, notification.EntityVersion) << message;
		}
	}

	// endregion

	// region publish - aggregate embedded transaction

	namespace {
		template<typename TDescriptor, typename TPluginFactory>
		void AssertCanRaiseEmbeddedTransactionNotifications(uint8_t numTransactions, uint8_t numCosignatures, VersionType version) {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<typename TPluginFactory::EmbeddedNotification> sub;
			auto registry = mocks::CreateDefaultTransactionRegistry();
			auto pPlugin = TPluginFactory::Create(registry);
			auto wrapper = CreateAggregateTransaction<TDescriptor>(numTransactions, numCosignatures, version);
			auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

			// Act:
			test::PublishTransaction(*pPlugin, model::WeakEntityInfoT<model::Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

			// Assert: the plugin raises an embedded transaction notification for each transaction
			ASSERT_EQ(numTransactions, sub.numMatchingNotifications());
			for (auto i = 0u; i < numTransactions; ++i) {
				auto message = "transaction at " + std::to_string(i);
				const auto& notification = sub.matchingNotifications()[i];

				EXPECT_EQ(wrapper.pTransaction->Signer, notification.Signer) << message;
				EXPECT_EQ(*wrapper.SubTransactions[i], notification.Transaction) << message;
				EXPECT_EQ(numCosignatures, notification.CosignaturesCount) << message;
				test::CompareCosignatures(wrapper.pTransaction->CosignaturesPtr(), notification.CosignaturesPtr, numCosignatures);
			}
		}
	}

	TRAITS_BASED_TEST(EmptyAggregateDoesNotRaiseEmbeddedTransactionNotifications) {
		// Assert:
		AssertCanRaiseEmbeddedTransactionNotifications<TDescriptor, TPluginFactory>(0, 0, version);
	}

	TRAITS_BASED_TEST(CanRaiseEmbeddedTransactionNotificationsFromAggregate) {
		// Assert:
		AssertCanRaiseEmbeddedTransactionNotifications<TDescriptor, TPluginFactory>(2, 3, version);
	}

	// endregion

	// region publish - signature

	namespace {
		template<typename TDescriptor, typename TPluginFactory>
		void AssertCanRaiseSignatureNotifications(uint8_t numTransactions, uint8_t numCosignatures, VersionType version) {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<typename TPluginFactory::SignatureNotificationType> sub; //In updated node version subtransactions always issue a V2 notification
			auto registry = mocks::CreateDefaultTransactionRegistry();
			auto pPlugin = TPluginFactory::Create(registry);
			auto wrapper = CreateAggregateTransaction<TDescriptor>(numTransactions, numCosignatures, version);

			auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();
			// Act:
			test::PublishTransaction(*pPlugin, WeakEntityInfoT<Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

			// Assert: the plugin only raises signature notifications for explicit cosigners
			//         (the signature notification for the signer is raised as part of normal processing)
			ASSERT_EQ(numCosignatures, sub.numMatchingNotifications());
			for (auto i = 0u; i < numCosignatures; ++i) {
				auto message = "cosigner at " + std::to_string(i);
				const auto& notification = sub.matchingNotifications()[i];

				// - notifications should refer to cosigners
				EXPECT_EQ(wrapper.Cosigners[i], notification.Signer) << message;
				EXPECT_EQ(wrapper.CosignerSignatures[i], notification.Signature) << message;

				// - notifications should refer to same (aggregate) data hash
				EXPECT_EQ(aggregateDataHash.data(), notification.Data.pData) << message;
				EXPECT_EQ(Hash256_Size, notification.Data.Size) << message;

				// - notifications should not have replay protection because they represent cosignatures
				EXPECT_EQ(Replay::ReplayProtectionMode::Disabled, notification.DataReplayProtectionMode);
			}
		}
	}

	TRAITS_BASED_TEST(EmptyAggregateDoesNotRaiseSignatureNotifications) {
		// Assert:
		AssertCanRaiseSignatureNotifications<TDescriptor, TPluginFactory>(0, 0, version);
	}

	TRAITS_BASED_TEST(CanRaiseSignatureNotificationsFromAggregate) {
		// Assert:
		AssertCanRaiseSignatureNotifications<TDescriptor, TPluginFactory>(2, 3, version);
	}

	// endregion

	// region publish - aggregate cosignatures v1

	TRAITS_BASED_TEST(CanRaiseAggregateCosignaturesNotificationsFromEmptyAggregate) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<typename TPluginFactory::CosignaturesNotification> sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(0, 0, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, WeakEntityInfoT<Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(wrapper.pTransaction->Signer, notification.Signer);
		EXPECT_EQ(0u, notification.TransactionsCount);
		EXPECT_FALSE(!!notification.TransactionsPtr);
		EXPECT_EQ(0u, notification.CosignaturesCount);
		EXPECT_FALSE(!!notification.CosignaturesPtr);
	}

	TRAITS_BASED_TEST(CanRaiseAggregateCosignaturesNotificationsFromAggregate) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<typename TPluginFactory::CosignaturesNotification> sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(2, 3, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, WeakEntityInfoT<Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(wrapper.pTransaction->Signer, notification.Signer);
		EXPECT_EQ(2u, notification.TransactionsCount);
		EXPECT_EQ(wrapper.pTransaction->TransactionsPtr(), notification.TransactionsPtr);
		EXPECT_EQ(3u, notification.CosignaturesCount);
		EXPECT_EQ(wrapper.pTransaction->CosignaturesPtr(), notification.CosignaturesPtr.template Get<typename TDescriptor::CosignatureType>());
	}

	// endregion

	// region dataBuffer

	namespace {
		template<typename TDescriptor, typename TPluginFactory>
		void AssertCanExtractDataBufferFromAggregate(uint8_t numTransactions, uint8_t numCosignatures, VersionType version) {
			// Arrange:
			auto registry = mocks::CreateDefaultTransactionRegistry();
			auto pPlugin = TPluginFactory::Create(registry);
			auto wrapper = CreateAggregateTransaction<TDescriptor>(numTransactions, numCosignatures, version);

			const auto* pAggregateDataStart = test::AsVoidPointer(&wrapper.pTransaction->Version);
			auto aggregateDataSize = sizeof(AggregateTransaction<TDescriptor>) - VerifiableEntity::Header_Size;
			aggregateDataSize += numTransactions * sizeof(mocks::EmbeddedMockTransaction);

			// Act:
			auto buffer = pPlugin->dataBuffer(*wrapper.pTransaction);

			// Assert:
			EXPECT_EQ(pAggregateDataStart, buffer.pData);
			ASSERT_EQ(aggregateDataSize, buffer.Size);
		}
	}

	TRAITS_BASED_TEST(CanExtractDataBufferFromEmptyAggregate) {
		// Assert:
		AssertCanExtractDataBufferFromAggregate<TDescriptor, TPluginFactory>(0, 0, version);
	}

	TRAITS_BASED_TEST(CanExtractDataBufferFromNonEmptyAggregate) {
		// Assert:
		AssertCanExtractDataBufferFromAggregate<TDescriptor, TPluginFactory>(2, 3, version);
	}

	// endregion

	// region merkleSupplementaryBuffers

	namespace {
		template<typename TDescriptor, typename TPluginFactory>
		void AssertCanExtractMerkleSupplementaryBuffersFromAggregate(uint8_t numTransactions, uint8_t numCosignatures, VersionType version) {
			// Arrange:
			auto registry = mocks::CreateDefaultTransactionRegistry();
			auto pPlugin = TPluginFactory::Create(registry);
			auto wrapper = CreateAggregateTransaction<TDescriptor>(numTransactions, numCosignatures, version);

			// Act:
			auto buffers = pPlugin->merkleSupplementaryBuffers(*wrapper.pTransaction);

			// Assert:
			ASSERT_EQ(numCosignatures, buffers.size());

			const auto* pCosignature = wrapper.pTransaction->CosignaturesPtr();
			for (auto i = 0u; i < numCosignatures; ++i) {
				auto message = "buffer " + std::to_string(i);
				EXPECT_EQ(pCosignature->Signer.data(), buffers[i].pData) << message;
				EXPECT_EQ(Key_Size, buffers[i].Size) << message;
				++pCosignature;
			}
		}
	}

	TRAITS_BASED_TEST(CanExtractMerkleSupplementaryBuffersFromEmptyAggregate) {
		// Assert:
		AssertCanExtractMerkleSupplementaryBuffersFromAggregate<TDescriptor, TPluginFactory>(0, 0, version);
	}

	TRAITS_BASED_TEST(CanExtractMerkleSupplementaryBuffersFromNonEmptyAggregate) {
		// Assert:
		AssertCanExtractMerkleSupplementaryBuffersFromAggregate<TDescriptor, TPluginFactory>(2, 3, version);
	}

	// region publish - aggregate transaction hash

	TRAITS_BASED_TEST(CanRaiseAggregateTransactionHashNotificationsFromEmptyAggregate) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AggregateTransactionHashNotification<1>> sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(0, 0, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, WeakEntityInfoT<Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

		// Assert:
		if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionRawDescriptor>) {
			if (version > 2u) {
				ASSERT_EQ(1u, sub.numMatchingNotifications());
				const auto &notification = sub.matchingNotifications()[0];
				EXPECT_EQ(aggregateDataHash, notification.AggregateHash);
				EXPECT_EQ(0u, notification.TransactionsCount);
				EXPECT_FALSE(!!notification.TransactionsPtr);
			} else {
				ASSERT_EQ(0u, sub.numMatchingNotifications());
			}
		}
		else
		{
			ASSERT_EQ(1u, sub.numMatchingNotifications());
			const auto &notification = sub.matchingNotifications()[0];
			EXPECT_EQ(aggregateDataHash, notification.AggregateHash);
			EXPECT_EQ(0u, notification.TransactionsCount);
			EXPECT_FALSE(!!notification.TransactionsPtr);
		}

	}

	TRAITS_BASED_TEST(CanRaiseAggregateTransactionHashNotificationsFromAggregate) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AggregateTransactionHashNotification<1>> sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = TPluginFactory::Create(registry);
		auto wrapper = CreateAggregateTransaction<TDescriptor>(2, 3, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, WeakEntityInfoT<Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

		// Assert:
		if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionRawDescriptor>) {
			if (version > 2u) {
				ASSERT_EQ(1u, sub.numMatchingNotifications());
				const auto &notification = sub.matchingNotifications()[0];
				EXPECT_EQ(aggregateDataHash, notification.AggregateHash);
				EXPECT_EQ(2u, notification.TransactionsCount);
				EXPECT_EQ(wrapper.pTransaction->TransactionsPtr(), notification.TransactionsPtr);
			} else {
				ASSERT_EQ(0u, sub.numMatchingNotifications());
			}
		}
		else
		{
			ASSERT_EQ(1u, sub.numMatchingNotifications());
			const auto &notification = sub.matchingNotifications()[0];
			EXPECT_EQ(aggregateDataHash, notification.AggregateHash);
			EXPECT_EQ(2u, notification.TransactionsCount);
			EXPECT_EQ(wrapper.pTransaction->TransactionsPtr(), notification.TransactionsPtr);
		}
	}

	// endregion

	// region publish - aggregate cosignatures v2

	TRAITS_BASED_TEST(CanRaiseV2AggregateCosignaturesNotificationsFromEmptyAggregate) {
		// Arrange:
		if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionExtendedDescriptor>)
			return;
		mocks::MockTypedNotificationSubscriber<AggregateCosignaturesNotification<2>> sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = CreateTransactionPluginV1(registry);
		auto wrapper = CreateAggregateTransaction<model::AggregateTransactionRawDescriptor>(0, 0, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, WeakEntityInfoT<Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

		// Assert:
		if (version > 2u) {
			ASSERT_EQ(1u, sub.numMatchingNotifications());
			const auto& notification = sub.matchingNotifications()[0];
			EXPECT_EQ(wrapper.pTransaction->Signer, notification.Signer);
			EXPECT_EQ(0u, notification.TransactionsCount);
			EXPECT_FALSE(!!notification.TransactionsPtr);
			EXPECT_EQ(0u, notification.CosignaturesCount);
			EXPECT_FALSE(!!notification.CosignaturesPtr);
		} else {
			ASSERT_EQ(0u, sub.numMatchingNotifications());
		}
	}

	TRAITS_BASED_TEST(CanRaiseV2AggregateCosignaturesNotificationsFromAggregate) {
		// Arrange:
		if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionExtendedDescriptor>)
			return;
		mocks::MockTypedNotificationSubscriber<AggregateCosignaturesNotification<2>> sub;
		auto registry = mocks::CreateDefaultTransactionRegistry();
		auto pPlugin = CreateTransactionPluginV1(registry);
		auto wrapper = CreateAggregateTransaction<model::AggregateTransactionRawDescriptor>(2, 3, version);
		auto aggregateDataHash = test::GenerateRandomByteArray<Hash256>();

		// Act:
		test::PublishTransaction(*pPlugin, WeakEntityInfoT<Transaction>(*wrapper.pTransaction, aggregateDataHash, Height{0}), sub);

		// Assert:
		if (version > 2u) {
			ASSERT_EQ(1u, sub.numMatchingNotifications());
			const auto& notification = sub.matchingNotifications()[0];
			EXPECT_EQ(wrapper.pTransaction->Signer, notification.Signer);
			EXPECT_EQ(2u, notification.TransactionsCount);
			EXPECT_EQ(wrapper.pTransaction->TransactionsPtr(), notification.TransactionsPtr);
			EXPECT_EQ(3u, notification.CosignaturesCount);
			EXPECT_EQ(wrapper.pTransaction->CosignaturesPtr(), notification.CosignaturesPtr.template Get<model::AggregateTransactionRawDescriptor::CosignatureType>());
		} else {
			ASSERT_EQ(0u, sub.numMatchingNotifications());
		}
	}
#undef TRAITS_BASED_TEST
	// endregion
}}
