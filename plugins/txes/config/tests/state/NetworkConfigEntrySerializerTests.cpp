/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/NetworkConfigEntrySerializer.h"
#include "tests/test/core/SerializerTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS NetworkConfigEntrySerializerTests

	namespace {
#define ENTRY_SIZE(BLOCKCHAIN_CONFIG_SIZE, SUPPORTED_VERSIONS_CONFIG_SIZE) \
		(sizeof(VersionType) + sizeof(uint64_t) + sizeof(uint16_t) + BLOCKCHAIN_CONFIG_SIZE + sizeof(uint16_t) + SUPPORTED_VERSIONS_CONFIG_SIZE)

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

		void AssertEntryBuffer(const state::NetworkConfigEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
			const auto* pExpectedEnd = pData + expectedSize;
			EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			EXPECT_EQ(entry.height().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);

			auto networkConfigSize = *reinterpret_cast<const uint16_t*>(pData);
			pData += sizeof(uint16_t);
			EXPECT_EQ_MEMORY(reinterpret_cast<const uint8_t*>(entry.networkConfig().data()), pData, networkConfigSize);
			pData += networkConfigSize;

			auto supportedEntityVersionsSize = *reinterpret_cast<const uint16_t*>(pData);
			pData += sizeof(uint16_t);
			EXPECT_EQ_MEMORY(reinterpret_cast<const uint8_t*>(entry.supportedEntityVersions().data()), pData, supportedEntityVersionsSize);
			pData += supportedEntityVersionsSize;

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertEqual(const state::NetworkConfigEntry& expectedEntry, const state::NetworkConfigEntry& entry) {
			EXPECT_EQ(expectedEntry.height(), entry.height());
			EXPECT_EQ(expectedEntry.networkConfig(), entry.networkConfig());
			EXPECT_EQ(expectedEntry.supportedEntityVersions(), entry.supportedEntityVersions());
		}

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = state::NetworkConfigEntry(Height{1}, "a", "bb");
			auto entrySize = ENTRY_SIZE(1, 2);

			// Act:
			NetworkConfigEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(entrySize, context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), entrySize, version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = state::NetworkConfigEntry(Height{1}, "aa", "bbb");
			auto entrySize1 = ENTRY_SIZE(2, 3);
			auto entry2 = state::NetworkConfigEntry(Height{1}, "cccc", "ddddd");
			auto entrySize2 = ENTRY_SIZE(4, 5);

			// Act:
			NetworkConfigEntrySerializer::Save(entry1, context.outputStream());
			NetworkConfigEntrySerializer::Save(entry2, context.outputStream());

			// Assert:
			ASSERT_EQ(entrySize1 + entrySize2, context.buffer().size());
			const auto* pBuffer1 = context.buffer().data();
			const auto* pBuffer2 = pBuffer1 + entrySize1;
			AssertEntryBuffer(entry1, pBuffer1, entrySize1, version);
			AssertEntryBuffer(entry2, pBuffer2, entrySize2, version);
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

		std::vector<uint8_t> CreateEntryBuffer(const state::NetworkConfigEntry& entry, VersionType version) {
			auto entrySize = ENTRY_SIZE(entry.networkConfig().size(), entry.supportedEntityVersions().size());
			std::vector<uint8_t> buffer(entrySize);

			auto* pData = buffer.data();
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			auto height = entry.height().unwrap();
			memcpy(pData, &height, sizeof(uint64_t));
			pData += sizeof(uint64_t);

			auto networkConfigSize = entry.networkConfig().size();
			memcpy(pData, &networkConfigSize, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			memcpy(pData, entry.networkConfig().data(), networkConfigSize);
			pData += networkConfigSize;

			auto supportedEntityVersionsSize = entry.supportedEntityVersions().size();
			memcpy(pData, &supportedEntityVersionsSize, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			memcpy(pData, entry.supportedEntityVersions().data(), supportedEntityVersionsSize);
			pData += supportedEntityVersionsSize;

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = state::NetworkConfigEntry(Height{1}, "aa", "bbb");
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			state::NetworkConfigEntry result;
			test::RunLoadValueTest<NetworkConfigEntrySerializer>(buffer, result);

			// Assert:
			AssertEqual(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
		AssertCanLoadSingleEntry(1);
	}

	// endregion
}}
