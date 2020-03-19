/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/SuperContractEntrySerializer.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS SuperContractEntrySerializerTests

	namespace {
		constexpr auto Entry_Size =
			sizeof(VersionType) + // version
			Key_Size + // super contract key
			sizeof(uint8_t) + // state
			Key_Size + // owner
			sizeof(Height) + // start
			sizeof(Height) + // end
			Key_Size + // main drive key
			Hash256_Size + // file hash
			sizeof(VmVersion) + // VM version
			sizeof(uint16_t); // execution count

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

		void AssertEntryBuffer(const state::SuperContractEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			EXPECT_EQ_MEMORY(entry.key().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ(entry.state(), static_cast<SuperContractState>(*pData));
			pData++;
			EXPECT_EQ_MEMORY(entry.owner().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ(entry.start().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.end().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ_MEMORY(entry.mainDriveKey().data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ_MEMORY(entry.fileHash().data(), pData, Hash256_Size);
			pData += Hash256_Size;
			EXPECT_EQ(entry.vmVersion().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.executionCount(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = test::CreateSuperContractEntry();

			// Act:
			SuperContractEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(Entry_Size, context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = test::CreateSuperContractEntry();
			auto entry2 = test::CreateSuperContractEntry();

			// Act:
			SuperContractEntrySerializer::Save(entry1, context.outputStream());
			SuperContractEntrySerializer::Save(entry2, context.outputStream());

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
		std::vector<uint8_t> CreateEntryBuffer(const state::SuperContractEntry& entry, VersionType version) {
			std::vector<uint8_t> buffer(Entry_Size);

			auto* pData = buffer.data();
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			memcpy(pData, entry.key().data(), Key_Size);
			pData += Key_Size;
			*pData = utils::to_underlying_type(entry.state());
			pData++;
			memcpy(pData, entry.owner().data(), Key_Size);
			pData += Key_Size;
			auto start = entry.start();
			memcpy(pData, &start, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			auto end = entry.end();
			memcpy(pData, &end, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			memcpy(pData, entry.mainDriveKey().data(), Key_Size);
			pData += Key_Size;
			memcpy(pData, entry.fileHash().data(), Hash256_Size);
			pData += Hash256_Size;
			auto vmVersion = entry.vmVersion();
			memcpy(pData, &vmVersion, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			auto executionCount = entry.executionCount();
			memcpy(pData, &executionCount, sizeof(uint16_t));
			pData += sizeof(uint16_t);

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = test::CreateSuperContractEntry();
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			auto result = state::SuperContractEntry(Key());
			test::RunLoadValueTest<SuperContractEntrySerializer>(buffer, result);

			// Assert:
			test::AssertEqualSuperContractData(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
		AssertCanLoadSingleEntry(1);
	}

	// endregion
}}
