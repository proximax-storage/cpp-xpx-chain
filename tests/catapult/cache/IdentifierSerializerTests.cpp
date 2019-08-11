/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache/IdentifierSerializer.h"
#include "catapult/utils/Hashers.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS IdentifierSerializerTests

	namespace {
		struct HeightBasedDescriptor {
		public:
			using ValueType = Height;
		};

		using ValueType = HeightBasedDescriptor::ValueType;
		using Serializer = IdentifierSerializer<HeightBasedDescriptor>;

		template<typename TContainer>
		RawBuffer ToRawBuffer(const TContainer& container) {
			return { reinterpret_cast<const uint8_t*>(container.data()), container.size() * sizeof(typename TContainer::value_type) };
		}
	}

	TEST(TEST_CLASS, CanSerialize) {
		// Arrange:
		auto originalValue = ValueType(5);

		// Act:
		auto result = Serializer::SerializeValue(originalValue);

		// Assert:
		auto expectedSize = sizeof(VersionType) + sizeof(Height);
		ASSERT_EQ(expectedSize, result.size());

		auto version = *reinterpret_cast<const VersionType*>(reinterpret_cast<const uint8_t*>(result.data()));
		EXPECT_EQ(1, version);

		auto value = *reinterpret_cast<const ValueType*>(reinterpret_cast<const uint8_t*>(result.data()) + sizeof(VersionType));
		EXPECT_EQ(originalValue, value);
	}

	TEST(TEST_CLASS, DeserializeThrowsWhenVersionInvalid) {
		// Arrange:
		std::vector<uint32_t> buffer{ std::numeric_limits<uint32_t>::max(), 0x00, 0x00 };

		// Act + Assert:
		EXPECT_THROW(Serializer::DeserializeValue(ToRawBuffer(buffer)), catapult_runtime_error);
	}

	TEST(TEST_CLASS, CanDeserialize) {
		// Arrange:
		std::vector<uint32_t> buffer{ 0x01, 0x02, 0x00 };

		// Act:
		auto value = Serializer::DeserializeValue(ToRawBuffer(buffer));

		// Assert:
		EXPECT_EQ(Height{2}, value);
	}
}}
