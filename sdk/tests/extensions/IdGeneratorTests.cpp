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

#include "catapult/crypto/IdGenerator.h"
#include "src/extensions/IdGenerator.h"
#include "catapult/constants.h"
#include "tests/TestHarness.h"

namespace catapult { namespace extensions {

#define TEST_CLASS IdGeneratorTests

	namespace {
		template<typename TGenerator>
		void AssertDifferentNamesProduceDifferentResults(TGenerator generator) {
			// Assert:
			for (const auto name : { "bloodyrookie.alice", "prx.pxx", "bloodyrookie.xpx", "bloody_rookie.xpx" })
				EXPECT_NE(generator("prx.xpx"), generator(name)) << "prx.xpx vs " << name;
		}

		template<typename TGenerator>
		void AssertNamesWithUppercaseCharactersAreRejected(TGenerator generator) {
			// Act + Assert:
			for (const auto name : { "PRX.xpx", "PRX.XPX", "prx.XPX", "pRx.XpX", "PrX.xPx" }) {
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
		TEST(TEST_CLASS, PREFIX##_DifferentNamesProduceDifferentResults) { \
			AssertDifferentNamesProduceDifferentResults(GENERATOR); \
		} \
		TEST(TEST_CLASS, PREFIX##_RejectsNamesWithUppercaseCharacters) { \
			AssertNamesWithUppercaseCharactersAreRejected(GENERATOR); \
		} \
		TEST(TEST_CLASS, PREFIX##_RejectsImproperQualifiedNames) { \
			AssertImproperQualifiedNamesAreRejected(GENERATOR); \
		} \
		TEST(TEST_CLASS, PREFIX##_RejectsImproperPartNames) { \
			AssertImproperPartNamesAreRejected(GENERATOR); \
		} \
		TEST(TEST_CLASS, PREFIX##_RejectsEmptyString) { \
			AssertEmptyStringIsRejected(GENERATOR); \
		}
	}

	// region GenerateMosaicId

	TEST(TEST_CLASS, GenerateMosaicId_GeneratesCorrectWellKnownId) {
		// Assert:
		EXPECT_EQ(Xpx_Id, GenerateMosaicId("prx:xpx"));
	}

	TEST(TEST_CLASS, GenerateMosaicId_SupportsMultiLevelMosaics) {
		// Arrange:
		auto expected = crypto::GenerateMosaicId(
				crypto::GenerateNamespaceId(crypto::GenerateNamespaceId(crypto::GenerateRootNamespaceId("foo"), "bar"), "baz"),
				"tokens");

		// Assert:
		EXPECT_EQ(expected, GenerateMosaicId("foo.bar.baz:tokens"));
	}

	TEST(TEST_CLASS, GenerateMosaicId_RejectsNamespaceOnlyNames) {
		// Act + Assert:
		for (const auto name : { "bloodyrookie.alice", "prx.xxp", "bloodyrookie.xpx", "bloody_rookie.xpx" }) {
			EXPECT_THROW(GenerateMosaicId(name), catapult_invalid_argument) << "name " << name;
		}
	}

	TEST(TEST_CLASS, GenerateMosaicId_RejectsMosaicOnlyNames) {
		// Act + Assert:
		for (const auto name : { "prx", "xpx", "alpha" }) {
			EXPECT_THROW(GenerateMosaicId(name), catapult_invalid_argument) << "name " << name;
		}
	}

	TEST(TEST_CLASS, GenerateMosaicId_RejectsNamesWithTooManyParts) {
		// Act + Assert:
		for (const auto name : { "a.b.c.d:e", "a.b.c.d.e:f" }) {
			EXPECT_THROW(GenerateMosaicId(name), catapult_invalid_argument) << "name " << name;
		}
	}

	TEST(TEST_CLASS, GenerateMosaicId_RejectsImproperMosaicQualifiedNames) {
		// Act + Assert:
		for (const auto name : { "a:b:c", "a::b" }) {
			EXPECT_THROW(GenerateMosaicId(name), catapult_invalid_argument) << "name " << name;
		}
	}

	namespace {
		MosaicId GenerateMosaicIdAdapter(const RawString& namespaceName) {
			// Arrange: replace the last namespace separator with a mosaic separator
			std::string namespaceAndMosaicName(namespaceName.pData, namespaceName.Size);
			auto separatorIndex = namespaceAndMosaicName.find_last_of('.');
			if (std::string::npos != separatorIndex)
				namespaceAndMosaicName[separatorIndex] = ':';

			// Act:
			return GenerateMosaicId(namespaceAndMosaicName);
		}
	}

	ADD_BASIC_TESTS(GenerateMosaicId, GenerateMosaicIdAdapter)

	// endregion

	// region GenerateNamespacePath

	TEST(TEST_CLASS, GenerateNamespacePath_GeneratesCorrectWellKnownRootPath) {
		// Act:
		auto path = GenerateNamespacePath("prx");

		// Assert:
		EXPECT_EQ(1u, path.size());
		EXPECT_EQ(Prx_Id, path[0]);
	}

	TEST(TEST_CLASS, GenerateNamespacePath_GeneratesCorrectWellKnownChildPath) {
		// Act:
		auto path = GenerateNamespacePath("prx.xpx");

		// Assert:
		EXPECT_EQ(2u, path.size());
		EXPECT_EQ(Prx_Id, path[0]);
		EXPECT_EQ(NamespaceId(Xpx_Id.unwrap()), path[1]);
	}

	TEST(TEST_CLASS, GenerateNamespacePath_SupportsMultiLevelNamespaces) {
		// Arrange:
		NamespacePath expected;
		expected.push_back(crypto::GenerateRootNamespaceId("foo"));
		expected.push_back(crypto::GenerateNamespaceId(expected[0], "bar"));
		expected.push_back(crypto::GenerateNamespaceId(expected[1], "baz"));

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
