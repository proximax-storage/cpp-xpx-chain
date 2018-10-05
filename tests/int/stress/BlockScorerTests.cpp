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

#include "catapult/chain/BlockScorer.h"
#include "catapult/crypto/Hashes.h"
#include "catapult/model/Block.h"
#include "catapult/utils/Logging.h"
#include "catapult/utils/TimeSpan.h"
#include "tests/TestHarness.h"
#include <boost/thread.hpp>
#include <deque>

namespace catapult { namespace chain {

#define TEST_CLASS BlockScorerTests

	namespace {
#ifdef STRESS
		constexpr size_t Num_Iterations = 100'000;
#else
		constexpr size_t Num_Iterations = 25000;
#endif

		constexpr Timestamp Max_Time(1000 * 1000);
		constexpr size_t HitCounterNumber{20};
		constexpr BlockTarget ParentBaseTarget{100000000u};
		constexpr uint64_t EffectiveBalanceStep{100000000u};

		model::BlockChainConfiguration CreateConfiguration() {
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(60);
			config.BlockTimeSmoothingFactor = 0;
			return config;
		}

		struct HitCounter {
		public:
			Amount EffectiveBalance;
			std::atomic<size_t> HitCount;
			Key Signer;

		public:
			explicit HitCounter(const Amount& effectiveBalance)
					: EffectiveBalance(effectiveBalance)
					, HitCount(0)
					, Signer(test::GenerateRandomData<Key_Size>())
			{}
		};

		using HitCounters = std::vector<std::unique_ptr<HitCounter>>;

		HitCounters CreateHitCounters() {
			HitCounters hitCounters;
			for (auto i = 0u; i < HitCounterNumber; ++i) {
				hitCounters.push_back(std::make_unique<HitCounter>(Amount{EffectiveBalanceStep * (i + 1)}));
			}

			return hitCounters;
		}

		class AverageBlockTime {
		public:
			void Add(const utils::TimeSpan& blockTime)
			{
				Add(blockTime.millis());
			}

			void Add(const Timestamp& blockTime)
			{
				Add(blockTime.unwrap());
			}

			void Add(uint64_t blockTime)
			{
				if (m_blockTimes.size() == Block_Timestamp_History_Size) {
					m_blockTimes.pop_front();
				}

				m_blockTimes.push_back(blockTime);
				m_average = 0;
				std::for_each(m_blockTimes.begin(), m_blockTimes.end(), [this](const uint64_t& value){ m_average += value; });
				m_average /= m_blockTimes.size();
			}

			utils::TimeSpan Get() { return utils::TimeSpan::FromMilliseconds(m_average); }
		private:
			std::deque<uint64_t> m_blockTimes;
			uint64_t m_average;
		};

		// calculate the block time by updating the Timestamp in current and checking if it hits using the predicate
		Timestamp GetBlockTime(
				const Amount& effectiveBalance,
				const Hash256& generationHash,
				const BlockTarget& baseTarget,
				const BlockHitPredicate& predicate) {
			const auto MS_In_S = 1000;

			// - make sure that there is a hit possibility
			Timestamp elapsedTime = Max_Time;
			if (!predicate(generationHash, baseTarget, utils::TimeSpan::FromMilliseconds(elapsedTime.unwrap()), effectiveBalance))
				return Max_Time;

			// - use a binary search to find the hit time
			uint64_t lowerBound = 0;
			uint64_t upperBound = Max_Time.unwrap();
			while (upperBound - lowerBound > MS_In_S) {
				auto middle = (upperBound + lowerBound) / 2;
				elapsedTime = Timestamp(middle);

				if (predicate(generationHash, baseTarget, utils::TimeSpan::FromMilliseconds(elapsedTime.unwrap()), effectiveBalance)) {
					upperBound = middle;
				} else {
					lowerBound = middle;
				}
			}

			return elapsedTime;
		}

		// runs an iteration and returns the next generation hash
		void RunHitCountIteration(
				HitCounters& hitCounters,
				Hash256& parentGenerationHash,
				AverageBlockTime& averageBlockTime,
				const model::BlockChainConfiguration& config) {
			BlockHitPredicate predicate;
			Timestamp bestTime = Max_Time;
			Hash256 bestGenerationHash{};
			BlockTarget baseTarget = CalculateBaseTarget(ParentBaseTarget, averageBlockTime.Get(), config);
			HitCounter* pBestHitter = nullptr;
			for (const auto& pHitCounter : hitCounters) {
				// - set the signer and generation hash
				Hash256 nextGenerationHash;

				crypto::Sha3_256_Builder sha3;
				sha3.update(parentGenerationHash);
				sha3.update(pHitCounter->Signer);
				sha3.final(nextGenerationHash);

				auto time = GetBlockTime(pHitCounter->EffectiveBalance, nextGenerationHash, baseTarget, predicate);
				if (time >= bestTime)
					continue;

				bestTime = time;
				bestGenerationHash = nextGenerationHash;
				pBestHitter = pHitCounter.get();
			}

			if (!pBestHitter) {
				// - if no blocks hit, use a random generation hash for the next iteration
				//   (in a real scenario, this would result in a lowered difficulty)
				parentGenerationHash = test::GenerateRandomData<Hash256_Size>();
			} else {
				// - increment the hit count for the best group and use its generation hash for the next iteration
				++pBestHitter->HitCount;
				parentGenerationHash = bestGenerationHash;
				averageBlockTime.Add(bestTime);
			}
		}

		void RunHitCount(const model::BlockChainConfiguration& config, HitCounters& hitCounters) {

			// - calculate chain scores on all threads
			const auto numIterationsPerThread = Num_Iterations / test::GetNumDefaultPoolThreads();
			boost::thread_group threads;
			for (auto i = 0u; i < test::GetNumDefaultPoolThreads(); ++i) {
				threads.create_thread([&config, &hitCounters, i, numIterationsPerThread] {
					// Arrange: seed srand per thread
					std::srand(static_cast<unsigned int>(std::time(nullptr)) + (2u << i));

					// - set up blocks
					auto parentGenerationHash = test::GenerateRandomData<Hash256_Size>();

					AverageBlockTime averageBlockTime;
					averageBlockTime.Add(config.BlockGenerationTargetTime);

					// Act: calculate hit counts for lots of blocks
					for (auto j = 0u; j < numIterationsPerThread; ++j)
						RunHitCountIteration(hitCounters, parentGenerationHash, averageBlockTime, config);
				});
			}

			// - wait for all threads
			threads.join_all();
		}

		void CalculateLinearlyCorrelatedHitCountAndEffectiveBalance(const HitCounters& hitCounters) {
			// Assert:
			auto lowerBalanceHitCount = 0u;
			for (auto i = 0u; i < hitCounters.size() / 2; ++i) {
				lowerBalanceHitCount = hitCounters[i]->HitCount.load();
			}

			auto higherBalanceHitCount = 0u;
			for (auto i = hitCounters.size() / 2; i < hitCounters.size(); ++i) {
				higherBalanceHitCount = hitCounters[i]->HitCount.load();
			}

			EXPECT_GT(higherBalanceHitCount, lowerBalanceHitCount);
		}

		void AssertHitProbabilityIsLinearlyCorrelatedWithEffectiveBalance(const model::BlockChainConfiguration& config) {
			// Arrange:
			auto hitCounters = CreateHitCounters();

			//Act:
			RunHitCount(config, hitCounters);

			// Assert: the distribution is linearly correlated with effective balance
			CalculateLinearlyCorrelatedHitCountAndEffectiveBalance(hitCounters);
		}
	}

	NO_STRESS_TEST(TEST_CLASS, HitProbabilityIsLinearlyCorrelatedWithEffectiveBalance) {
		// Assert:
		auto config = CreateConfiguration();
		AssertHitProbabilityIsLinearlyCorrelatedWithEffectiveBalance(config);
	}

	NO_STRESS_TEST(TEST_CLASS, HitProbabilityIsLinearlyCorrelatedWithEffectiveBalanceWhenSmoothingIsEnabled) {
		// Assert:
		auto config = CreateConfiguration();
		config.BlockTimeSmoothingFactor = 10000;
		AssertHitProbabilityIsLinearlyCorrelatedWithEffectiveBalance(config);
	}
}}
