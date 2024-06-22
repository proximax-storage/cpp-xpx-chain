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
		constexpr auto Entry_Size_v1 = sizeof(VersionType) // version
			+ Key_Size // key
			+ Key_Size // owner
			+ sizeof(uint64_t) // disabled height
			+ sizeof(uint64_t) // last signing block height
			+ sizeof(uint64_t) // effective balance
			+ 1; // can harvest

		constexpr auto Entry_Size_v2 = sizeof(VersionType) // version
			+ Key_Size // key
			+ Key_Size // owner
			+ sizeof(uint64_t) // disabled height
			+ sizeof(uint64_t) // last signing block height
			+ sizeof(uint64_t) // effective balance
			+ 1; // can harvest

		constexpr auto Entry_Size_v3 = sizeof(VersionType) // version
			+ Key_Size // key
			+ Key_Size // owner
			+ sizeof(uint64_t) // disabled height
			+ sizeof(uint64_t) // last signing block height
			+ sizeof(uint64_t) // effective balance
			+ 1 // can harvest
			+ sizeof(int64_t) // activity
			+ sizeof(uint32_t) // fee interest
			+ sizeof(uint32_t); // fee interest denominator

		constexpr auto Entry_Size_v4 = sizeof(VersionType) // version
			+ Key_Size // key
			+ Key_Size // owner
			+ sizeof(uint64_t) // disabled height
			+ sizeof(uint64_t) // last signing block height
			+ sizeof(uint64_t) // effective balance
			+ 1 // can harvest
			+ sizeof(int64_t) // activity
			+ sizeof(uint32_t) // fee interest
			+ sizeof(uint32_t) // fee interest denominator
			+ Key_Size; // boot key

		constexpr auto Entry_Size_v5 = sizeof(VersionType) // version
			+ Key_Size // key
			+ Key_Size // owner
			+ sizeof(uint64_t) // disabled height
			+ sizeof(uint64_t) // last signing block height
			+ sizeof(uint64_t) // effective balance
			+ 1 // can harvest
			+ sizeof(int64_t) // activity
			+ sizeof(uint32_t) // fee interest
			+ sizeof(uint32_t) // fee interest denominator
			+ Key_Size // boot key
			+ sizeof(uint64_t); // blockchain version

		auto GetSize(VersionType version) {
			switch (version) {
				case 1: return Entry_Size_v1;
				case 2: return Entry_Size_v2;
				case 3: return Entry_Size_v3;
				case 4: return Entry_Size_v4;
				case 5: return Entry_Size_v5;
				default: return size_t(0u);
			}
		}

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

			if (version > 2) {
				EXPECT_EQ(entry.activity(), *reinterpret_cast<const int64_t*>(pData));
				pData += sizeof(int64_t);
				EXPECT_EQ(entry.feeInterest(), *reinterpret_cast<const uint32_t*>(pData));
				pData += sizeof(uint32_t);
				EXPECT_EQ(entry.feeInterestDenominator(), *reinterpret_cast<const uint32_t*>(pData));
				pData += sizeof(uint32_t);
			}

			if (version > 3) {
				EXPECT_EQ_MEMORY(entry.bootKey().data(), pData, Key_Size);
				pData += Key_Size;
			}

			if (version > 4) {
				EXPECT_EQ(entry.blockchainVersion().unwrap(), *reinterpret_cast<const int64_t*>(pData));
				pData += sizeof(int64_t);
			}

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = test::CreateCommitteeEntry();
			entry.setVersion(version);

			// Act:
			CommitteeEntryPatriciaTreeSerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(GetSize(version), context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), GetSize(version), version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = test::CreateCommitteeEntry();
			entry1.setVersion(version);
			auto entry2 = test::CreateCommitteeEntry();
			entry2.setVersion(version);

			// Act:
			CommitteeEntryPatriciaTreeSerializer::Save(entry1, context.outputStream());
			CommitteeEntryPatriciaTreeSerializer::Save(entry2, context.outputStream());

			// Assert:
			ASSERT_EQ(2 * GetSize(version), context.buffer().size());
			const auto* pBuffer1 = context.buffer().data();
			const auto* pBuffer2 = pBuffer1 + GetSize(version);
			AssertEntryBuffer(entry1, pBuffer1, GetSize(version), version);
			AssertEntryBuffer(entry2, pBuffer2, GetSize(version), version);
		}
	}

	// region Save

	TEST(TEST_CLASS, CanSaveSingleEntry_v1) {
		AssertCanSaveSingleEntry(1);
	}

	TEST(TEST_CLASS, CanSaveSingleEntry_v2) {
		AssertCanSaveSingleEntry(2);
	}

	TEST(TEST_CLASS, CanSaveSingleEntry_v3) {
		AssertCanSaveSingleEntry(3);
	}

	TEST(TEST_CLASS, CanSaveSingleEntry_v4) {
		AssertCanSaveSingleEntry(4);
	}

	TEST(TEST_CLASS, CanSaveSingleEntry_v5) {
		AssertCanSaveSingleEntry(5);
	}

	TEST(TEST_CLASS, CanSaveMultipleEntries_v1) {
		AssertCanSaveMultipleEntries(1);
	}

	TEST(TEST_CLASS, CanSaveMultipleEntries_v2) {
		AssertCanSaveMultipleEntries(2);
	}

	TEST(TEST_CLASS, CanSaveMultipleEntries_v3) {
		AssertCanSaveMultipleEntries(3);
	}

	TEST(TEST_CLASS, CanSaveMultipleEntries_v4) {
		AssertCanSaveMultipleEntries(4);
	}

	TEST(TEST_CLASS, CanSaveMultipleEntries_v5) {
		AssertCanSaveMultipleEntries(5);
	}

	// endregion

	// region Load

	namespace {

		std::vector<uint8_t> CreateEntryBuffer(const state::CommitteeEntry& entry, VersionType version) {
			std::vector<uint8_t> buffer(GetSize(version));

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

			if (version > 2) {
				auto activity = entry.activity();
				memcpy(pData, &activity, sizeof(int64_t));
				pData += sizeof(int64_t);

				auto feeInterest = entry.feeInterest();
				memcpy(pData, &feeInterest, sizeof(uint32_t));
				pData += sizeof(uint32_t);

				auto feeInterestDenominator = entry.feeInterestDenominator();
				memcpy(pData, &feeInterestDenominator, sizeof(uint32_t));
				pData += sizeof(uint32_t);
			}

			if (version > 3) {
				memcpy(pData, entry.bootKey().data(), Key_Size);
				pData += Key_Size;
			}

			if (version > 4) {
				auto blockchainVersion = entry.blockchainVersion().unwrap();
				memcpy(pData, &blockchainVersion, sizeof(uint64_t));
				pData += sizeof(uint64_t);
			}

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = test::CreateCommitteeEntry();
			originalEntry.setVersion(version);
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			auto result = test::CreateCommitteeEntry();
			result.setVersion(version);
			test::RunLoadValueTest<CommitteeEntryPatriciaTreeSerializer>(buffer, result);

			// Assert:
			originalEntry.setActivityObsolete(0.0);
			originalEntry.setGreedObsolete(0.0);
			test::AssertEqualCommitteeEntry(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
		AssertCanLoadSingleEntry(1);
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v2) {
		AssertCanLoadSingleEntry(2);
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v3) {
		AssertCanLoadSingleEntry(3);
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v4) {
		AssertCanLoadSingleEntry(4);
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v5) {
		AssertCanLoadSingleEntry(5);
	}

	// endregion
}}
