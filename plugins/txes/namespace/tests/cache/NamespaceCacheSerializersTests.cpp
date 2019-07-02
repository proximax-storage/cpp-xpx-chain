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

#include "src/cache/NamespaceCacheSerializers.h"
#include "tests/test/NamespaceTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS NamespaceCacheSerializersTests

	namespace {
		using Serializer = NamespaceFlatMapTypesSerializer;
	}

	TEST(TEST_CLASS, FlatMapTypesSerializer_CanSerializePartialValue) {
		// Arrange:
		auto ns = state::Namespace(test::CreatePath({ 11 }));

		// Act:
		auto result = Serializer::SerializeValue(ns);

		// Assert:
		ASSERT_EQ(sizeof(VersionType) + sizeof(uint64_t) + sizeof(NamespaceId), result.size());

		const auto* pValues = reinterpret_cast<const uint64_t*>(result.data() + sizeof(VersionType));
		EXPECT_EQ(1u, pValues[0]);
		EXPECT_EQ(11u, pValues[1]);
	}

	TEST(TEST_CLASS, FlatMapTypesSerializer_CanSerializeFullValue) {
		// Arrange:
		auto ns = state::Namespace(test::CreatePath({ 11, 7, 21 }));

		// Act:
		auto result = Serializer::SerializeValue(ns);

		// Assert:
		ASSERT_EQ(sizeof(VersionType) + sizeof(uint64_t) + 3 * sizeof(NamespaceId), result.size());

		const auto* pValues = reinterpret_cast<const uint64_t*>(result.data() + sizeof(VersionType));
		EXPECT_EQ(3u, pValues[0]);
		EXPECT_EQ(11u, pValues[1]);
		EXPECT_EQ(7u, pValues[2]);
		EXPECT_EQ(21u, pValues[3]);
	}

	namespace {
		void AssertFlatMapTypesSerializer_CanDeserializePartialValue(VersionType version) {
			// Arrange:
			auto buffer = std::vector<uint64_t>{ 0, 1, 11 };
			auto* pData = reinterpret_cast<uint8_t*>(buffer.data()) + sizeof(uint64_t) - sizeof(VersionType);
			memcpy(pData, &version, sizeof(VersionType));

			// Act:
			auto ns = Serializer::DeserializeValue({ pData, sizeof(VersionType) + (buffer.size() - 1) * sizeof(uint64_t) });

			// Assert:
			ASSERT_EQ(1u, ns.path().size());
			EXPECT_EQ(NamespaceId(11), ns.path()[0]);
		}

		void AssertFlatMapTypesSerializer_CanDeserializeFullValue(VersionType version) {
			// Arrange:
			auto buffer = std::vector<uint64_t>{ 0, 3, 11, 7, 21 };
			auto* pData = reinterpret_cast<uint8_t*>(buffer.data()) + sizeof(uint64_t) - sizeof(VersionType);
			memcpy(pData, &version, sizeof(VersionType));

			// Act:
			auto ns = Serializer::DeserializeValue({ pData, sizeof(VersionType) + (buffer.size() - 1) * sizeof(uint64_t) });

			// Assert:
			ASSERT_EQ(3u, ns.path().size());
			EXPECT_EQ(NamespaceId(11), ns.path()[0]);
			EXPECT_EQ(NamespaceId(7), ns.path()[1]);
			EXPECT_EQ(NamespaceId(21), ns.path()[2]);
		}
	}

	TEST(TEST_CLASS, FlatMapTypesSerializer_CanDeserializePartialValue_v1) {
		AssertFlatMapTypesSerializer_CanDeserializePartialValue(1);
	}

	TEST(TEST_CLASS, FlatMapTypesSerializer_CanDeserializeFullValue_v1) {
		AssertFlatMapTypesSerializer_CanDeserializeFullValue(1);
	}
}}
