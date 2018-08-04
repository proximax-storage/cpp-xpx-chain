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
#include "catapult/constants.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS IdGeneratorTests

	namespace {
		template<typename TGenerator>
		void AssertDifferentNamesProduceDifferentIds(TGenerator generator) {
			// Assert:
			for (const auto name : { "jeff", "bloodyrookie", "prx.xpx", "prxx" })
				EXPECT_NE(generator("prx"), generator(name)) << "prx vs " << name;
		}

		template<typename TGenerator>
		void AssertDifferentlyCasedNamesProduceDifferentIds(TGenerator generator) {
			// Assert:
			for (const auto name : { "PRX", "Prx", "pRx", "PrX" })
				EXPECT_NE(generator("prx"), generator(name)) << "prx vs " << name;
		}

		template<typename TGenerator>
		void AssertDifferentParentNamespaceIdsProduceDifferentIds(TGenerator generator) {
			// Assert:
			for (auto i = 1u; i <= 5; ++i)
				EXPECT_NE(generator(NamespaceId()), generator(NamespaceId(i))) << "ns root vs ns " << i;
		}
	}

	// region GenerateRootNamespaceId

	TEST(TEST_CLASS, GenerateRootNamespaceId_GeneratesCorrectWellKnownIds) {
		// Assert:
		EXPECT_EQ(Prx_Id, crypto::GenerateRootNamespaceId("prx"));
	}

	TEST(TEST_CLASS, GenerateRootNamespaceId_DifferentNamesProduceDifferentIds) {
		// Assert:
		AssertDifferentNamesProduceDifferentIds(crypto::GenerateRootNamespaceId);
	}

	TEST(TEST_CLASS, GenerateRootNamespaceId_NamesAreCaseSensitive) {
		// Assert:
		AssertDifferentlyCasedNamesProduceDifferentIds(crypto::GenerateRootNamespaceId);
	}

	// endregion

	// region GenerateNamespaceId

	TEST(TEST_CLASS, GenerateNamespaceId_GeneratesCorrectWellKnownIds) {
		// Assert:
		EXPECT_EQ(Prx_Id, crypto::GenerateNamespaceId(NamespaceId(), "prx"));
	}

	TEST(TEST_CLASS, GenerateNamespaceId_DifferentNamesProduceDifferentIds) {
		// Assert:
		AssertDifferentNamesProduceDifferentIds([](const auto& name) { return crypto::GenerateNamespaceId(NamespaceId(), name); });
	}

	TEST(TEST_CLASS, GenerateNamespaceId_NamesAreCaseSensitive) {
		// Assert:
		AssertDifferentlyCasedNamesProduceDifferentIds([](const auto& name) { return crypto::GenerateNamespaceId(NamespaceId(), name); });
	}

	TEST(TEST_CLASS, GenerateNamespaceId_DifferentParentNamespaceIdsProduceDifferentIds) {
		// Assert:
		AssertDifferentParentNamespaceIdsProduceDifferentIds([](const auto& nsId) { return crypto::GenerateNamespaceId(nsId, "prx"); });
	}

	TEST(TEST_CLASS, GenerateNamespaceId_CanGenerateRootNamespaceIds) {
		// Assert:
		for (const auto name : { "jeff", "bloodyrookie", "prx.xpx", "prxx" })
			EXPECT_EQ(crypto::GenerateRootNamespaceId(name), crypto::GenerateNamespaceId(NamespaceId(), name)) << "ns: " << name;
	}

	// endregion

	// region GenerateMosaicId

	TEST(TEST_CLASS, GenerateMosaicId_GeneratesCorrectWellKnownIds) {
		// Assert:
		EXPECT_EQ(Xpx_Id, crypto::GenerateMosaicId(Prx_Id, "xpx"));
	}

	TEST(TEST_CLASS, GenerateMosaicId_DifferentNamesProduceDifferentIds) {
		// Assert:
		AssertDifferentNamesProduceDifferentIds([](const auto& name) { return crypto::GenerateMosaicId(NamespaceId(), name); });
	}

	TEST(TEST_CLASS, GenerateMosaicId_NamesAreCaseSensitive) {
		// Assert:
		AssertDifferentlyCasedNamesProduceDifferentIds([](const auto& name) { return crypto::GenerateMosaicId(NamespaceId(), name); });
	}

	TEST(TEST_CLASS, GenerateMosaicId_DifferentParentNamespaceIdsProduceDifferentIds) {
		// Assert:
		AssertDifferentParentNamespaceIdsProduceDifferentIds([](const auto& nsId) { return crypto::GenerateMosaicId(nsId, "prx"); });
	}

	// endregion
}}
