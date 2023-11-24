/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/CommitteeTestUtils.h"
#include "tests/test/core/SerializerTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS CommitteeEntryPatriciaTreeSerializerTests

	namespace {
		constexpr auto Entry_Size = sizeof(VersionType) // version
			+ Key_Size // key
			+ Key_Size // owner
			+ sizeof(uint64_t) // disabled height
			+ sizeof(uint64_t) // last signing block height
			+ sizeof(uint64_t) // effective balance
			+ 1; // can harvest;

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

		void AssertEntryBuffer(const state::CommitteeEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			EXPECT_EQ_MEMORY(entry.key().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ_MEMORY(entry.owner().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ(entry.disabledHeight().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.lastSigningBlockHeight().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.effectiveBalance().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.canHarvest(), *reinterpret_cast<const bool*>(pData));
			pData += 1;

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = test::CreateCommitteeEntry();

			// Act:
			CommitteeEntryPatriciaTreeSerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(Entry_Size, context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = test::CreateCommitteeEntry();
			auto entry2 = test::CreateCommitteeEntry();

			// Act:
			CommitteeEntryPatriciaTreeSerializer::Save(entry1, context.outputStream());
			CommitteeEntryPatriciaTreeSerializer::Save(entry2, context.outputStream());

			// Assert:
			ASSERT_EQ(2 * Entry_Size, context.buffer().size());
			const auto* pBuffer1 = context.buffer().data();
			const auto* pBuffer2 = pBuffer1 + Entry_Size;
			AssertEntryBuffer(entry1, pBuffer1, Entry_Size, version);
			AssertEntryBuffer(entry2, pBuffer2, Entry_Size, version);
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

		std::vector<uint8_t> CreateEntryBuffer(const state::CommitteeEntry& entry, VersionType version) {
			std::vector<uint8_t> buffer(Entry_Size);

			auto* pData = buffer.data();
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			memcpy(pData, entry.key().data(), Key_Size);
			pData += Key_Size;
			memcpy(pData, entry.owner().data(), Key_Size);
			pData += Key_Size;
			auto disabledHeight = entry.disabledHeight().unwrap();
			memcpy(pData, &disabledHeight, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			auto lastSigningBlockHeight = entry.lastSigningBlockHeight().unwrap();
			memcpy(pData, &lastSigningBlockHeight, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			auto effectiveBalance = entry.effectiveBalance().unwrap();
			memcpy(pData, &effectiveBalance, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			*pData++ = entry.canHarvest();

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = test::CreateCommitteeEntry();
			originalEntry.setActivity(0.0);
			originalEntry.setGreed(0.0);
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			auto result = test::CreateCommitteeEntry();
			test::RunLoadValueTest<CommitteeEntryPatriciaTreeSerializer>(buffer, result);

			// Assert:
			test::AssertEqualCommitteeEntry(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
		AssertCanLoadSingleEntry(1);
	}

	// endregion
}}
