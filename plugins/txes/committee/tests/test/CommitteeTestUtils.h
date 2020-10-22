/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/chain/WeightedVotingCommitteeManager.h"
#include "plugins/txes/committee/src/cache/CommitteeAccountCollector.h"
#include "plugins/txes/committee/src/cache/CommitteeCache.h"
#include "plugins/txes/committee/src/cache/CommitteeCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/nodeps/Random.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace test {

	constexpr double Min_Greed = 0.1;
	constexpr double Initial_Activity = std::log(static_cast<double>(7.0)/3.0);
	
	state::CommitteeEntry CreateCommitteeEntry(
		Key key = test::GenerateRandomByteArray<Key>(),
		const Height& lastSigningBlockHeight = test::GenerateRandomValue<Height>(),
		const Importance& effectiveBalance = test::GenerateRandomValue<Importance>(),
		bool canHarvest = test::RandomByte(),
		double activity = Initial_Activity,
		double greed = Min_Greed);

	state::AccountData CreateAccountData(
		const Height& lastSigningBlockHeight = test::GenerateRandomValue<Height>(),
		const Importance& effectiveBalance = test::GenerateRandomValue<Importance>(),
		bool canHarvest = true,
		double activity = Initial_Activity,
		double greed = Min_Greed);

	void AssertEqualAccountData(const state::AccountData& data1, const state::AccountData& data2);
	void AssertEqualCommitteeEntry(const state::CommitteeEntry& entry1, const state::CommitteeEntry& entry2);

	struct CommitteeCacheFactory {
	private:
		static auto CreateSubCachesWithCommitteeCache(const config::BlockchainConfiguration& config) {
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cache::CommitteeCache::Id + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto pAccountCollector = std::make_shared<cache::CommitteeAccountCollector>();
			subCaches[cache::CommitteeCache::Id] =
				test::MakeSubCachePlugin<cache::CommitteeCache, cache::CommitteeCacheStorage>(pAccountCollector, pConfigHolder);
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCachesWithCommitteeCache(config);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateHarvesterTransaction() {
        uint32_t entitySize = sizeof(TTransaction);
        auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();
		pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
        pTransaction->Type = TTransaction::Entity_Type;
        pTransaction->Size = entitySize;

        return pTransaction;
    }

	class TestWeightedVotingCommitteeManager : public chain::WeightedVotingCommitteeManager {
	public:
		explicit TestWeightedVotingCommitteeManager(const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector)
			: chain::WeightedVotingCommitteeManager(pAccountCollector)
		{}

	public:
		void setCommittee(const chain::Committee& committee) {
			m_committee = committee;
		}
	};
}}


