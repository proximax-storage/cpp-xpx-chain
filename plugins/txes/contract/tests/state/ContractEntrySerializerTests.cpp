/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/ContractEntrySerializer.h"
#include "catapult/utils/HexFormatter.h"
#include "tests/test/ReputationTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/SerializerOrderingTests.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS ContractEntrySerializerTests

	namespace {
		constexpr auto Entry_Size = Key_Size * 3 + Key_Size * 3 + Key_Size * 3 + sizeof(uint64_t) * 3 + Hash256_Size + sizeof(uint64_t) * 2 + Key_Size;
		class TestContext {
		public:
			explicit TestContext(size_t numAccounts = 10)
					: m_stream("", m_buffer)
					, m_accountKeys(test::GenerateKeys(numAccounts))
			{}

		public:
			auto& buffer() {
				return m_buffer;
			}

			auto& outputStream() {
				return m_stream;
			}

		public:
			auto createEntry(size_t mainAccountId) {
				state::ContractEntry entry(m_accountKeys[mainAccountId]);
				entry.setDuration(BlockDuration(mainAccountId + 20));
				entry.setStart(Height(mainAccountId + 12));
				entry.setHash(test::GenerateRandomData<Hash256_Size>());
				entry.customers() = { test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>() };
				entry.executors() = { test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>() };
				entry.verifiers() = { test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>() };

				return entry;
			}

		private:
			std::vector<uint8_t> m_buffer;
			mocks::MockMemoryStream m_stream;
			std::vector<Key> m_accountKeys;
		};

		template<std::size_t N>
		std::array<uint8_t, N> Extract(const uint8_t* data) {
			std::array<uint8_t, N> hash = { 0 };
			memcpy(hash.data(), data, N);
			return hash;
		}

		void AssertVectorBuffer(utils::SortedKeySet keys, const uint8_t* pData) {
			const auto* pExpectedEnd = pData + Key_Size * keys.size() + sizeof(uint64_t);

			auto count = *reinterpret_cast<const uint64_t*>(pData);
			EXPECT_EQ(keys.size(), count);
			pData += sizeof(uint64_t);
			for (auto i = 0u; i < count; ++i) {
				auto accountKey = Extract<Key_Size>(pData);
				EXPECT_TRUE(keys.count(accountKey));
				pData += Key_Size;
				keys.erase(keys.find(accountKey));
			}
			EXPECT_TRUE(keys.empty());

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertEntryBuffer(const state::ContractEntry& entry, const uint8_t* pData, size_t expectedSize) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(entry.start().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.duration().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);

			auto hash = Extract<Hash256_Size>(pData);
			EXPECT_EQ(entry.hash(), hash);
			pData += Hash256_Size;

			auto accountKey = Extract<Key_Size>(pData);
			EXPECT_EQ(entry.key(), accountKey);
			pData += Key_Size;

			AssertVectorBuffer(entry.customers(), pData);
			pData += Key_Size * entry.customers().size() + sizeof(uint64_t);
			AssertVectorBuffer(entry.executors(), pData);
			pData += Key_Size * entry.executors().size() + sizeof(uint64_t);
			AssertVectorBuffer(entry.verifiers(), pData);
			pData += Key_Size * entry.verifiers().size() + sizeof(uint64_t);

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertEqual(const state::ContractEntry& expectedEntry, const state::ContractEntry& entry) {
			EXPECT_EQ(expectedEntry.key(), entry.key());
			EXPECT_EQ(expectedEntry.start(), entry.start());
			EXPECT_EQ(expectedEntry.duration(), entry.duration());
			EXPECT_EQ(expectedEntry.customers(), entry.customers());
			EXPECT_EQ(expectedEntry.executors(), entry.executors());
			EXPECT_EQ(expectedEntry.verifiers(), entry.verifiers());
		}
	}

	// region Save

	TEST(TEST_CLASS, CanSaveSingleEntry) {
		// Arrange:
		TestContext context;
		auto entry = context.createEntry(0);

		// Act:
		ContractEntrySerializer::Save(entry, context.outputStream());

		// Assert:
		ASSERT_EQ(Entry_Size, context.buffer().size());
		AssertEntryBuffer(entry, context.buffer().data(), Entry_Size);
	}

	TEST(TEST_CLASS, CanSaveMultipleEntries) {
		// Arrange:
		TestContext context(20);
		auto entry1 = context.createEntry(0);
		auto entry2 = context.createEntry(10);

		// Act:
		ContractEntrySerializer::Save(entry1, context.outputStream());
		ContractEntrySerializer::Save(entry2, context.outputStream());

		// Assert:
		ASSERT_EQ(2 * Entry_Size, context.buffer().size());
		const auto* pBuffer1 = context.buffer().data();
		const auto* pBuffer2 = pBuffer1 + Entry_Size;
		AssertEntryBuffer(entry1, pBuffer1, Entry_Size);
		AssertEntryBuffer(entry2, pBuffer2, Entry_Size);
	}

	// endregion

	// region Load

	namespace {

		void AddVectorToBuffer(const utils::SortedKeySet& keys, uint8_t*& pData) {
			auto count = keys.size();
			memcpy(pData, &count, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			for (const auto& key : keys) {
				memcpy(pData, key.data(), Key_Size);
				pData += Key_Size;
			}
		}

		std::vector<uint8_t> CreateEntryBuffer(const state::ContractEntry& entry) {
			// positiveInteractions / negativeInteractions / key
			std::vector<uint8_t> buffer(Entry_Size);

			// - positiveInteractions / negativeInteractions
			auto* pData = buffer.data();
			auto positiveInteractions = entry.start().unwrap();
			memcpy(pData, &positiveInteractions, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			auto negativeInteractions = entry.duration().unwrap();
			memcpy(pData, &negativeInteractions, sizeof(uint64_t));
			pData += sizeof(uint64_t);

			// - hash
			memcpy(pData, entry.hash().data(), Hash256_Size);
			pData += Key_Size;

			// - account key
			memcpy(pData, entry.key().data(), Key_Size);
			pData += Key_Size;

			AddVectorToBuffer(entry.customers(), pData);
			AddVectorToBuffer(entry.executors(), pData);
			AddVectorToBuffer(entry.verifiers(), pData);

			return buffer;
		}

		void AssertCanLoadSingleEntry() {
			// Arrange:
			TestContext context;
			auto originalEntry = context.createEntry(0);
			auto buffer = CreateEntryBuffer(originalEntry);

			// Act:
			state::ContractEntry result(test::GenerateRandomData<Key_Size>());
			test::RunLoadValueTest<ContractEntrySerializer>(buffer, result);

			// Assert:
			AssertEqual(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry) {
		AssertCanLoadSingleEntry();
	}

	// endregion
}}
