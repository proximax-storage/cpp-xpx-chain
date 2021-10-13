/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/StorageTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS DownloadChannelEntrySerializerTests

	namespace {

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
        
        auto CreateDownloadChannelEntry() {
            return test::CreateDownloadChannelEntry();
        }

		void AssertListOfPublicKeysBuffer(const std::vector<Key>& expected, const uint8_t*& pData) {
			uint16_t size = *reinterpret_cast<const uint16_t*>(pData);
			pData += sizeof(size);
			EXPECT_EQ(expected.size(), size);
			for (const auto& key: expected) {
				EXPECT_EQ_MEMORY(key.data(), pData, Key_Size);
				pData += Key_Size;
			}
		}

		void AssertCumulativePaymentsBuffer(const std::map<Key, Amount>& expected, const uint8_t*& pData) {
			uint16_t size = *reinterpret_cast<const uint16_t*>(pData);
			pData += sizeof(size);
			EXPECT_EQ(expected.size(), size);
			for (const auto& pair: expected) {
				EXPECT_EQ_MEMORY(pair.first.data(), pData, Key_Size);
				pData += Key_Size;
				uint64_t amount = *reinterpret_cast<const uint64_t*>(pData);
				pData += sizeof(amount);
				EXPECT_EQ(pair.second.unwrap(), amount);
			}
		}

        void AssertEntryBuffer(const state::DownloadChannelEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
            const auto* pExpectedEnd = pData + expectedSize;
            EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
            EXPECT_EQ_MEMORY(entry.id().data(), pData, Hash256_Size);
			pData += Hash256_Size;
            EXPECT_EQ_MEMORY(entry.consumer().data(), pData, Key_Size);
            pData += Key_Size;
			uint64_t downloadSize = *reinterpret_cast<const uint64_t*>(pData);
			pData += sizeof(downloadSize);
			EXPECT_EQ(entry.downloadSize(), downloadSize);
			uint16_t downloadApprovalCount = *reinterpret_cast<const uint16_t*>(pData);
			pData += sizeof(downloadApprovalCount);
			EXPECT_EQ(entry.downloadApprovalCount(), downloadApprovalCount);

			AssertListOfPublicKeysBuffer(entry.listOfPublicKeys(), pData);
			AssertCumulativePaymentsBuffer(entry.cumulativePayments(), pData);

            EXPECT_EQ(pExpectedEnd, pData);
        }

        void AssertCanSaveSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto entry = CreateDownloadChannelEntry();

            // Act:
            DownloadChannelEntrySerializer::Save(entry, context.outputStream());

            // Assert:
//            ASSERT_EQ(Entry_Size, context.buffer().size());
            AssertEntryBuffer(entry, context.buffer().data(), context.buffer().size(), version);
        }

        void AssertCanSaveMultipleEntries(VersionType version) {
            // Arrange:
            TestContext context;
            auto entry1 = CreateDownloadChannelEntry();
            auto entry2 = CreateDownloadChannelEntry();

            // Act:
            DownloadChannelEntrySerializer::Save(entry1, context.outputStream());
			auto entryBufferSize1 = context.buffer().size();
            DownloadChannelEntrySerializer::Save(entry2, context.outputStream());
			auto entryBufferSize2 = context.buffer().size() - entryBufferSize1;

            // Assert:
            const auto* pBuffer1 = context.buffer().data();
            const auto* pBuffer2 = pBuffer1 + entryBufferSize1;
            AssertEntryBuffer(entry1, pBuffer1, entryBufferSize1, version);
            AssertEntryBuffer(entry2, pBuffer2, entryBufferSize2, version);
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

		void CopyToVector(std::vector<uint8_t>& data, const uint8_t * p, size_t bytes) {
			data.insert(data.end(), p, p + bytes);
		}

		void SaveListOfPublicKeys(const std::vector<Key>& listOfPublicKeys, std::vector<uint8_t>& buffer) {
			auto publicKeysSize = listOfPublicKeys.size();
			CopyToVector(buffer, (const uint8_t*) &publicKeysSize, sizeof(uint16_t));
			for (const auto& publicKey: listOfPublicKeys) {
				CopyToVector(buffer, publicKey.data(), Key_Size);
			}
		}

		void SaveCumulativePayments(const std::map<Key, Amount>& cumulativePayments,
									std::vector<uint8_t>& buffer) {
			auto cumulativePaymentsSize = cumulativePayments.size();
			CopyToVector(buffer, (const uint8_t*) &cumulativePaymentsSize, sizeof(uint16_t));
			for (const auto& pair: cumulativePayments) {
				CopyToVector(buffer, pair.first.data(), Key_Size);
				CopyToVector(buffer, (const uint8_t*) &pair.second, sizeof(Amount));
			}
		}

        std::vector<uint8_t> CreateEntryBuffer(const state::DownloadChannelEntry& entry, VersionType version) {
            std::vector<uint8_t> buffer;

			CopyToVector(buffer, (const uint8_t*) &version, sizeof(VersionType));
			CopyToVector(buffer, entry.id().data(), Hash256_Size);
			CopyToVector(buffer, entry.consumer().data(), Key_Size);
			CopyToVector(buffer, (const uint8_t*) &entry.downloadSize(), sizeof(uint64_t));
			CopyToVector(buffer, (const uint8_t*) &entry.downloadApprovalCount(), sizeof(uint16_t));

			SaveListOfPublicKeys(entry.listOfPublicKeys(), buffer);
			SaveCumulativePayments(entry.cumulativePayments(), buffer);

            return buffer;
        }

        void AssertCanLoadSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto originalEntry = CreateDownloadChannelEntry();
            auto buffer = CreateEntryBuffer(originalEntry, version);

            // Act:
            state::DownloadChannelEntry result(test::GenerateRandomByteArray<Hash256>());
            test::RunLoadValueTest<DownloadChannelEntrySerializer>(buffer, result);

            // Assert:
            test::AssertEqualDownloadChannelData(originalEntry, result);
        }
    }

    TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
        AssertCanLoadSingleEntry(1);
    }

    // endregion
}}