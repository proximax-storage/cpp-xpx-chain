/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/DownloadEntrySerializer.h"
#include "catapult/utils/HexFormatter.h"
#include "catapult/utils/Casting.h"
#include "tests/test/core/SerializerOrderingTests.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS DownloadEntrySerializerTests

	namespace {
		constexpr auto File_Count = 4;

		constexpr auto Entry_Size =
			sizeof(VersionType) + // version
			Hash256_Size + // operation token
			Key_Size + // drive key
			Key_Size + // file recipient key
			sizeof(Height) + // end
			sizeof(uint16_t) + // file count
			File_Count * Hash256_Size; // file hashes

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

		auto CreateDownloadEntry() {
			return test::CreateDownloadEntry(
				test::GenerateRandomByteArray<Hash256>(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomValue<Height>(),
				File_Count);
		}

		void AssertEntryBuffer(const state::DownloadEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			EXPECT_EQ_MEMORY(entry.OperationToken.data(), pData, Hash256_Size);
			pData += Hash256_Size;
			EXPECT_EQ_MEMORY(entry.DriveKey.data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ_MEMORY(entry.FileRecipient.data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ(entry.Height.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);

			EXPECT_EQ(entry.Files.size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& fileHash : entry.Files) {
				EXPECT_EQ_MEMORY(fileHash.data(), pData, Hash256_Size);
				pData += Hash256_Size;
			}

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = CreateDownloadEntry();

			// Act:
			DownloadEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(Entry_Size, context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = CreateDownloadEntry();
			auto entry2 = CreateDownloadEntry();

			// Act:
			DownloadEntrySerializer::Save(entry1, context.outputStream());
			DownloadEntrySerializer::Save(entry2, context.outputStream());

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
		std::vector<uint8_t> CreateEntryBuffer(const state::DownloadEntry& entry, VersionType version) {
			std::vector<uint8_t> buffer(Entry_Size);

			auto* pData = buffer.data();
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			memcpy(pData, entry.OperationToken.data(), Hash256_Size);
			pData += Hash256_Size;
			memcpy(pData, entry.DriveKey.data(), Key_Size);
			pData += Key_Size;
			memcpy(pData, entry.FileRecipient.data(), Key_Size);
			pData += Key_Size;
			memcpy(pData, &entry.Height, sizeof(uint64_t));
			pData += sizeof(uint64_t);

			auto fileCount = utils::checked_cast<size_t, uint16_t>(entry.Files.size());
			memcpy(pData, &fileCount, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			for (const auto& fileHash : entry.Files) {
				memcpy(pData, fileHash.data(), Hash256_Size);
				pData += Hash256_Size;
			}

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = CreateDownloadEntry();
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			state::DownloadEntry result;
			test::RunLoadValueTest<DownloadEntrySerializer>(buffer, result);

			// Assert:
			test::AssertEqualDownloadData(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
		AssertCanLoadSingleEntry(1);
	}

	// endregion
}}
