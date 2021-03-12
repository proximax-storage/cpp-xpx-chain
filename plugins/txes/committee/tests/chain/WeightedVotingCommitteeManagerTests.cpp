/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/CommitteeTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"

namespace catapult { namespace chain {

#define TEST_CLASS WeightedVotingCommitteeManagerTests

	namespace {
		constexpr uint8_t Committee_Size = 21u;
		constexpr uint8_t Account_Number = 50u;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Network.CommitteeSize = Committee_Size;
			auto pluginConfig = config::CommitteeConfiguration::Uninitialized();
			pluginConfig.MinGreed = test::Min_Greed;
			config.Network.SetPluginConfiguration(pluginConfig);
			return config.ToConst().Network;
		}

		class MockHasher : public Hasher {
		public:
			Hash256& calculateHash(Hash256& hash, const GenerationHash&, const Key& key) const override {
				std::memcpy(static_cast<void*>(&hash), &key, Hash256_Size);
				return hash;
			}

			Hash256& calculateHash(Hash256& hash) const override {
				return hash;
			}

			Hash256 calculateHash(double, const Key& key) const override {
				Hash256 hash;
				std::memcpy(static_cast<void*>(&hash), &key, Hash256_Size);
				return hash;
			}
		};

		class TestWeightedVotingCommitteeManager : public WeightedVotingCommitteeManager {
		public:
			explicit TestWeightedVotingCommitteeManager(const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector)
					: WeightedVotingCommitteeManager(pAccountCollector) {
				m_pHasher = std::make_unique<MockHasher>();
			}
		};
		
		void RunTest(const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector, const Committee& expectedCommittee) {
			// Arrange:
			auto pCommitteeManager = std::make_shared<TestWeightedVotingCommitteeManager>(pAccountCollector);

			auto generationHash = test::GenerateRandomByteArray<GenerationHash>();
			auto pBlock = test::GenerateEmptyRandomBlock();
			auto pBlockElement = std::make_shared<model::BlockElement>(test::BlockToBlockElement(*pBlock, generationHash));
			pCommitteeManager->setLastBlockElementSupplier([pBlockElement]() { return pBlockElement; });

			// Act:
			auto actualCommittee = pCommitteeManager->selectCommittee(CreateConfig());

			// Assert:
			EXPECT_EQ(expectedCommittee.BlockProposer, actualCommittee.BlockProposer);
			ASSERT_EQ(expectedCommittee.Cosigners.size(), actualCommittee.Cosigners.size());
			for (const auto& key : expectedCommittee.Cosigners)
				EXPECT_TRUE(actualCommittee.Cosigners.find(key) != actualCommittee.Cosigners.end());
		}
	}

	TEST(TEST_CLASS, WeightedVotingCommitteeManager_AssertWhenNoCandidates) {
		EXPECT_THROW(RunTest(std::make_shared<cache::CommitteeAccountCollector>(), Committee()), catapult_runtime_error);
	}

	TEST(TEST_CLASS, WeightedVotingCommitteeManager_AssertWhenCommitteeNotFull) {
		Committee expectedCommittee;
		auto pAccountCollector = std::make_shared<cache::CommitteeAccountCollector>();

		// The number of accounts is less than the committee number.
		auto minHit = std::numeric_limits<uint64_t>::max();
		for (uint8_t i = 1u; i <= Committee_Size / 2; ++i) {
			auto key = test::GenerateRandomByteArray<Key>();
			expectedCommittee.Cosigners.insert(key);
			pAccountCollector->addAccount(state::CommitteeEntry(key, test::CreateAccountData(Height(), Importance(1))));
			auto hit = *reinterpret_cast<const uint64_t*>(key.data());
			if (hit < minHit) {
				minHit = hit;
				expectedCommittee.BlockProposer = key;
			}
		}
		expectedCommittee.Cosigners.erase(expectedCommittee.BlockProposer);

		EXPECT_THROW(RunTest(pAccountCollector, expectedCommittee), catapult_runtime_error);
	}

	TEST(TEST_CLASS, WeightedVotingCommitteeManager_SelectsCommittee) {
		Committee expectedCommittee;
		auto pAccountCollector = std::make_shared<cache::CommitteeAccountCollector>();

		// Generate Account_Number accounts with hits increasing from 1 to Account_Number. For this set the first byte
		// of account key equal to the counter. Account hit is the first 8 bytes of the account key because the mock
		// hasher doesn't do any transformation, but just copies the key to hash.
		// All account activities are equal, so any account rate is constant * effective balance / hit.
		// First Committee_Size accounts have the same effective balance and increasing hits, hence account rates are
		// in decreasing order, the first account has the largest rate. Set their greeds to be equal to 0.8 / hit.
		// The first 8 accounts have the minimum value of max(greed, min_greed) * hit (min_greed = 0.1) equal to 0.8.
		// The next accounts, as their greeds are less or equal 0.1, have values max(greed, min_greed) * hit = 0.1 * hit,
		// which are greater than 0.8. Among the first 8 accounts the block proposer is the very first account because
		// it has the lowest key.
		expectedCommittee.BlockProposer = Key{ { 1 } };
		pAccountCollector->addAccount(state::CommitteeEntry(
			expectedCommittee.BlockProposer,
			test::CreateAccountData(Height(), Importance(Committee_Size), true, test::Initial_Activity, 0.8)));
		for (uint8_t i = 2u; i <= Committee_Size; ++i) {
			auto key = Key{ { i } };
			expectedCommittee.Cosigners.insert(key);
			pAccountCollector->addAccount(state::CommitteeEntry(
				key, test::CreateAccountData(Height(), Importance(Committee_Size), true, test::Initial_Activity, 0.8 / i)));
		}

		// The next accounts have rates equal to the rate of the 21st account. The 21st account is selected to the
		// committee because it has the lowest key.
		for (uint8_t i = Committee_Size + 1u; i <= Account_Number; ++i)
			pAccountCollector->addAccount(state::CommitteeEntry(Key{ { i } }, test::CreateAccountData(Height(), Importance(i))));

		RunTest(pAccountCollector, expectedCommittee);
	}

	TEST(TEST_CLASS, WeightedVotingCommitteeManager_AssertWhenLastBlockElementSupplierNotSet) {
		// Arrange:
		auto pCommitteeManager = std::make_shared<TestWeightedVotingCommitteeManager>(std::make_shared<cache::CommitteeAccountCollector>());

		// Act + Assert:
		EXPECT_THROW(pCommitteeManager->lastBlockElementSupplier(), catapult_runtime_error);
	}

	TEST(TEST_CLASS, WeightedVotingCommitteeManager_SetLastBlockElementSupplier) {
		// Arrange:
		auto pCommitteeManager = std::make_shared<TestWeightedVotingCommitteeManager>(std::make_shared<cache::CommitteeAccountCollector>());
		pCommitteeManager->setLastBlockElementSupplier([]() { return nullptr; });

		// Act:
		auto pSupplier = pCommitteeManager->lastBlockElementSupplier();

		// Assert:
		EXPECT_NE(nullptr, pSupplier);
	}
}}
