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

#include "catapult/crypto/KeyGenerator.h"
#include "catapult/crypto/PrivateKey.h"
#include "tests/TestHarness.h"

namespace catapult { namespace crypto {

#define TEST_CLASS KeyGeneratorTests

	namespace {
		const Key Public_Key_Zero = []() {
			Key zeroKey;
			std::fill(zeroKey.begin(), zeroKey.end(), static_cast<uint8_t>(0));
			return zeroKey; }();

		const BLSPublicKey BLS_Public_Key_Zero = []() {
		  	BLSPublicKey zeroKey;
			std::fill(zeroKey.begin(), zeroKey.end(), static_cast<uint8_t>(0));
			return zeroKey; }();
	}

	TEST(TEST_CLASS, CanExtractPublicKeyFromPrivateKey) {
		// Arrange:
		auto privateKey = PrivateKey::Generate(test::RandomByte);
		Key publicKey;

		// Act:
		ExtractPublicKeyFromPrivateKey(privateKey, publicKey);

		// Assert:
		EXPECT_NE(Public_Key_Zero, publicKey);
	}

	TEST(TEST_CLASS, CanExtractSamePublicKeyFromSamePrivateKey) {
		// Arrange::
		auto generatePublicKey = []() {
			Key publicKey;
			auto privateKey = PrivateKey::Generate([]() { return static_cast<uint8_t>(7); });
			ExtractPublicKeyFromPrivateKey(privateKey, publicKey);
			return publicKey;
		};

		// Act:
		auto publicKey1 = generatePublicKey();
		auto publicKey2 = generatePublicKey();

		// Assert:
		EXPECT_EQ(publicKey1, publicKey2);
	}

	TEST(TEST_CLASS, CanExtractPublicKeyFromPrivateKey_BLS) {
		// Arrange:
		auto privateKey = BLSPrivateKey::Generate(test::RandomByte);
		BLSPublicKey publicKey;

		// Act:
		ExtractPublicKeyFromPrivateKey(privateKey, publicKey);

		// Assert:
		EXPECT_NE(BLS_Public_Key_Zero, publicKey);
	}

	TEST(TEST_CLASS, CanExtractSamePublicKeyFromSamePrivateKey_BLS) {
		// Arrange::
		auto generatePublicKey = []() {
			BLSPublicKey publicKey;
			auto privateKey = BLSPrivateKey::Generate([]() { return static_cast<uint8_t>(7); });
			ExtractPublicKeyFromPrivateKey(privateKey, publicKey);
			return publicKey;
		};

		// Act:
		auto publicKey1 = generatePublicKey();
		auto publicKey2 = generatePublicKey();

		// Assert:
		EXPECT_EQ(publicKey1, publicKey2);
	}

	TEST(TEST_CLASS, CanExtractSamePublicKeyFromSamePrivateKey_BLS_Zero) {
		// Arrange::
		auto generatePublicKey = []() {
			BLSPublicKey publicKey;
			auto privateKey = BLSPrivateKey::Generate([]() { return static_cast<uint8_t>(0); });
			ExtractPublicKeyFromPrivateKey(privateKey, publicKey);
			return publicKey;
		};

		// Act:
		auto publicKey = generatePublicKey();

		// Assert:
		EXPECT_EQ(test::ToString(publicKey), "A695AD325DFC7E1191FBC9F186F58EFF42A634029731B18380FF89BF42C464A42CB8CA55B200F051F57F1E1893C68759");
	}
}}
