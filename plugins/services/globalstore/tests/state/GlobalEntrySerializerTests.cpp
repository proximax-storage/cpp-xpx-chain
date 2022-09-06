/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#include "src/state/GlobalEntrySerializer.h"
#include "catapult/model/EntityType.h"
#include "tests/test/GlobalStoreTestUtils.h"
#include "tests/test/core/SerializerOrderingTests.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS GlobalEntrySerializerTests

	namespace {

		auto CalculateExpectedSize(const GlobalEntry& entry) {
			return sizeof(uint32_t)*2 + Hash256::Size + 30;
		}

		void AssertBuffer(const GlobalEntry& entry, const std::vector<uint8_t>& buffer, size_t expectedSize) {
			ASSERT_EQ(expectedSize, buffer.size());
			test::BufferReader reader(buffer);
			EXPECT_EQ(1, reader.read<uint32_t>());
			EXPECT_EQ(entry.GetKey(), reader.read<Hash256>());
			EXPECT_EQ(30, reader.read<uint32_t>());
			auto array = reader.read<std::array<uint8_t, 30>>();
			EXPECT_EQ_MEMORY(entry.Ref().data(), array.data(), 30);
		}

		void AssertCanSaveGlobalEntry() {
			// Arrange:
			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream outputStream(buffer);
			auto entry = test::CreateGlobalEntry(test::GenerateRandomByteArray<Hash256>());

			// Act:
			GlobalEntrySerializer::Save(entry, outputStream);

			// Assert:
			AssertBuffer(entry, buffer, CalculateExpectedSize(entry));
		}
	}

	// region Save

	TEST(TEST_CLASS, CanSaveGlobalEntry) {
		AssertCanSaveGlobalEntry();
	}

	// region Roundtrip

	namespace {
		void AssertCanRoundtripGlobalEntry() {
			auto originalEntry = test::CreateGlobalEntry(test::GenerateRandomByteArray<Hash256>());

			// Act:
			auto result = test::RunRoundtripBufferTest<GlobalEntrySerializer>(originalEntry);

			// Assert:
			test::AssertEqual(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanRoundtripGlobalEntry) {
		AssertCanRoundtripGlobalEntry();
	}

	// endregion
}}
