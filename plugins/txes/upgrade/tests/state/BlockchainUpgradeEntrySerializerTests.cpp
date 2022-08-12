/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/BlockchainUpgradeEntrySerializer.h"
#include "tests/test/core/SerializerTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS BlockchainUpgradeEntrySerializerTests

	namespace {
		constexpr auto Entry_Size = sizeof(VersionType) + 2 * sizeof(uint64_t);

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

		void AssertEntryBuffer(const state::BlockchainUpgradeEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			EXPECT_EQ(entry.height().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.blockChainVersion().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertEqual(const state::BlockchainUpgradeEntry& expectedEntry, const state::BlockchainUpgradeEntry& entry) {
			EXPECT_EQ(expectedEntry.height(), entry.height());
			EXPECT_EQ(expectedEntry.blockChainVersion(), entry.blockChainVersion());
		}

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = state::BlockchainUpgradeEntry(Height{1}, BlockchainVersion{2});

			// Act:
			BlockchainUpgradeEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(Entry_Size, context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = state::BlockchainUpgradeEntry(Height{1}, BlockchainVersion{3});
			auto entry2 = state::BlockchainUpgradeEntry(Height{1}, BlockchainVersion{4});

			// Act:
			BlockchainUpgradeEntrySerializer::Save(entry1, context.outputStream());
			BlockchainUpgradeEntrySerializer::Save(entry2, context.outputStream());

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

		std::vector<uint8_t> CreateEntryBuffer(const state::BlockchainUpgradeEntry& entry, VersionType version) {
			std::vector<uint8_t> buffer(Entry_Size);

			auto* pData = buffer.data();
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			auto height = entry.height().unwrap();
			memcpy(pData, &height, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			auto blockChainVersion = entry.blockChainVersion().unwrap();
			memcpy(pData, &blockChainVersion, sizeof(uint64_t));
			pData += sizeof(uint64_t);

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = state::BlockchainUpgradeEntry(Height{1}, BlockchainVersion{5});
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			state::BlockchainUpgradeEntry result;
			test::RunLoadValueTest<BlockchainUpgradeEntrySerializer>(buffer, result);

			// Assert:
			AssertEqual(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
		AssertCanLoadSingleEntry(1);
	}

	// endregion
}}
