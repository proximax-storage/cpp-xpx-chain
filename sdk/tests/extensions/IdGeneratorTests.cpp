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

#include "src/extensions/IdGenerator.h"
#include "plugins/txes/namespace/src/model/NamespaceIdGenerator.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/TestHarness.h"

namespace catapult { namespace extensions {

#define TEST_CLASS IdGeneratorTests

	namespace {
		template<typename TGenerator>
		void AssertDifferentNamesProduceDifferentResults(TGenerator generator) {
			// Assert:
			for (const auto name : { "bloodyrookie.alice", "cat.nekot", "bloodyrookie.token", "bloody_rookie.token" })
				EXPECT_NE(generator("cat.token"), generator(name)) << "cat.token vs " << name;
		}

		template<typename TGenerator>
		void AssertNamesWithUppercaseCharactersAreRejected(TGenerator generator) {
			// Act + Assert:
			for (const auto name : { "CAT.token", "CAT.TOKEN", "cat.TOKEN", "cAt.ToKeN", "CaT.tOkEn" }) {
				EXPECT_THROW(generator(name), catapult_invalid_argument) << "name " << name;
			}
		}

		template<typename TGenerator>
		void AssertImproperQualifiedNamesAreRejected(TGenerator generator) {
			// Act + Assert:
			for (const auto name : { ".", "..", "...", ".a", "b.", "a..b", ".a.b", "b.a." }) {
				EXPECT_THROW(generator(name), catapult_invalid_argument) << "name " << name;
			}
		}

		template<typename TGenerator>
		void AssertImproperPartNamesAreRejected(TGenerator generator) {
			// Act + Assert:
			for (const auto name : { "alpha.bet@.zeta", "a!pha.beta.zeta", "alpha.beta.ze^a" }) {
				EXPECT_THROW(generator(name), catapult_invalid_argument) << "name " << name;
			}
		}

		template<typename TGenerator>
		void AssertEmptyStringIsRejected(TGenerator generator) {
			// Act + Assert:
			EXPECT_THROW(generator(""), catapult_invalid_argument) << "empty string";
		}

#define ADD_BASIC_TESTS(PREFIX, GENERATOR) \
		TEST(TEST_CLASS, PREFIX##_DifferentNamesProduceDifferentResults) { AssertDifferentNamesProduceDifferentResults(GENERATOR); } \
		TEST(TEST_CLASS, PREFIX##_RejectsNamesWithUppercaseCharacters) { AssertNamesWithUppercaseCharactersAreRejected(GENERATOR); } \
		TEST(TEST_CLASS, PREFIX##_RejectsImproperQualifiedNames) { AssertImproperQualifiedNamesAreRejected(GENERATOR); } \
		TEST(TEST_CLASS, PREFIX##_RejectsImproperPartNames) { AssertImproperPartNamesAreRejected(GENERATOR); } \
		TEST(TEST_CLASS, PREFIX##_RejectsEmptyString) { AssertEmptyStringIsRejected(GENERATOR); }
	}

	// region GenerateMosaicAliasId

	TEST(TEST_CLASS, GenerateMosaicAliasId_GeneratesCorrectWellKnownId) {
		// Arrange:
		constexpr auto Known_Mosaic_Alias_Id = UnresolvedMosaicId(0xA029'E100'621B'2E33ULL);

		// Assert:
		EXPECT_EQ(Known_Mosaic_Alias_Id, GenerateMosaicAliasId("cat.token"));
	}

	TEST(TEST_CLASS, GenerateMosaicAliasId_SupportsMultiLevelMosaics) {
		// Arrange:
		auto expected = model::GenerateNamespaceId(model::GenerateNamespaceId(model::GenerateRootNamespaceId("foo"), "bar"), "baz");

		// Assert:
		EXPECT_EQ(UnresolvedMosaicId(expected.unwrap()), GenerateMosaicAliasId("foo.bar.baz"));
	}

	TEST(TEST_CLASS, GenerateMosaicAliasId_CanGenerateIdFromTopLevelNamespace) {
		// Act + Assert:
		for (const auto name : { "cat", "token", "alpha" })
			EXPECT_NE(UnresolvedMosaicId(), GenerateMosaicAliasId(name)) << "name " << name;
	}

	TEST(TEST_CLASS, GenerateMosaicAliasId_RejectsNamesWithTooManyParts) {
		// Act + Assert:
		for (const auto name : { "a.b.c.d", "a.b.c.d.e.f" }) {
			EXPECT_THROW(GenerateMosaicAliasId(name), catapult_invalid_argument) << "name " << name;
		}
	}

	namespace {
		UnresolvedMosaicId GenerateMosaicIdAdapter(const RawString& namespaceName) {
			// Act:
			return GenerateMosaicAliasId(namespaceName);
		}
	}

	ADD_BASIC_TESTS(GenerateMosaicId, GenerateMosaicIdAdapter)

	// endregion

	// region GenerateNamespacePath

	TEST(TEST_CLASS, GenerateNamespacePath_GeneratesCorrectWellKnownRootPath) {
		// Act:
		auto path = GenerateNamespacePath("cat");

		// Assert:
		EXPECT_EQ(1u, path.size());
		EXPECT_EQ(NamespaceId(test::Default_Namespace_Id), path[0]);
	}

	TEST(TEST_CLASS, GenerateNamespacePath_GeneratesCorrectWellKnownNamespace) {
		// Arrange:
		constexpr auto Known_Namespace_Id = NamespaceId(0xA029'E100'621B'2E33ULL);

		// Act:
		auto path = GenerateNamespacePath("cat.token");

		// Assert:
		EXPECT_EQ(2u, path.size());
		EXPECT_EQ(NamespaceId(test::Default_Namespace_Id), path[0]);
		EXPECT_EQ(Known_Namespace_Id, path[1]);
	}

	TEST(TEST_CLASS, GenerateNamespacePath_SupportsMultiLevelNamespaces) {
		// Arrange:
		NamespacePath expected;
		expected.push_back(model::GenerateRootNamespaceId("foo"));
		expected.push_back(model::GenerateNamespaceId(expected[0], "bar"));
		expected.push_back(model::GenerateNamespaceId(expected[1], "baz"));

		// Assert:
		EXPECT_EQ(expected, GenerateNamespacePath("foo.bar.baz"));
	}

	TEST(TEST_CLASS, GenerateNamespacePath_RejectsNamesWithTooManyParts) {
		// Act + Assert:
		for (const auto name : { "a.b.c.d", "a.b.c.d.e" }) {
			EXPECT_THROW(GenerateNamespacePath(name), catapult_invalid_argument) << "name " << name;
		}
	}

	ADD_BASIC_TESTS(GenerateNamespacePath, GenerateNamespacePath)

	// endregion
}}
