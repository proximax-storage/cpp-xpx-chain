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

#include "catapult/local/server/FileStateChangeStorage.h"
#include "catapult/subscribers/StateChangeInfo.h"
#include "catapult/subscribers/SubscriberOperationTypes.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockMemoryStream.h"
#include "tests/TestHarness.h"

namespace catapult { namespace local {

#define TEST_CLASS FileStateChangeStorageTests

	namespace {
		// region MockCacheChangesStorageWriter

		// simulate CacheChangesStorage by writing uint32_t value and CacheChanges pointer in saveAll
		class MockCacheChangesStorageWriter : public cache::CacheChangesStorage {
		public:
			explicit MockCacheChangesStorageWriter(uint32_t value) : m_value(value)
			{}

		public:
			void saveAll(const cache::CacheChanges& changes, io::OutputStream& output) const override {
				io::Write32(output, m_value);
				io::Write64(output, reinterpret_cast<uintptr_t>(&changes));
			}

			std::unique_ptr<const cache::MemoryCacheChanges> loadAll(io::InputStream&) const override {
				CATAPULT_THROW_INVALID_ARGUMENT("loadAll - not supported in mock");
			}

			void apply(const cache::CacheChanges&) const override {
				CATAPULT_THROW_INVALID_ARGUMENT("apply - not supported in mock");
			}

		private:
			uint32_t m_value;
		};

		// endregion
	}

	TEST(TEST_CLASS, NotifyScoreChangeWritesToUnderlyingStream) {
		// Arrange: create output stream
		std::vector<uint8_t> buffer;
		auto pStream = std::make_unique<mocks::MockMemoryStream>(buffer);
		const auto& stream = *pStream;

		// - create data
		auto chainScore = model::ChainScore(test::Random(), test::Random());

		// - create storage
		auto pStorage = CreateFileStateChangeStorage(std::move(pStream), {});

		// Act:
		pStorage->notifyScoreChange(chainScore);

		// Assert:
		EXPECT_EQ(1u, stream.numFlushes());
		ASSERT_EQ(1u + 2 * sizeof(uint64_t), buffer.size());

		auto expectedOperationType = subscribers::StateChangeOperationType::Score_Change;
		EXPECT_EQ(expectedOperationType, static_cast<subscribers::StateChangeOperationType>(buffer[0]));

		auto* pUint64Values = reinterpret_cast<const uint64_t*>(buffer.data() + 1);
		EXPECT_EQ(chainScore.toArray()[0], pUint64Values[0]);
		EXPECT_EQ(chainScore.toArray()[1], pUint64Values[1]);
	}

	TEST(TEST_CLASS, NotifyStateChangeWritesToUnderlyingStream) {
		// Arrange: create output stream
		std::vector<uint8_t> buffer;
		auto pStream = std::make_unique<mocks::MockMemoryStream>(buffer);
		const auto& stream = *pStream;

		// - create data
		auto chainScore = model::ChainScore(test::Random(), test::Random());
		auto height = test::GenerateRandomValue<Height>();
		auto stateChangeInfo = subscribers::StateChangeInfo(cache::CacheChanges({}), chainScore, height);

		// - simulate two cache changes storages
		auto storageSentinel1 = static_cast<uint32_t>(test::Random());
		auto storageSentinel2 = static_cast<uint32_t>(test::Random());

		// - create storage
		auto pStorage = CreateFileStateChangeStorage(std::move(pStream), [storageSentinel1, storageSentinel2]() {
			CacheChangesStorages cacheChangesStorages;
			cacheChangesStorages.emplace_back(std::make_unique<MockCacheChangesStorageWriter>(storageSentinel1));
			cacheChangesStorages.emplace_back(std::make_unique<MockCacheChangesStorageWriter>(storageSentinel2));
			return cacheChangesStorages;
		});

		// Sanity:
		auto expectedCacheChangesPointerValue = reinterpret_cast<uintptr_t>(&stateChangeInfo.CacheChanges);
		EXPECT_NE(0u, expectedCacheChangesPointerValue);

		// Act:
		pStorage->notifyStateChange(stateChangeInfo);

		// Assert:
		EXPECT_EQ(1u, stream.numFlushes());
		ASSERT_EQ(1u + 3 * sizeof(uint64_t) + 2 * (sizeof(uint32_t) + sizeof(uintptr_t)), buffer.size());

		auto expectedOperationType = subscribers::StateChangeOperationType::State_Change;
		EXPECT_EQ(expectedOperationType, static_cast<subscribers::StateChangeOperationType>(buffer[0]));

		auto* pUint64Values = reinterpret_cast<const uint64_t*>(buffer.data() + 1);
		EXPECT_EQ(chainScore.toArray()[0], pUint64Values[0]);
		EXPECT_EQ(chainScore.toArray()[1], pUint64Values[1]);
		EXPECT_EQ(height, Height(pUint64Values[2]));

		// - check that both uint32_t value and CacheChanges pointer were written for each storage
		auto offset = 1 + 3 * sizeof(uint64_t);
		for (auto storageSentinel : { storageSentinel1, storageSentinel2 }) {
			EXPECT_EQ(storageSentinel, reinterpret_cast<const uint32_t&>(buffer[offset])) << offset;
			offset += sizeof(uint32_t);

			EXPECT_EQ(expectedCacheChangesPointerValue, reinterpret_cast<const uintptr_t&>(buffer[offset])) << offset;
			offset += sizeof(uintptr_t);
		}
	}
}}
