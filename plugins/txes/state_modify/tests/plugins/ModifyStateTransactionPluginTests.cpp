/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/EntityHasher.h"
#include "src/model/ModifyStateTransaction.h"
#include "src/model/ModifyStateNotifications.h"
#include "src/config/ModifyStateConfiguration.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "src/plugins/ModifyStateTransactionPlugin.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS ModifyStateTransactionPluginTests

	// region TransactionPlugin

	namespace {

		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(ModifyState, std::shared_ptr<config::BlockchainConfigurationHolder>, 1, 1,)

		static const auto Generation_Hash = test::GenerateRandomByteArray<GenerationHash>();

		static constexpr auto Block_Generation_Target_Time = utils::TimeSpan::FromSeconds(15);
		static constexpr auto CacheName = "testcache";
		static constexpr auto SubCacheId = cache::GeneralSubCache::Main;
		static constexpr auto Key_Size = 10;
		static constexpr auto Data_Size = 15;

		auto CreateConfiguration() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.GenerationHash = Generation_Hash;
			config.Network.BlockGenerationTargetTime = Block_Generation_Target_Time;
			auto pluginConfig = config::ModifyStateConfiguration::Uninitialized();
			pluginConfig.Enabled = true;
			config.Network.SetPluginConfiguration(pluginConfig);
			return config.ToConst();
		}

		/// Creates a transaction.
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> CreateTransaction(model::EntityType type, size_t additionalSize = 0) {
			uint32_t entitySize = sizeof(TTransaction) + additionalSize;
			auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
			pTransaction->Signer = test::GenerateRandomByteArray<Key>();
			pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
			pTransaction->Type = type;
			pTransaction->Size = entitySize;

			return pTransaction;
		}

		/// Creates a start execute transaction.
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> CreateModifyStateTransaction(size_t cacheNameSize, size_t keySize, size_t dataSize) {
			auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_ModifyState,
																cacheNameSize + keySize + dataSize);
			pTransaction->CacheNameSize = cacheNameSize;
			pTransaction->KeySize = keySize;
			pTransaction->ContentSize = dataSize;
			pTransaction->SubCacheId = static_cast<uint8_t>(SubCacheId);
			auto data = test::GenerateRandomVector(cacheNameSize+keySize+dataSize);
			memcpy(pTransaction->CacheNamePtr(), static_cast<const void*>(&data), data.size());
			return pTransaction;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return CreateModifyStateTransaction<typename TTraits::TransactionType>(sizeof(CacheName), Key_Size, Data_Size);
		}

		auto CalculateHash(const ModifyStateTransaction& transaction, const GenerationHash& generationHash, model::NotificationSubscriber&) {
			return model::CalculateHash(transaction, generationHash);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(
		TEST_CLASS,
		,
		,Entity_Type_ModifyState,
		config::CreateMockConfigurationHolder(CreateConfiguration()))

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.CacheNameSize = sizeof(CacheName);
		transaction.KeySize = Key_Size;
		transaction.ContentSize = Data_Size;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + sizeof(CacheName) + Key_Size + Data_Size, realSize);
	}

	// endregion

	// region publish - basic

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = MakeVersion(NetworkIdentifier::Mijin_Test, std::numeric_limits<uint32_t>::max());

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		ASSERT_EQ(0, sub.numNotifications());
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications) {
		// Arrange:
		auto pTransaction = CreateTransaction<TTraits>();
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numNotifications());
		EXPECT_EQ(ModifyState_Modify_State_Notification, sub.notificationTypes()[0]);
	}
	// endregion
}}
