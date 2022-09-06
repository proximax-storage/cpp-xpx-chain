/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/LiquidityProviderEntry.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "src/state/LiquidityProviderEntrySerializer.h"

namespace catapult { namespace state {

#define TEST_CLASS LiquidityProviderEntrySerializerTests

	namespace {
		constexpr auto Replicator_Count = 10;
        constexpr auto Active_Data_Modifications_Count = 5;
        constexpr auto Completed_Data_Modifications_Count = 5;
        constexpr auto Verifications_Count = 1;

        constexpr auto Entry_Size =
            sizeof(VersionType) + // version
            Key_Size + // drive key
            Key_Size + // owner
            Hash256_Size + // root hash
            sizeof(uint64_t) + // size
            sizeof(uint64_t) + // used size
            sizeof(uint64_t) + // meta files size
            sizeof(uint16_t) + // replicator count
            sizeof(uint16_t) + // active data modifications count
            Active_Data_Modifications_Count * (Hash256_Size + Key_Size + Hash256_Size + sizeof(uint64_t)) + // active data modifications
            sizeof(uint16_t) + // completed data modifications count
            Completed_Data_Modifications_Count * (Hash256_Size + Key_Size + Hash256_Size + sizeof(uint64_t) + sizeof(uint8_t)) + // completed data modifications
            sizeof(uint16_t) + // verifications count
            Verifications_Count * (Hash256_Size + sizeof(uint16_t) + Replicator_Count * sizeof(uint16_t)); // verification data

        class TestContext {
        public:
            explicit TestContext()
                    : m_stream(m_buffer)
            {}

        public:
            auto& buffer() {
                return m_buffer;
            }

            auto& outputStream() {
                return m_stream;
            }

        private:
            std::vector<uint8_t> m_buffer;
            mocks::MockMemoryStream m_stream;
        };

        auto CreateLiquidityProviderEntry() {
        	state::LiquidityProviderEntry entry(UnresolvedMosaicId {test::Random()});

        	entry.setOwner(test::GenerateRandomByteArray<Key>());

        	auto maxWindowSize = test::Random16();
        	entry.setWindowSize(maxWindowSize);

        	for (int i = 0; i < maxWindowSize; i++) {
        		entry.turnoverHistory().push_back(state::HistoryObservation{
        			state::ExchangeRate{
        				Amount {test::Random()},
        				Amount {test::Random()}
        				},
        				Amount {test::Random()}});
        	}

        	entry.recentTurnover() = state::HistoryObservation{
        		state::ExchangeRate{
        			Amount {test::Random()},
        			Amount {test::Random()}
        			},
        			Amount {test::Random()}};
        	entry.setCreationHeight(test::GenerateRandomValue<Height>());
        	entry.setSlashingPeriod(test::Random16());
        	entry.setSlashingAccount(test::GenerateRandomByteArray<Key>());
        	entry.setProviderKey(test::GenerateRandomByteArray<Key>());
        	entry.setAdditionallyMinted(test::GenerateRandomValue<Amount>());
			entry.setAlpha(test::Random16());
			entry.setBeta(test::Random16());

        	return entry;
        }

        void AssertHistoryObservation(const HistoryObservation& observation, const uint8_t*& pData) {
			const auto& rate = observation.m_rate;

			ASSERT_EQ(rate.m_currencyAmount.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			ASSERT_EQ(rate.m_mosaicAmount.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			ASSERT_EQ(observation.m_turnover.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
		}

		void AssertHistoryObservations(const std::deque<HistoryObservation>& observations, const uint8_t*& pData) {
			auto size = observations.size();
			ASSERT_EQ(size, *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& observation: observations) {
				AssertHistoryObservation(observation, pData);
			}
		}

        void AssertEntryBuffer(const state::LiquidityProviderEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
            const auto* const pExpectedEnd = pData + expectedSize;
            ASSERT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			EXPECT_EQ(entry.mosaicId().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ_MEMORY(entry.providerKey().data(), pData, Key_Size);
			pData += Key_Size;
            EXPECT_EQ_MEMORY(entry.owner().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ(entry.additionallyMinted().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ_MEMORY(entry.slashingAccount().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ(entry.slashingPeriod(), *reinterpret_cast<const uint32_t*>(pData));
			pData += sizeof(uint32_t);
			EXPECT_EQ(entry.windowSize(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			EXPECT_EQ(entry.creationHeight().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.alpha(), *reinterpret_cast<const uint32_t*>(pData));
			pData += sizeof(uint32_t);
			EXPECT_EQ(entry.beta(), *reinterpret_cast<const uint32_t*>(pData));
			pData += sizeof(uint32_t);

			AssertHistoryObservations(entry.turnoverHistory(), pData);
			AssertHistoryObservation(entry.recentTurnover(), pData);

			ASSERT_EQ(pExpectedEnd, pData);
        }

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = CreateLiquidityProviderEntry();

			// Act:
			LiquidityProviderEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			AssertEntryBuffer(entry, context.buffer().data(), context.buffer().size(), version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = CreateLiquidityProviderEntry();
			auto entry2 = CreateLiquidityProviderEntry();

			// Act:
			LiquidityProviderEntrySerializer::Save(entry1, context.outputStream());
			auto entryBufferSize1 = context.buffer().size();
			LiquidityProviderEntrySerializer::Save(entry2, context.outputStream());
			auto entryBufferSize2 = context.buffer().size() - entryBufferSize1;

			// Assert:
			const auto* pBuffer1 = context.buffer().data();
			const auto* pBuffer2 = pBuffer1 + entryBufferSize1;
			AssertEntryBuffer(entry1, pBuffer1, entryBufferSize1, version);
			AssertEntryBuffer(entry2, pBuffer2, entryBufferSize2, version);
		}
    }

    // region Save

    TEST(TEST_CLASS, CanSaveSingleEntry_v1) {
        AssertCanSaveSingleEntry(1);
    }

    TEST(TEST_CLASS, CanSaveMultipleEntries_v1) {
        AssertCanSaveMultipleEntries(1);
    }

    // endregion

    // region Load

    namespace {

		void CopyToVector(std::vector<uint8_t>& data, const uint8_t * p, size_t bytes) {
			data.insert(data.end(), p, p + bytes);
		}

		void SaveHistoryObservation(const HistoryObservation& observation, std::vector<uint8_t>& data) {
			CopyToVector(data, reinterpret_cast<const uint8_t*>(&observation.m_rate.m_currencyAmount), sizeof(uint64_t));
			CopyToVector(data, reinterpret_cast<const uint8_t*>(&observation.m_rate.m_mosaicAmount), sizeof(uint64_t));
			CopyToVector(data, reinterpret_cast<const uint8_t*>(&observation.m_turnover), sizeof(uint64_t));
		}

		void SaveHistoryObservations(const std::deque<HistoryObservation>& observations, std::vector<uint8_t>& data) {
			auto size = observations.size();
			CopyToVector(data, reinterpret_cast<const uint8_t*>(&size), sizeof(uint16_t));
			for (const auto& observation: observations) {
				SaveHistoryObservation(observation, data);
			}
		}

        std::vector<uint8_t> CreateEntryBuffer(const state::LiquidityProviderEntry& entry, VersionType version) {
            std::vector<uint8_t> data;
			CopyToVector(data, (const uint8_t*)&version, sizeof(version));
			CopyToVector(data, reinterpret_cast<const uint8_t*>(&entry.mosaicId()), sizeof(uint64_t));
			CopyToVector(data, entry.providerKey().data(), Key_Size);
			CopyToVector(data, entry.owner().data(), Key_Size);
			CopyToVector(data, reinterpret_cast<const uint8_t*>(&entry.additionallyMinted()), sizeof(uint64_t));
			CopyToVector(data, entry.slashingAccount().data(), Key_Size);

			auto slashingPeriod = entry.slashingPeriod();
			CopyToVector(data, reinterpret_cast<const uint8_t*>(&slashingPeriod), sizeof(uint32_t));

			auto windowSize = entry.windowSize();
			CopyToVector(data, reinterpret_cast<const uint8_t*>(&windowSize), sizeof(uint16_t));

			CopyToVector(data, reinterpret_cast<const uint8_t*>(&entry.creationHeight()), sizeof(uint64_t));

			auto alpha = entry.alpha();
			CopyToVector(data, reinterpret_cast<const uint8_t*>(&alpha), sizeof(uint32_t));

			auto beta = entry.beta();
			CopyToVector(data, reinterpret_cast<const uint8_t*>(&beta), sizeof(uint32_t));

			SaveHistoryObservations(entry.turnoverHistory(), data);
			SaveHistoryObservation(entry.recentTurnover(), data);

            return data;
        }

		void AssertEqualHistoryObservation(const HistoryObservation& original, const HistoryObservation& result) {
			ASSERT_EQ(original.m_rate.m_currencyAmount, result.m_rate.m_currencyAmount);
			ASSERT_EQ(original.m_rate.m_mosaicAmount, result.m_rate.m_mosaicAmount);
			ASSERT_EQ(original.m_turnover, result.m_turnover);
		}

		void AssertEqualHistoryObservations(const std::deque<HistoryObservation>& original, const std::deque<HistoryObservation>& result) {
			ASSERT_EQ(original.size(), result.size());
			for (auto originalIt = original.begin(), resultIt = result.begin(); originalIt != original.end();
				 originalIt++, resultIt++) {
				AssertEqualHistoryObservation(*originalIt, *resultIt);
			}
		}

		void AssertEqualLiquidityProviderData(const LiquidityProviderEntry& original, const LiquidityProviderEntry& result) {
			ASSERT_EQ(original.version(), result.version());
			ASSERT_EQ(original.mosaicId(), result.mosaicId());
			ASSERT_EQ(original.providerKey(), result.providerKey());
			ASSERT_EQ(original.owner(), result.owner());
			ASSERT_EQ(original.additionallyMinted(), result.additionallyMinted());
			ASSERT_EQ(original.slashingAccount(), result.slashingAccount());
			ASSERT_EQ(original.slashingPeriod(), result.slashingPeriod());
			ASSERT_EQ(original.windowSize(), result.windowSize());
			ASSERT_EQ(original.creationHeight(), result.creationHeight());
			ASSERT_EQ(original.alpha(), result.alpha());
			ASSERT_EQ(original.beta(), result.beta());

			AssertEqualHistoryObservations(original.turnoverHistory(), result.turnoverHistory());
			AssertEqualHistoryObservation(original.recentTurnover(), result.recentTurnover());
		}

        void AssertCanLoadSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto originalEntry = CreateLiquidityProviderEntry();
            auto buffer = CreateEntryBuffer(originalEntry, version);

            // Act:
            state::LiquidityProviderEntry result(test::GenerateRandomValue<UnresolvedMosaicId>());
            test::RunLoadValueTest<LiquidityProviderEntrySerializer>(buffer, result);

            // Assert:
            AssertEqualLiquidityProviderData(originalEntry, result);
        }
    }

    TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
        AssertCanLoadSingleEntry(1);
    }

    // endregion

}}