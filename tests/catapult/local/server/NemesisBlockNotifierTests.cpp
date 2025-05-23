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

#include "catapult/local/server/NemesisBlockNotifier.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "tests/test/core/mocks/MockMemoryBlockStorage.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/nemesis/NemesisCompatibleConfiguration.h"
#include "tests/test/nemesis/NemesisTestUtils.h"
#include "tests/test/nodeps/MijinConstants.h"
#include "tests/test/other/mocks/MockBlockChangeSubscriber.h"
#include "tests/test/other/mocks/MockStateChangeSubscriber.h"

namespace catapult { namespace local {

#define TEST_CLASS NemesisBlockNotifierTests

	namespace {
		// region TestContext

		class TestContext {
		public:
			explicit TestContext(uint32_t numBlocks)
					: m_pPluginManager(test::CreatePluginManagerWithRealPlugins(CreateConfiguration()))
					, m_cache(m_pPluginManager->createCache())
					, m_pStorage(mocks::CreateMemoryBlockStorageCache(numBlocks))
					, m_notifier(m_cache, *m_pStorage, *m_pPluginManager)
			{}

		public:
			auto& notifier() {
				return m_notifier;
			}

		public:
			void addRandomAccountToCache() {
				auto cacheDelta = m_cache.createDelta();
				cacheDelta.sub<cache::AccountStateCache>().addAccount(test::GenerateRandomByteArray<Key>(), Height(1));
				m_cache.commit(Height());
			}

		private:
			static config::BlockchainConfiguration CreateConfiguration() {
				return test::CreateBlockchainConfigurationWithNemesisPluginExtensions("");
			}

		private:
			std::shared_ptr<plugins::PluginManager> m_pPluginManager;
			cache::CatapultCache m_cache;
			std::unique_ptr<io::BlockStorageCache> m_pStorage;
			NemesisBlockNotifier m_notifier;
		};

		// endregion
	}

	// region block notifications

	TEST(TEST_CLASS, BlockNotificationsAreNotRaisedWhenHeightIsGreaterThanOne) {
		// Arrange:
		TestContext context(2);
		mocks::MockBlockChangeSubscriber subscriber;

		// Act:
		EXPECT_THROW(context.notifier().raise(subscriber), catapult_runtime_error);

		// Assert:
		const auto& capturedBlockElements = subscriber.copiedBlockElements();
		EXPECT_EQ(0u, capturedBlockElements.size());
	}

	TEST(TEST_CLASS, BlockNotificationsAreNotRaisedWhenHeightIsEqualToOneAndPreviousExecutionIsDetected) {
		// Arrange: add account to indicate previous execution
		TestContext context(1);
		context.addRandomAccountToCache();
		mocks::MockBlockChangeSubscriber subscriber;

		// Act:
		EXPECT_THROW(context.notifier().raise(subscriber), catapult_runtime_error);

		// Assert:
		const auto& capturedBlockElements = subscriber.copiedBlockElements();
		EXPECT_EQ(0u, capturedBlockElements.size());
	}

	TEST(TEST_CLASS, BlockNotificationsAreRaisedWhenHeightIsEqualToOneAndPreviousExecutionIsNotDetected) {
		// Arrange:
		TestContext context(1);
		mocks::MockBlockChangeSubscriber subscriber;

		// Act:
		context.notifier().raise(subscriber);

		// Assert:
		const auto& capturedBlockElements = subscriber.copiedBlockElements();
		ASSERT_EQ(1u, capturedBlockElements.size());
		EXPECT_EQ(Height(1), capturedBlockElements[0]->Block.Height);
	}

	// endregion

	// region state notifications

	namespace {
		model::AddressSet GetAddedAccountAddresses(const cache::CacheChanges& cacheChanges) {
			model::AddressSet addresses;
			auto accountStateCacheChanges = cacheChanges.sub<cache::AccountStateCache>();
			for (const auto* pAccountState : accountStateCacheChanges.addedElements())
				addresses.insert(pAccountState->Address);

			return addresses;
		}

		bool ContainsAddress(const model::AddressSet& addresses, const Address& address) {
			return addresses.cend() != addresses.find(address);
		}

		bool ContainsModifiedPrivate(const model::AddressSet& addresses, const char* privateKeyString) {
			return ContainsAddress(addresses, test::RawPrivateKeyToAddress(privateKeyString));
		}

		bool ContainsModifiedPublic(const model::AddressSet& addresses, const char* publicKeyString) {
			return ContainsAddress(addresses, test::RawPublicKeyToAddress(publicKeyString));
		}
	}

	TEST(TEST_CLASS, StateNotificationsAreNotRaisedWhenHeightIsGreaterThanOne) {
		// Arrange:
		TestContext context(2);
		mocks::MockStateChangeSubscriber subscriber;

		// Act:
		EXPECT_THROW(context.notifier().raise(subscriber), catapult_runtime_error);

		// Assert:
		EXPECT_EQ(0u, subscriber.numScoreChanges());
		EXPECT_EQ(0u, subscriber.numStateChanges());
	}

	TEST(TEST_CLASS, StateNotificationsAreRaisedWhenHeightIsEqualToOne) {
		// Arrange:
		TestContext context(1);
		mocks::MockStateChangeSubscriber subscriber;

		// - register consumer because CatapultCacheDelta wrapped by CacheChanges is temporary and will be out of scope below
		model::AddressSet addedAddresses;
		subscriber.setCacheChangesConsumer([&addedAddresses](const auto& cacheChanges) {
			addedAddresses = GetAddedAccountAddresses(cacheChanges);
		});

		// Act:
		context.notifier().raise(subscriber);

		// Assert:
		ASSERT_EQ(1u, subscriber.numScoreChanges());
		ASSERT_EQ(1u, subscriber.numStateChanges());

		const auto& chainScore = subscriber.lastChainScore();
		EXPECT_EQ(model::ChainScore(1), chainScore);

		const auto& stateChangeInfo = subscriber.lastStateChangeInfo();
		EXPECT_EQ(model::ChainScore(1), stateChangeInfo.ScoreDelta);
		EXPECT_EQ(Height(1), stateChangeInfo.Height);

		// - check account state changes
		EXPECT_EQ(3u + CountOf(test::Mijin_Test_Private_Keys), addedAddresses.size());

		// - check nemesis and rental fee sinks
		EXPECT_TRUE(ContainsModifiedPrivate(addedAddresses, test::Mijin_Test_Nemesis_Private_Key));
		EXPECT_TRUE(ContainsModifiedPublic(addedAddresses, test::Namespace_Rental_Fee_Sink_Public_Key));
		EXPECT_TRUE(ContainsModifiedPublic(addedAddresses, test::Mosaic_Rental_Fee_Sink_Public_Key));

		// - check recipient accounts
		for (const auto* pRecipientPrivateKeyString : test::Mijin_Test_Private_Keys)
			EXPECT_TRUE(ContainsModifiedPrivate(addedAddresses, pRecipientPrivateKeyString)) << pRecipientPrivateKeyString;
	}

	// endregion
}}
