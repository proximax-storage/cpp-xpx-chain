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

#include "SecretLockInfoCacheTestUtils.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	BasicSecretLockInfoTestTraits::ValueType BasicSecretLockInfoTestTraits::CreateLockInfo(Height height) {
		return state::SecretLockInfo(
				GenerateRandomData<Key_Size>(),
				MosaicId(Random()),
				Amount(Random()),
				height,
				static_cast<model::LockHashAlgorithm>(RandomByte()),
				GenerateRandomData<Hash256_Size>(),
				GenerateRandomData<Address_Decoded_Size>());
	}

	BasicSecretLockInfoTestTraits::ValueType BasicSecretLockInfoTestTraits::CreateLockInfo() {
		return CreateLockInfo(test::GenerateRandomValue<Height>());
	}

	void BasicSecretLockInfoTestTraits::AssertEqual(const ValueType& lhs, const ValueType& rhs) {
		test::AssertEqualLockInfo(lhs, rhs);
		EXPECT_EQ(lhs.HashAlgorithm, rhs.HashAlgorithm);
		EXPECT_EQ(lhs.Secret, rhs.Secret);
		EXPECT_EQ(lhs.Recipient, rhs.Recipient);
	}
}}
