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
		constexpr auto File_Recipient_Count = 2;
		constexpr auto Download_Count = 3;
		constexpr auto File_Count = 4;

		constexpr auto Entry_Size =
			sizeof(VersionType) + // version
			Key_Size + // drive key
			sizeof(uint32_t) + // file recipient count
			File_Recipient_Count * Key_Size + // file recipient key
			File_Recipient_Count * sizeof(uint32_t) + // download count
			File_Recipient_Count * Download_Count * Hash256_Size + // operation token
			File_Recipient_Count * Download_Count * sizeof(uint16_t) + // file count
			File_Recipient_Count * Download_Count * File_Count * Hash256_Size; // file hashes

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
				test::GenerateRandomByteArray<Key>(),
				File_Recipient_Count,
				Download_Count,
				File_Count);
		}

		void AssertEntryBuffer(const state::DownloadEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			EXPECT_EQ_MEMORY(entry.driveKey().data(), pData, Key_Size);
			pData += Key_Size;

			EXPECT_EQ(entry.fileRecipients().size(), *reinterpret_cast<const uint32_t*>(pData));
			pData += sizeof(uint32_t);
			for (const auto& fileRecipientPair : entry.fileRecipients()) {
				EXPECT_EQ_MEMORY(fileRecipientPair.first.data(), pData, Key_Size);
				pData += Key_Size;
				const auto& downloads = fileRecipientPair.second;
				EXPECT_EQ(downloads.size(), *reinterpret_cast<const uint32_t*>(pData));
				pData += sizeof(uint32_t);
				for (const auto& downloadPair : downloads) {
					EXPECT_EQ_MEMORY(downloadPair.first.data(), pData, Hash256_Size);
					pData += Hash256_Size;
					const auto& fileHashes = downloadPair.second;
					EXPECT_EQ(fileHashes.size(), *reinterpret_cast<const uint16_t*>(pData));
					pData += sizeof(uint16_t);
					for (const auto& fileHash : fileHashes) {
						EXPECT_EQ_MEMORY(fileHash.data(), pData, Hash256_Size);
						pData += Hash256_Size;
					}
				}
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
			memcpy(pData, entry.driveKey().data(), Key_Size);
			pData += Key_Size;

			auto fileRecipientCount = utils::checked_cast<size_t, uint32_t>(entry.fileRecipients().size());
			memcpy(pData, &fileRecipientCount, sizeof(uint32_t));
			pData += sizeof(uint32_t);
			for (const auto& fileRecipientPair : entry.fileRecipients()) {
				memcpy(pData, fileRecipientPair.first.data(), Key_Size);
				pData += Key_Size;
				const auto& downloads = fileRecipientPair.second;
				auto downloadCount = utils::checked_cast<size_t, uint32_t>(downloads.size());
				memcpy(pData, &downloadCount, sizeof(uint32_t));
				pData += sizeof(uint32_t);
				for (const auto& downloadPair : downloads) {
					memcpy(pData, downloadPair.first.data(), Hash256_Size);
					pData += Hash256_Size;
					const auto& fileHashes = downloadPair.second;
					auto fileCount = utils::checked_cast<size_t, uint16_t>(fileHashes.size());
					memcpy(pData, &fileCount, sizeof(uint16_t));
					pData += sizeof(uint16_t);
					for (const auto& fileHash : fileHashes) {
						memcpy(pData, fileHash.data(), Hash256_Size);
						pData += Hash256_Size;
					}
				}
			}

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = CreateDownloadEntry();
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			state::DownloadEntry result(test::GenerateRandomByteArray<Key>());
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
