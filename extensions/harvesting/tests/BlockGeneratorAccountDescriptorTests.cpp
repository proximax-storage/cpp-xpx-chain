/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "harvesting/src/BlockGeneratorAccountDescriptor.h"
#include "tests/test/nodeps/Equality.h"
#include "tests/test/nodeps/KeyTestUtils.h"
#include "tests/test/core/KeyPairTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace harvesting {

#define TEST_CLASS BlockGeneratorAccountDescriptorTests

	TEST(TEST_CLASS, CanCreateDefault) {
		// Arrange:
		auto zeroKeyPair = crypto::KeyPair::FromPrivate(crypto::PrivateKey(), KeyHashingType::Sha3);

		// Act:
		auto descriptor = BlockGeneratorAccountDescriptor();

		// Assert:
		EXPECT_EQ(zeroKeyPair.publicKey(), descriptor.signingKeyPair().publicKey());
		EXPECT_EQ(zeroKeyPair.publicKey(), descriptor.vrfKeyPair().publicKey());
	}

	TEST(TEST_CLASS, CanCreateWithExplicitKeyPairs) {
		// Arrange:
		auto signingKeyPair = test::GenerateKeyPair(KeyHashingType::Sha3);
		auto vrfKeyPair = test::GenerateVrfKeyPair();

		// Act:
		auto descriptor = BlockGeneratorAccountDescriptor(test::CopyKeyPair(signingKeyPair), test::CopyKeyPair(vrfKeyPair), 2);

		// Assert:
		EXPECT_EQ(signingKeyPair.publicKey(), descriptor.signingKeyPair().publicKey());
		EXPECT_EQ(vrfKeyPair.publicKey(), descriptor.vrfKeyPair().publicKey());
	}

	namespace {
		std::unordered_set<std::string> GetEqualTags() {
			return { "default", "copy" };
		}

		std::unordered_map<std::string, BlockGeneratorAccountDescriptor> GenerateEqualityInstanceMap() {
			auto signingKeyPair = test::GenerateKeyPair(2);
			auto vrfKeyPair = test::GenerateVrfKeyPair();

			std::unordered_map<std::string, BlockGeneratorAccountDescriptor> map;
			map.emplace("default", BlockGeneratorAccountDescriptor(test::CopyKeyPair(signingKeyPair), test::CopyKeyPair(vrfKeyPair), 2));
			map.emplace("copy", BlockGeneratorAccountDescriptor(test::CopyKeyPair(signingKeyPair), test::CopyKeyPair(vrfKeyPair), 2));
			map.emplace("diff-signing", BlockGeneratorAccountDescriptor(test::GenerateKeyPair(2), test::CopyKeyPair(vrfKeyPair), 2));
			map.emplace("diff-vrf", BlockGeneratorAccountDescriptor(test::CopyKeyPair(signingKeyPair), test::GenerateVrfKeyPair(), 2));
			map.emplace("diff-signing-vrf", BlockGeneratorAccountDescriptor(test::GenerateKeyPair(2), test::GenerateVrfKeyPair(), 2));
			return map;
		}
	}

	TEST(TEST_CLASS, OperatorEqualReturnsTrueOnlyForEqualValues) {
		test::AssertOperatorEqualReturnsTrueForEqualObjects("default", GenerateEqualityInstanceMap(), GetEqualTags());
	}

	TEST(TEST_CLASS, OperatorNotEqualReturnsTrueOnlyForUnequalValues) {
		test::AssertOperatorNotEqualReturnsTrueForUnequalObjects("default", GenerateEqualityInstanceMap(), GetEqualTags());
	}
}}
