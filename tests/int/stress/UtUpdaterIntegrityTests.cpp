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

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_tx/MemoryUtCache.h"
#include "catapult/chain/UtUpdater.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/local/RealTransactionFactory.h"
#include "tests/test/nodeps/TestConstants.h"
#include <boost/thread.hpp>
#include <plugins/txes/transfer/src/config/TransferConfiguration.h>
#include "plugins/txes/mosaic/src/config/MosaicConfiguration.h"
#include "tests/test/nodeps/MijinConstants.h"
#include "catapult/model/TransactionFeeCalculator.h"

namespace catapult { namespace chain {

#define TEST_CLASS UtUpdaterIntegrityTests

	namespace {
		constexpr auto Default_Time = Timestamp(987);

		uint64_t GetNumIterations() {
			return test::GetStressIterationCount() ? 5'000 : 250;
		}

		std::shared_ptr<plugins::PluginManager> CreatePluginManager() {
			auto config = test::CreatePrototypicalBlockchainConfiguration();
			const_cast<model::NetworkConfiguration&>(config.Network).Plugins.emplace(PLUGIN_NAME(transfer), utils::ConfigurationBag({{ "", {
				{ "maxMessageSize", "0" },
				{ "maxMosaicsSize", "512" },
				} }}));
			
			const_cast<model::NetworkConfiguration&>(config.Network).Plugins.emplace(PLUGIN_NAME(mosaic), utils::ConfigurationBag({ { "", {
                { "maxMosaicsPerAccount", "123" },
                { "maxMosaicDuration", "456d" },
                { "maxMosaicDivisibility", "6" },

                { "mosaicRentalFeeSinkPublicKey", test::Mosaic_Rental_Fee_Sink_Public_Key },
                { "mosaicRentalFee", "500" },
                {"levyEnabled", "true"}
            } } }));
			
			const_cast<model::NetworkConfiguration&>(config.Network).template InitPluginConfiguration<config::TransferConfiguration>();
			const_cast<model::NetworkConfiguration&>(config.Network).template InitPluginConfiguration<config::MosaicConfiguration>();
			return test::CreatePluginManagerWithRealPlugins(config);
		}

		// region UpdaterTestContext

		class UpdaterTestContext {
		public:
			UpdaterTestContext()
					: m_pPluginManager(CreatePluginManager())
					, m_transactionsCache(cache::MemoryCacheOptions(1024, GetNumIterations() * 2),
									  std::make_shared<model::TransactionFeeCalculator>())
					, m_cache(m_pPluginManager->createCache())
					, m_updater(
							m_transactionsCache,
							m_cache,
							extensions::CreateExecutionConfiguration(*m_pPluginManager),
							[]() { return Default_Time; },
							[](const auto&, const auto&, const auto&, auto) {},
							[](const auto&, const auto&) { return false; })
			{}

		public:
			cache::MemoryUtCache& transactionsCache() {
				return m_transactionsCache;
			}

			cache::CatapultCache& cache() {
				return m_cache;
			}

			UtUpdater& updater() {
				return m_updater;
			}

		private:
			std::shared_ptr<plugins::PluginManager> m_pPluginManager;
			cache::MemoryUtCache m_transactionsCache;
			cache::CatapultCache m_cache;
			UtUpdater m_updater;
		};

		// endregion
	}

	NO_STRESS_TEST(TEST_CLASS, UtUpdaterUpdateOverloadsAreThreadSafe) {
		// Arrange:
		UpdaterTestContext context;

		// - seed an account with an initial balance of N
		auto senderKeyPair = test::GenerateKeyPair();
		{
			auto mosaicId = test::Default_Currency_Mosaic_Id;
			auto cacheDelta = context.cache().createDelta();
			auto& accountStateCacheDelta = cacheDelta.sub<cache::AccountStateCache>();
			accountStateCacheDelta.addAccount(senderKeyPair.publicKey(), Height(1));
			accountStateCacheDelta.find(senderKeyPair.publicKey()).get().Balances.credit(mosaicId, Amount(GetNumIterations()));
			context.cache().commit(Height(1));
		}

		// Act:
		// - simulate tx dispatcher processing N elements of 1 tx transfering 1 unit each
		boost::thread_group threads;
		threads.create_thread([&senderKeyPair, &updater = context.updater()] {
			auto recipient = test::GenerateRandomByteArray<Key>();
			for (auto i = 0u; i < GetNumIterations(); ++i) {
				auto pTransaction = test::CreateTransferTransaction(senderKeyPair, recipient, Amount(1));
				pTransaction->MaxFee = Amount(0);
				pTransaction->Deadline = Default_Time + Timestamp(1);
				model::TransactionInfo transactionInfo(std::move(pTransaction), test::GenerateRandomByteArray<Hash256>(), Height());

				std::vector<model::TransactionInfo> transactionInfos;
				transactionInfos.emplace_back(std::move(transactionInfo));
				updater.update(transactionInfos);
			}
		});

		// - simulate block dispatcher processing N block elements with single confirmed tx
		threads.create_thread([&updater = context.updater()] {
			auto hash = test::GenerateRandomByteArray<Hash256>();
			for (auto i = 0u; i < GetNumIterations(); ++i)
				updater.update({ &hash }, {});
		});

		// - wait for all threads
		threads.join_all();

		// Assert: all transactions are in the UT cache
		EXPECT_EQ(GetNumIterations(), context.transactionsCache().view().size());
	}
}}
