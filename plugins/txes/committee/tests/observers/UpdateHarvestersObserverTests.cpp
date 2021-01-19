/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "src/observers/Observers.h"
#include "tests/test/CommitteeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS UpdateHarvestersObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(UpdateHarvesters, nullptr)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::CommitteeCacheFactory>;
		using Notification = model::BlockCosignaturesNotification<1>;

		const Key Block_Signer_Key = test::GenerateRandomByteArray<Key>();
		const Height Initial_Height = Height(100);
		const Height Current_Height = Height(200);
		constexpr MosaicId Harvesting_Mosaic_Id(1234);
		constexpr Amount Min_Harvester_Balance(100);
		constexpr uint32_t Fee_Interest(5);
		constexpr uint32_t Fee_Interest_Denominator(10);
		constexpr double Activity_Delta = 0.00001;
		constexpr double Activity_Committee_Cosigned_Delta = 0.01;
		constexpr double Activity_Committee_Not_Cosigned_Delta = 0.02;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;
			config.Network.MinHarvesterBalance = Min_Harvester_Balance;
			auto pluginConfig = config::CommitteeConfiguration::Uninitialized();
			pluginConfig.MinGreed = test::Min_Greed;
			pluginConfig.InitialActivity = test::Initial_Activity;
			pluginConfig.ActivityDelta = Activity_Delta;
			pluginConfig.ActivityCommitteeCosignedDelta = Activity_Committee_Cosigned_Delta;
			pluginConfig.ActivityCommitteeNotCosignedDelta = Activity_Committee_Not_Cosigned_Delta;
			config.Network.SetPluginConfiguration(pluginConfig);
			return config.ToConst();
		}
	}

	TEST(TEST_CLASS, UpdateHarvesters_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height, CreateConfig());
		auto pAccountCollector = std::make_shared<cache::CommitteeAccountCollector>();
		auto pCommitteeManager = std::make_shared<test::TestWeightedVotingCommitteeManager>(pAccountCollector);

		std::vector<Key> cosigners{
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Key>(),
		};

		auto committee = chain::Committee();
		for (const auto& cosigner : cosigners) {
			committee.Cosigners.insert(cosigner);
		}
		committee.Round = 1;
		pCommitteeManager->setCommittee(committee);

		std::vector<model::Cosignature> cosignatures{
			{ cosigners[1], test::GenerateRandomByteArray<Signature>() },
			{ cosigners[3], test::GenerateRandomByteArray<Signature>() },
		};

		auto notification = Notification(Block_Signer_Key, committee.Round, cosignatures.size(), cosignatures.data(), Fee_Interest, Fee_Interest_Denominator);

		auto pObserver = CreateUpdateHarvestersObserver(pCommitteeManager);
		auto& committeeCache = context.cache().sub<cache::CommitteeCache>();
		auto& accountCache = context.cache().sub<cache::AccountStateCache>();

		std::vector<state::CommitteeEntry> initialEntries{
			{ state::CommitteeEntry(Block_Signer_Key, test::CreateAccountData(Initial_Height, Importance())) },
			{ state::CommitteeEntry(cosigners[0], test::CreateAccountData(Initial_Height, Importance())) },
			{ state::CommitteeEntry(cosigners[1], test::CreateAccountData(Initial_Height, Importance())) },
			{ state::CommitteeEntry(cosigners[2], test::CreateAccountData(Initial_Height, Importance())) },
			{ state::CommitteeEntry(cosigners[3], test::CreateAccountData(Initial_Height, Importance())) },
			{ state::CommitteeEntry(test::GenerateRandomByteArray<Key>(), test::CreateAccountData(Initial_Height, Importance())) },
		};

		size_t count = 0;
		for (const auto& entry : initialEntries) {
			committeeCache.insert(entry);
			pAccountCollector->addAccount(entry);

			accountCache.addAccount(entry.key(), Initial_Height);
			auto &accountState = accountCache.find(entry.key()).get();
			accountState.Balances.track(Harvesting_Mosaic_Id);
			accountState.Balances.credit(Harvesting_Mosaic_Id, Amount(++count * 30), Initial_Height);
		}

		std::vector<state::CommitteeEntry> expectedEntries{
			{ state::CommitteeEntry(initialEntries[0].key(), test::CreateAccountData(Current_Height, Importance(30), false, test::Initial_Activity + Activity_Committee_Cosigned_Delta - Activity_Delta,
				static_cast<double>(Fee_Interest) / Fee_Interest_Denominator)) },
			{ state::CommitteeEntry(initialEntries[1].key(), test::CreateAccountData(Initial_Height, Importance(60), false, test::Initial_Activity - Activity_Committee_Not_Cosigned_Delta - Activity_Delta)) },
			{ state::CommitteeEntry(initialEntries[2].key(), test::CreateAccountData(Initial_Height, Importance(90), false, test::Initial_Activity + Activity_Committee_Cosigned_Delta - Activity_Delta)) },
			{ state::CommitteeEntry(initialEntries[3].key(), test::CreateAccountData(Initial_Height, Importance(120), true, test::Initial_Activity - Activity_Committee_Not_Cosigned_Delta - Activity_Delta)) },
			{ state::CommitteeEntry(initialEntries[4].key(), test::CreateAccountData(Initial_Height, Importance(150), true, test::Initial_Activity + Activity_Committee_Cosigned_Delta - Activity_Delta)) },
			{ state::CommitteeEntry(initialEntries[5].key(), test::CreateAccountData(Initial_Height, Importance(180), true, test::Initial_Activity - Activity_Delta)) },
		};

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		for (const auto& expectedEntry : expectedEntries) {
			auto iter = committeeCache.find(expectedEntry.key());
			const auto &actualEntry = iter.get();
			test::AssertEqualCommitteeEntry(expectedEntry, actualEntry);
		}
	}

	TEST(TEST_CLASS, UpdateHarvesters_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		auto notification = Notification(Block_Signer_Key, 0u, 0u, nullptr, Fee_Interest, Fee_Interest_Denominator);
		auto pObserver = CreateUpdateHarvestersObserver(nullptr);

		// Act:
		EXPECT_THROW(test::ObserveNotification(*pObserver, notification, context), catapult_runtime_error);
	}
}}
