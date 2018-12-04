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

#include "src/state/ReputationEntrySerializer.h"
#include "catapult/utils/HexFormatter.h"
#include "tests/test/ReputationTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/SerializerOrderingTests.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS ReputationEntrySerializerTests

	namespace {
		constexpr auto Entry_Size = sizeof(uint64_t) * 2 + sizeof(Key);
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
				state::ReputationEntry entry(m_accountKeys[mainAccountId]);
				entry.setPositiveInteractions(Reputation{mainAccountId + 23});
				entry.setNegativeInteractions(Reputation{mainAccountId + 34});

				return entry;
			}

		private:
			std::vector<uint8_t> m_buffer;
			mocks::MockMemoryStream m_stream;
			std::vector<Key> m_accountKeys;
		};

		Key ExtractKey(const uint8_t* data) {
			Key key;
			memcpy(key.data(), data, Key_Size);
			return key;
		}

		void AssertEntryBuffer(const state::ReputationEntry& entry, const uint8_t* pData, size_t expectedSize) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(entry.positiveInteractions().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.negativeInteractions().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);

			auto accountKey = ExtractKey(pData);
			EXPECT_EQ(entry.key(), accountKey);
			pData += Key_Size;

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertEqual(const state::ReputationEntry& expectedEntry, const state::ReputationEntry& entry) {
			EXPECT_EQ(expectedEntry.positiveInteractions(), entry.positiveInteractions());
			EXPECT_EQ(expectedEntry.negativeInteractions(), entry.negativeInteractions());
			EXPECT_EQ(expectedEntry.key(), entry.key());
		}
	}

	// region Save

	TEST(TEST_CLASS, CanSaveSingleEntry) {
		// Arrange:
		TestContext context;
		auto entry = context.createEntry(0);

		// Act:
		ReputationEntrySerializer::Save(entry, context.outputStream());

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
		ReputationEntrySerializer::Save(entry1, context.outputStream());
		ReputationEntrySerializer::Save(entry2, context.outputStream());

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
		std::vector<uint8_t> CreateBuffer(const state::ReputationEntry& entry) {
			// positiveInteractions / negativeInteractions / key
			std::vector<uint8_t> buffer(Entry_Size);

			// - positiveInteractions / negativeInteractions
			auto* pData = buffer.data();
			auto positiveInteractions = entry.positiveInteractions().unwrap();
			memcpy(pData, &positiveInteractions, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			auto negativeInteractions = entry.negativeInteractions().unwrap();
			memcpy(pData, &negativeInteractions, sizeof(uint64_t));
			pData += sizeof(uint64_t);

			// - account key
			memcpy(pData, entry.key().data(), Key_Size);

			return buffer;
		}

		void AssertCanLoadSingleEntry() {
			// Arrange:
			TestContext context;
			auto originalEntry = context.createEntry(0);
			auto buffer = CreateBuffer(originalEntry);

			// Act:
			state::ReputationEntry result(test::GenerateRandomData<Key_Size>());
			test::RunLoadValueTest<ReputationEntrySerializer>(buffer, result);

			// Assert:
			AssertEqual(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry) {
		AssertCanLoadSingleEntry();
	}

	// endregion
}}
