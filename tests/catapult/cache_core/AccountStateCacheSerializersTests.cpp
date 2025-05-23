/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "catapult/cache_core/AccountStateCacheSerializers.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS AccountStateCacheSerializersTests

	TEST(TEST_CLASS, KeyAddressPairSerializer_CanSerializeValue) {
		// Arrange:
		auto pair = std::make_pair(test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Address>());

		// Act:
		auto result = KeyAddressPairSerializer::SerializeValue(pair);

		// Assert:
		ASSERT_EQ(sizeof(VersionType) + Key_Size + Address_Decoded_Size, result.size());
		EXPECT_EQ(pair.first, reinterpret_cast<const Key&>(*(result.data() + sizeof(VersionType))));
		EXPECT_EQ(pair.second, reinterpret_cast<const Address&>(*(result.data() + sizeof(VersionType) + Key_Size)));
	}

	TEST(TEST_CLASS, KeyAddressPairSerializer_CanDeserializeValue) {
		// Arrange:
		auto buffer = test::GenerateRandomArray<sizeof(VersionType) + Key_Size + Address_Decoded_Size>();
		VersionType version{1};
		memcpy(buffer.data(), &version, sizeof(VersionType));

		// Act:
		auto pair = KeyAddressPairSerializer::DeserializeValue(buffer);

		// Assert:
		EXPECT_EQ(reinterpret_cast<const Key&>(*(buffer.data() + sizeof(VersionType))), pair.first);
		EXPECT_EQ(reinterpret_cast<const Address&>(*(buffer.data() + sizeof(VersionType) + Key_Size)), pair.second);
	}
}}
