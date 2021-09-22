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

#include "catapult/crypto/KeyPair.h"
#include "tests/TestHarness.h"

namespace catapult { namespace crypto {

#define TEST_CLASS KeyPairTests

	namespace {
		void AssertCannotCreatePrivateKeyFromStringWithSize(size_t size, char keyFirstChar, KeyHashingType hashType) {
			// Arrange:
			auto rawKeyString = test::GenerateRandomHexString(size);
			rawKeyString[0] = keyFirstChar;

			// Act + Assert: key creation should fail but string should not be cleared
			EXPECT_THROW(KeyPair::FromString(rawKeyString, hashType), catapult_invalid_argument) << "string size: " << size;
			EXPECT_EQ(keyFirstChar, rawKeyString[0]);
		}
	}

	TEST(TEST_CLASS, CannotCreateKeyPairFromInvalidString) {
		AssertCannotCreatePrivateKeyFromStringWithSize(Key_Size * 1, 'a', KeyHashingType::Sha3);
		AssertCannotCreatePrivateKeyFromStringWithSize(Key_Size * 1, 'a', KeyHashingType::Sha2);
		AssertCannotCreatePrivateKeyFromStringWithSize(Key_Size * 2, 'g', KeyHashingType::Sha3);
		AssertCannotCreatePrivateKeyFromStringWithSize(Key_Size * 2, 'g', KeyHashingType::Sha2);
		AssertCannotCreatePrivateKeyFromStringWithSize(Key_Size * 3, 'a', KeyHashingType::Sha3);
		AssertCannotCreatePrivateKeyFromStringWithSize(Key_Size * 3, 'a', KeyHashingType::Sha2);
	}

	TEST(TEST_CLASS, CanCreateKeyPairFromValidString) {
		// Arrange:
#ifdef SIGNATURE_SCHEME_NIS1
		auto rawKeyString = std::string("3485D98EFD7EB07ADAFCFD1A157D89DE2796A95E780813C0258AF3F5F84ED8CB");
		auto expectedKey = std::string("C54D6E33ED1446EEDD7F7A80A588DD01857F723687A09200C1917D5524752F8B");
#else
		auto rawKeyString = std::string("CBD84EF8F5F38A25C01308785EA99627DE897D151AFDFCDA7AB07EFD8ED98534");
		auto expectedKeySha3 = std::string("A6DC1C33C26BC67B21AC4B3F4D1E88901E23AD208260F40AF3CB0A6CE9557852");
		auto expectedKeySha2 = std::string("E343795087E44BC0CD516F3FF19954A9B90FEA7684724E5C145559D6D4D9F56D");
#endif
		// Act:
		auto keyPairSha3 = KeyPair::FromString(rawKeyString, KeyHashingType::Sha3);
		auto keyPairSha2 = KeyPair::FromString(rawKeyString, KeyHashingType::Sha2);

		// Assert:
		EXPECT_EQ(expectedKeySha3, test::ToString(keyPairSha3.publicKey()));
		EXPECT_EQ(rawKeyString, test::ToHexString(keyPairSha3.privateKey().data(), keyPairSha3.privateKey().size()));

		EXPECT_EQ(expectedKeySha2, test::ToString(keyPairSha2.publicKey()));
		EXPECT_EQ(rawKeyString, test::ToHexString(keyPairSha2.privateKey().data(), keyPairSha2.privateKey().size()));
	}

	TEST(TEST_CLASS, CanCreateKeyPairFromPrivateKey) {
		// Arrange:
		auto privateKeyStr = test::GenerateRandomHexString(Key_Size * 2);
		auto privateKey = PrivateKey::FromString(privateKeyStr);
		auto privateKey2 = PrivateKey::FromString(privateKeyStr);
		Key expectedPublicKeySha3, expectedPublicKeySha2;
		ExtractPublicKeyFromPrivateKey<KeyHashingType::Sha2>(privateKey, expectedPublicKeySha2);
		ExtractPublicKeyFromPrivateKey<KeyHashingType::Sha3>(privateKey, expectedPublicKeySha3);
		// Act:
		auto keyPairSha3 = KeyPair::FromPrivate(std::move(privateKey), KeyHashingType::Sha3);
		auto keyPairSha2 = KeyPair::FromPrivate(std::move(privateKey2), KeyHashingType::Sha2);
		// Assert:
		EXPECT_EQ(PrivateKey::FromString(privateKeyStr), keyPairSha3.privateKey());
		EXPECT_EQ(expectedPublicKeySha3, keyPairSha3.publicKey());

		EXPECT_EQ(PrivateKey::FromString(privateKeyStr), keyPairSha2.privateKey());
		EXPECT_EQ(expectedPublicKeySha2, keyPairSha2.publicKey());
	}

	TEST(TEST_CLASS, KeyPairCreatedFromPrivateKeyMatchesKeyPairCreatedFromString) {
		// Arrange:
		auto privateKeyStr = std::string("3485D98EFD7EB07ADAFCFD1A157D89DE2796A95E780813C0258AF3F5F84ED8CB");
		auto privateKey = PrivateKey::FromString(privateKeyStr);

		// Act:
		auto keyPair1 = KeyPair::FromPrivate(std::move(privateKey), KeyHashingType::Sha3);
		auto keyPair2 = KeyPair::FromString(privateKeyStr, KeyHashingType::Sha3);

		// Assert:
		EXPECT_EQ(keyPair1.privateKey(), keyPair2.privateKey());
	}

	TEST(TEST_CLASS, PassesNemTestVectors) {
		// Arrange:
#ifdef SIGNATURE_SCHEME_NIS1
		// from nem https://github.com/NewEconomyMovement/nem-test-vectors)
		std::string dataSet[] {
			"575DBB3062267EFF57C970A336EBBC8FBCFE12C5BD3ED7BC11EB0481D7704CED",
			"5B0E3FA5D3B49A79022D7C1E121BA1CBBF4DB5821F47AB8C708EF88DEFC29BFE",
			"738BA9BB9110AEA8F15CAA353ACA5653B4BDFCA1DB9F34D0EFED2CE1325AEEDA",
			"E8BF9BC0F35C12D8C8BF94DD3A8B5B4034F1063948E3CC5304E55E31AA4B95A6",
			"C325EA529674396DB5675939E7988883D59A5FC17A28CA977E3BA85370232A83"
		};

		std::string expectedSet[] {
			"C5F54BA980FCBB657DBAAA42700539B207873E134D2375EFEAB5F1AB52F87844",
			"96EB2A145211B1B7AB5F0D4B14F8ABC8D695C7AEE31A3CFC2D4881313C68EEA3",
			"2D8425E4CA2D8926346C7A7CA39826ACD881A8639E81BD68820409C6E30D142A",
			"4FEED486777ED38E44C489C7C4E93A830E4C4A907FA19A174E630EF0F6ED0409",
			"83EE32E4E145024D29BCA54F71FA335A98B3E68283F1A3099C4D4AE113B53E54",
		};
#else
		std::string dataSet[] {
			"ED4C70D78104EB11BCD73EBDC512FEBC8FBCEB36A370C957FF7E266230BB5D57",
			"FE9BC2EF8DF88E708CAB471F82B54DBFCBA11B121E7C2D02799AB4D3A53F0E5B",
			"DAEE5A32E12CEDEFD0349FDBA1FCBDB45356CA3A35AA5CF1A8AE1091BBA98B73",
			"A6954BAA315EE50453CCE3483906F134405B8B3ADD94BFC8D8125CF3C09BBFE8",
			"832A237053A83B7E97CA287AC15F9AD5838898E7395967B56D39749652EA25C3"
		};
		std::string expectedSetSha3[] {
			"5C9901721703B1B082263065BDE4929079312FB6A09683C00F131AA794796467",
			"887258790597075D955EC709131255333E5F62327933D236D6160E56A8B75A6D",
			"DBAC0727B529972CF54D0DFAA52928AAA5A99766CB1912EF3B430BD30647EEDE",
			"3A147249DD5DEC2DEBB0787F2B99E6BC5961FFB361600116D88444B461C8EF22",
			"0EF23A8FDC27032AC21065D39B965CACBEBECD08CDAE2E18AB205D751F1E7626",
		};

		std::string expectedSetSha2[] {
			"5112BA143B78132AF616AF1A94E911EAD890FDB51B164A1B57C352ECD9CA1894",
			"5F9EB725880D0B8AC122AD2939070172C8762713A1E29CE55EEEA0BFBA05E6DB",
			"2D8C6B2B1D69CC02464339F46A788D7A5A6D7875C9D12AAD4ACCF2D5B24887FC",
			"20E7F2BC716306F70A136121DC103604FD624328BCEA81E5786F3CB4CE96E60E",
			"2470623117B439AA09C487D0F3D4B23676565DC1478F7C7443579B0255FE6DE1",
		};
#endif

		ASSERT_EQ(CountOf(dataSet), CountOf(expectedSetSha3));
		ASSERT_EQ(CountOf(dataSet), CountOf(expectedSetSha2));
		for (size_t i = 0; i < CountOf(dataSet); ++i) {
			// Act:
			auto keyPairSha3 = KeyPair::FromString(dataSet[i], KeyHashingType::Sha3);
			auto keyPairSha2 = KeyPair::FromString(dataSet[i], KeyHashingType::Sha2);

			// Assert:
			EXPECT_EQ(expectedSetSha3[i], test::ToString(keyPairSha3.publicKey()));
			EXPECT_EQ(expectedSetSha2[i], test::ToString(keyPairSha2.publicKey()));
		}
	}
}}
