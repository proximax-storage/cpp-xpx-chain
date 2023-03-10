/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"
#include "src/state/DriveContractEntrySerializer.h"

namespace catapult { namespace state {

#define TEST_CLASS DriveContractEntrySerializerTests

	namespace {

		constexpr auto Entry_Size =
				Key_Size + // contract key
				Key_Size + // drive contract key
				sizeof(VersionType);

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

		auto CreateDriveContractEntry() {
			return test::CreateDriveContractEntry();
		}

		void AssertEntryBuffer(const state::DriveContractEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			EXPECT_EQ_MEMORY(entry.key().data(), pData, Key_Size);
			pData += Key_Size;

			EXPECT_EQ_MEMORY(entry.contractKey().data(), pData, Key_Size);
			pData += Key_Size;

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = CreateDriveContractEntry();

			// Act:
			DriveContractEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(Entry_Size, context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = CreateDriveContractEntry();
			auto entry2 = CreateDriveContractEntry();

			// Act:
			DriveContractEntrySerializer::Save(entry1, context.outputStream());
			DriveContractEntrySerializer::Save(entry2, context.outputStream());

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
		std::vector<uint8_t> CreateEntryBuffer(const state::DriveContractEntry& entry, VersionType version) {
			std::vector<uint8_t> buffer(Entry_Size);

			auto* pData = buffer.data();
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			memcpy(pData, entry.key().data(), Key_Size);
			pData += Key_Size;
			memcpy(pData, entry.contractKey().data(), Key_Size);
			pData += Key_Size;   // not sure why it said this value is never used

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = CreateDriveContractEntry();
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			state::DriveContractEntry result(test::GenerateRandomByteArray<Key>());
			test::RunLoadValueTest<DriveContractEntrySerializer>(buffer, result);

			// Assert:
			EXPECT_EQ(originalEntry.key(), result.key());
			EXPECT_EQ(originalEntry.contractKey(), result.contractKey());
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry) {
		AssertCanLoadSingleEntry(1);
	}

	// endregion
}}