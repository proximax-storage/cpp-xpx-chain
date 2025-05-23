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
	constexpr double Initial_Activity = 0.367976785;
	
	state::CommitteeEntry CreateCommitteeEntry(
		Key key = test::GenerateRandomByteArray<Key>(),
		Key owner = test::GenerateRandomByteArray<Key>(),
		const Height& disabledHeight = Height(0),
		const Height& lastSigningBlockHeight = test::GenerateRandomValue<Height>(),
		const Importance& effectiveBalance = test::GenerateRandomValue<Importance>(),
		bool canHarvest = test::RandomByte(),
		double activityObsolete = Initial_Activity,
		double greedObsolete = Min_Greed);

	state::AccountData CreateAccountData(
		const Height& lastSigningBlockHeight = test::GenerateRandomValue<Height>(),
		const Importance& effectiveBalance = test::GenerateRandomValue<Importance>(),
		bool canHarvest = true,
		double activityObsolete = Initial_Activity,
		double greedObsolete = Min_Greed);

	void AssertEqualAccountData(const state::AccountData& data1, const state::AccountData& data2);
	void AssertEqualCommitteeEntry(const state::CommitteeEntry& entry1, const state::CommitteeEntry& entry2);

	struct CommitteeCacheFactory {
	private:
		static auto CreateSubCachesWithCommitteeCache(const config::BlockchainConfiguration& config) {
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cache::CommitteeCache::Id + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			const_cast<model::NetworkConfiguration&>(pConfigHolder->Config().Network).SetPluginConfiguration(config::CommitteeConfiguration::Uninitialized());
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
		using Base = chain::WeightedVotingCommitteeManager;

	public:
		explicit TestWeightedVotingCommitteeManager(const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector)
			: Base(pAccountCollector)
			, m_round(-1)
		{}

	public:
		void selectCommittee(const model::NetworkConfiguration& networkConfig, const BlockchainVersion& blockchainVersion) override {
			if (m_round >= 0)
				decreaseActivities(networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>());

			m_round++;
			m_committee = m_committees.at(m_round);
		}

		void reset() override {
			m_round = -1;
			Base::reset();
		}

	public:
		void setCommittee(int64_t round, const chain::Committee& committee) {
			m_committees[round] = committee;
		}

	private:
		int64_t m_round;
		std::map<int64_t, chain::Committee> m_committees;
	};
}}


