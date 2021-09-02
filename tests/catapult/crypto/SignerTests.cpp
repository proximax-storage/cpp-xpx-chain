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

#include "catapult/crypto/Signer.h"
#include "tests/TestHarness.h"
#include <type_traits>
#include <numeric>

namespace catapult { namespace crypto {

#define TEST_CLASS SignerTests

	namespace {
		const char* Default_Key_String = "CBD84EF8F5F38A25C01308785EA99627DE897D151AFDFCDA7AB07EFD8ED98534";

		struct KeyTraits {
			using SignatureType = Signature;

			static KeyPair GetDefaultKeyPair() {
				return KeyPair::FromString(Default_Key_String);
			}

			template<typename TArray>
			static Signature SignPayload(const KeyPair& keyPair, const TArray& payload) {
				Signature signature{};
				EXPECT_NO_THROW(Sign(keyPair, payload, signature));
				return signature;
			}

			static KeyPair GetAlteredKeyPair() {
				return KeyPair::FromString("CBD84EF8F5F38A25C01308785EA99627DE897D151AFDFCDA7AB07EFD8ED98535");
			}
		};

		struct BLSTraits {
			using SignatureType = BLSSignature;

			static BLSKeyPair GetDefaultKeyPair() {
				return BLSKeyPair::FromRawString(Default_Key_String);
			}

			template<typename TArray>
			static BLSSignature SignPayload(const BLSKeyPair& keyPair, const TArray& payload) {
				BLSSignature signature{};
				EXPECT_NO_THROW(Sign(keyPair, payload, signature));
				return signature;
			}

			static BLSKeyPair GetAlteredKeyPair() {
				return BLSKeyPair::FromRawString("CBD84EF8F5F38A25C01308785EA99627DE897D151AFDFCDA7AB07EFD8ED98535");
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<KeyTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<BLSTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(SignFillsTheSignature) {
		// Arrange:
		auto payload = test::GenerateRandomArray<100>();

		// Act:
		typename TTraits::SignatureType signature;
		std::iota(signature.begin(), signature.end(), static_cast<uint8_t>(0));
		Sign(TTraits::GetDefaultKeyPair(), payload, signature);

		// Assert: the signature got overwritten in call to Sign() above
		typename TTraits::SignatureType invalid;
		std::iota(invalid.begin(), invalid.end(), static_cast<uint8_t>(0));
		EXPECT_NE(invalid, signature);
	}

	TRAITS_BASED_TEST(SignaturesGeneratedForSameDataBySameKeyPairsAreEqual) {
		// Arrange:
		auto keyPair1 = TTraits::GetDefaultKeyPair();
		auto keyPair2 = TTraits::GetDefaultKeyPair();
		auto payload = test::GenerateRandomArray<100>();

		// Act:

		auto signature1 = TTraits::SignPayload(keyPair1, payload);
		auto signature2 = TTraits::SignPayload(keyPair2, payload);

		// Assert:
		EXPECT_EQ(signature1, signature2);
	}

	TRAITS_BASED_TEST(SignaturesGeneratedForSameDataByDifferentKeyPairsAreDifferent) {
		// Arrange:
		auto payload = test::GenerateRandomArray<100>();

		// Act:
		auto signature1 = TTraits::SignPayload(TTraits::GetDefaultKeyPair(), payload);
		auto signature2 = TTraits::SignPayload(TTraits::GetAlteredKeyPair(), payload);

		// Assert:
		EXPECT_NE(signature1, signature2);
	}

	TRAITS_BASED_TEST(SignedDataCanBeVerified) {
		// Arrange:
		auto payload = test::GenerateRandomArray<100>();
		auto signature = TTraits::SignPayload(TTraits::GetDefaultKeyPair(), payload);

		// Act:
		bool isVerified = Verify(TTraits::GetDefaultKeyPair().publicKey(), payload, signature);

		// Assert:
		EXPECT_TRUE(isVerified);
	}

	TRAITS_BASED_TEST(SignedDataCannotBeVerifiedWithDifferentKeyPair) {
		// Arrange:
		auto payload = test::GenerateRandomArray<100>();
		auto signature = TTraits::SignPayload(TTraits::GetDefaultKeyPair(), payload);

		// Act:
		bool isVerified = Verify(TTraits::GetAlteredKeyPair().publicKey(), payload, signature);

		// Assert:
		EXPECT_FALSE(isVerified);
	}

	namespace {
		template<typename TTraits>
		void AssertSignatureChangeInvalidatesSignature(size_t position) {
			// Arrange:
			auto keyPair = TTraits::GetDefaultKeyPair();
			auto payload = test::GenerateRandomArray<100>();

			auto signature = TTraits::SignPayload(keyPair, payload);
			signature[position] ^= 0xFF;

			// Act:
			bool isVerified = Verify(keyPair.publicKey(), payload, signature);

			// Assert:
			EXPECT_FALSE(isVerified);
		}
	}

	TRAITS_BASED_TEST(SignatureDoesNotVerifyWhenRPartOfSignatureIsModified) {
		// Assert:
		for (auto i = 0u; i < Signature_Size / 2; ++i)
			AssertSignatureChangeInvalidatesSignature<TTraits>(i);
	}

	TRAITS_BASED_TEST(SignatureDoesNotVerifyWhenSPartOfSignatureIsModified) {
		// Assert:
		for (auto i = Signature_Size / 2; i < Signature_Size; ++i)
			AssertSignatureChangeInvalidatesSignature<TTraits>(i);
	}

	TRAITS_BASED_TEST(SignatureDoesNotVerifyWhenPayloadIsModified) {
		// Arrange:
		auto keyPair = TTraits::GetDefaultKeyPair();
		auto payload = test::GenerateRandomArray<100>();
		for (auto i = 0u; i < payload.size(); ++i) {
			auto signature = TTraits::SignPayload(keyPair, payload);
			payload[i] ^= 0xFF;

			// Act:
			bool isVerified = Verify(keyPair.publicKey(), payload, signature);

			// Assert:
			EXPECT_FALSE(isVerified);
		}
	}

	TRAITS_BASED_TEST(PublicKeyNotOnACurveCausesVerifyToFail) {
		// Arrange:
		auto hackedKeyPair = TTraits::GetDefaultKeyPair();
		auto payload = test::GenerateRandomArray<100>();

		// hack the key, to an invalid one (not on a curve)
		typedef decltype(hackedKeyPair.publicKey()) ConstKeyType;
		typedef typename std::add_lvalue_reference<
		        typename std::remove_const<
		                typename std::remove_reference<ConstKeyType>::type>::type>::type KeyType;
		auto& hackPublic = const_cast<KeyType>(hackedKeyPair.publicKey());
		std::fill(hackPublic.begin(), hackPublic.end(), static_cast<uint8_t>(0));
		hackPublic[hackPublic.size() - 1] = 0x01;

		auto signature = TTraits::SignPayload(hackedKeyPair, payload);

		// Act:
		bool isVerified = Verify(hackedKeyPair.publicKey(), payload, signature);

		// Assert:
		EXPECT_FALSE(isVerified);
	}

	TRAITS_BASED_TEST(VerificationFailsWhenPublicKeyDoesNotCorrespondToPrivateKey) {
		// Arrange:
		auto hackedKeyPair = TTraits::GetDefaultKeyPair();
		auto payload = test::GenerateRandomArray<100>();

		// hack the key, to an invalid one
		typedef decltype(hackedKeyPair.publicKey()) ConstKeyType;
		typedef typename std::add_lvalue_reference<
				typename std::remove_const<
						typename std::remove_reference<ConstKeyType>::type>::type>::type KeyType;
		auto& hackPublic = const_cast<KeyType>(hackedKeyPair.publicKey());
		std::transform(hackPublic.begin(), hackPublic.end(), hackPublic.begin(), [](uint8_t x) {
			return static_cast<uint8_t>(x ^ 0xFF);
		});

		auto signature = TTraits::SignPayload(hackedKeyPair, payload);

		// Act:
		bool isVerified = Verify(hackedKeyPair.publicKey(), payload, signature);

		// Assert:
		EXPECT_FALSE(isVerified);
	}

	TRAITS_BASED_TEST(VerifyRejectsZeroPublicKey) {
		// Arrange:
		auto hackedKeyPair = TTraits::GetDefaultKeyPair();
		auto payload = test::GenerateRandomArray<100>();

		// hack the key, to an invalid one
		typedef decltype(hackedKeyPair.publicKey()) ConstKeyType;
		typedef typename std::add_lvalue_reference<
				typename std::remove_const<
						typename std::remove_reference<ConstKeyType>::type>::type>::type KeyType;
		auto& hackPublic = const_cast<KeyType>(hackedKeyPair.publicKey());
		std::fill(hackPublic.begin(), hackPublic.end(), static_cast<uint8_t>(0));

		auto signature = TTraits::SignPayload(hackedKeyPair, payload);

		// Act:
		// keep in mind, there's no good way to make this test, as right now, we have
		// no way (and I don't think we need one), to check why verify failed
		bool isVerified = Verify(hackedKeyPair.publicKey(), payload, signature);

		// Assert:
		EXPECT_FALSE(isVerified);
	}

	namespace {
		template<typename TTraits>
		void ScalarAddGroupOrder(uint8_t* scalar) {
			// 2^252 + 27742317777372353535851937790883648493, little endian.
			const auto Group_Order = test::ToArray<sizeof(typename TTraits::SignatureType) / 2>("EDD3F55C1A631258D69CF7A2DEF9DE1400000000000000000000000000000010");
			uint8_t r = 0;
			for (auto i = 0u; i < sizeof(typename TTraits::SignatureType) / 2; ++i) {
				auto t = static_cast<uint16_t>(scalar[i]) + static_cast<uint16_t>(Group_Order[i]);
				scalar[i] += Group_Order[i] + r;
				r = static_cast<uint8_t>(t >> 8);
			}
		}
	}

	TRAITS_BASED_TEST(CannotVerifyNonCanonicalSignature) {
		// Arrange:
		std::array<uint8_t, 10> payload{ { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 } };

		auto keyPair = TTraits::GetDefaultKeyPair();
		auto canonicalSignature = TTraits::SignPayload(keyPair, payload);
		// this is signature with group order added to 'encodedS' part of signature
		auto nonCanonicalSignature = canonicalSignature;
		ScalarAddGroupOrder<TTraits>(nonCanonicalSignature.data() + Signature_Size / 2);

		// Act:
		bool isCanonicalVerified = Verify(keyPair.publicKey(), payload, canonicalSignature);
		bool isNonCanonicalVerified = Verify(keyPair.publicKey(), payload, nonCanonicalSignature);

		// Assert:
		EXPECT_TRUE(isCanonicalVerified);
		EXPECT_FALSE(isNonCanonicalVerified);
	}

	namespace {
		struct TestVectorsInput {
			std::vector<std::string> InputData;
			std::vector<std::string> PrivateKeys;
			std::vector<std::string> ExpectedPublicKeys;
			std::vector<std::string> ExpectedSignatures;
		};

		TestVectorsInput GetTestVectorsInput() {
			TestVectorsInput input;
			input.InputData = {
				"8CE03CD60514233B86789729102EA09E867FC6D964DEA8C2018EF7D0A2E0E24BF7E348E917116690B9",
				"E4A92208A6FC52282B620699191EE6FB9CF04DAF48B48FD542C5E43DAA9897763A199AAA4B6F10546109F47AC3564FADE0",
				"13ED795344C4448A3B256F23665336645A853C5C44DBFF6DB1B9224B5303B6447FBF8240A2249C55",
				"A2704638434E9F7340F22D08019C4C8E3DBEE0DF8DD4454A1D70844DE11694F4C8CA67FDCB08FED0CEC9ABB2112B5E5F89",
				"D2488E854DBCDFDB2C9D16C8C0B2FDBC0ABB6BAC991BFE2B14D359A6BC99D66C00FD60D731AE06D0",
			};

#ifdef SIGNATURE_SCHEME_NIS1
			// reversed private keys
			input.PrivateKeys = {
				"ABF4CF55A2B3F742D7543D9CC17F50447B969E6E06F5EA9195D428AB12B7318D",
				"6AA6DAD25D3ACB3385D5643293133936CDDDD7F7E11818771DB1FF2F9D3F9215",
				"8E32BC030A4C53DE782EC75BA7D5E25E64A2A072A56E5170B77A4924EF3C32A9",
				"C83CE30FCB5B81A51BA58FF827CCBC0142D61C13E2ED39E78E876605DA16D8D7",
				"2DA2A0AAE0F37235957B51D15843EDDE348A559692D8FA87B94848459899FC27"
			};
			input.ExpectedPublicKeys = {
				"8A558C728C21C126181E5E654B404A45B4F0137CE88177435A69978CC6BEC1F4",
				"BBC8CBB43DDA3ECF70A555981A351A064493F09658FFFE884C6FAB2A69C845C6",
				"72D0E65F1EDE79C4AF0BA7EC14204E10F0F7EA09F2BC43259CD60EA8C3A087E2",
				"3EC8923F9EA5EA14F8AAA7E7C2784653ED8C7DE44E352EF9FC1DEE81FC3FA1A3",
				"D73D0B14A9754EEC825FCB25EF1CFA9AE3B1370074EDA53FC64C22334A26C254",
			};
			input.ExpectedSignatures = {
				"D9CEC0CC0E3465FAB229F8E1D6DB68AB9CC99A18CB0435F70DEB6100948576CD5C0AA1FEB550BDD8693EF81EB10A556A622DB1F9301986827B96716A7134230C",
				"98BCA58B075D1748F1C3A7AE18F9341BC18E90D1BEB8499E8A654C65D8A0B4FBD2E084661088D1E5069187A2811996AE31F59463668EF0F8CB0AC46A726E7902",
				"EF257D6E73706BB04878875C58AA385385BF439F7040EA8297F7798A0EA30C1C5EFF5DDC05443F801849C68E98111AE65D088E726D1D9B7EECA2EB93B677860C",
				"0C684E71B35FED4D92B222FC60561DB34E0D8AFE44BDD958AAF4EE965911BEF5991236F3E1BCED59FC44030693BCAC37F34D29E5AE946669DC326E706E81B804",
				"6F17F7B21EF9D6907A7AB104559F77D5A2532B557D95EDFFD6D88C073D87AC00FC838FC0D05282A0280368092A4BD67E95C20F3E14580BE28D8B351968C65E03"
			};
#else
			input.PrivateKeys = {
				"8D31B712AB28D49591EAF5066E9E967B44507FC19C3D54D742F7B3A255CFF4AB",
				"15923F9D2FFFB11D771818E1F7D7DDCD363913933264D58533CB3A5DD2DAA66A",
				"A9323CEF24497AB770516EA572A0A2645EE2D5A75BC72E78DE534C0A03BC328E",
				"D7D816DA0566878EE739EDE2131CD64201BCCC27F88FA51BA5815BCB0FE33CC8",
				"27FC9998454848B987FAD89296558A34DEED4358D1517B953572F3E0AAA0A22D"
			};
			input.ExpectedPublicKeys = {
				"53C659B47C176A70EB228DE5C0A0FF391282C96640C2A42CD5BBD0982176AB1B",
				"3FE4A1AA148F5E76891CE924F5DC05627A87047B2B4AD9242C09C0ECED9B2338",
				"F398C0A2BDACDBD7037D2F686727201641BBF87EF458F632AE2A04B4E8F57994",
				"6A283A241A8D8203B3A1E918B1E6F0A3E14E75E16D4CFFA45AE4EF89E38ED6B5",
				"4DC62B38215826438DE2369743C6BBE6D13428405025DFEFF2857B9A9BC9D821"
			};
			input.ExpectedSignatures = {
				"C9B1342EAB27E906567586803DA265CC15CCACA411E0AEF44508595ACBC47600D02527F2EED9AB3F28C856D27E30C3808AF7F22F5F243DE698182D373A9ADE03",
				"0755E437ED4C8DD66F1EC29F581F6906AB1E98704ECA94B428A25937DF00EC64796F08E5FEF30C6F6C57E4A5FB4C811D617FA661EB6958D55DAE66DDED205501",
				"15D6585A2A456E90E89E8774E9D12FE01A6ACFE09936EE41271AA1FBE0551264A9FF9329CB6FEE6AE034238C8A91522A6258361D48C5E70A41C1F1C51F55330D",
				"F6FB0D8448FEC0605CF74CFFCC7B7AE8D31D403BCA26F7BD21CB4AC87B00769E9CC7465A601ED28CDF08920C73C583E69D621BA2E45266B86B5FCF8165CBE309",
				"E88D8C32FE165D34B775F70657B96D8229FFA9C783E61198A6F3CCB92F487982D08F8B16AB9157E2EFC3B78F126088F585E26055741A9F25127AC13E883C9A05"
			};
#endif

			// Sanity:
			EXPECT_EQ(input.InputData.size(), input.PrivateKeys.size());
			EXPECT_EQ(input.InputData.size(), input.ExpectedPublicKeys.size());
			EXPECT_EQ(input.InputData.size(), input.ExpectedSignatures.size());
			return input;
		}

		struct BLSTestVectorsInput {
			std::vector<std::string> InputData;
			std::vector<std::string> RawPrivateKeys;
			std::vector<std::string> ExpectedPrivateKeys;
			std::vector<std::string> ExpectedPublicKeys;
			std::vector<std::string> ExpectedSignatures;
		};

		BLSTestVectorsInput GetBLSTestVectorsInput() {
			BLSTestVectorsInput input;
			input.InputData = {
				"hello foo",
				"Brother, I love you",
				"Jo Jo",
				"Hello Kitty",
				"Ha Haha Hahaha Hahahaha Hahahahaha Hahahahahaha Hahahahahahaha",
			};

			input.RawPrivateKeys = {
				"0000000000000000000000000000000000000000000000000000000000000000",
				"15923F9D2FFFB11D771818E1F7D7DDCD363913933264D58533CB3A5DD2DAA66A",
				"A9323CEF24497AB770516EA572A0A2645EE2D5A75BC72E78DE534C0A03BC328E",
				"D7D816DA0566878EE739EDE2131CD64201BCCC27F88FA51BA5815BCB0FE33CC8",
				"27FC9998454848B987FAD89296558A34DEED4358D1517B953572F3E0AAA0A22D"
			};

			input.ExpectedPrivateKeys = {
				"3562DBB3987D4FEB5B898633CC9C812AEC49F2C64CAD5B34F5A086DF199A124D",
				"B49CAEC8626B832103C7B999D001B911A215E8942EB4C7FED235B6DBC3E48E69",
				"D5D8B9ABF20D6493BD21162415C1B756F19188BD36FE93C5339E8F2E6E5C614E",
				"426AD2746B63CB08EBE6C16B4E2B9C47BEBD8648135FC720EAAA886A0323AA36",
				"A364026A6F25E375553646C4A29A51C635F669DD68B6188E2D72CEED95516E37"
			};
			input.ExpectedPublicKeys = {
				"A695AD325DFC7E1191FBC9F186F58EFF42A634029731B18380FF89BF42C464A42CB8CA55B200F051F57F1E1893C68759",
				"A389E43A21A4F2C2CC465F2CB666FD8C5BEEBCBB05547DA36121035DF1D0FF9BDE2583B5F1886A0A66CD729BC619E770",
				"8504A48E1116F51D5857C5E281CD4EACF196C7A288ED55546C3E0B16FADFEFC95D0E947DF2D483310CDCE5836DD5DCB9",
				"B0900B51CD5FE877EA91248537D787E90883C85361DE79A25A460741EFB29DFEC86B06D028C0FFE9B02671FDA7113538",
				"AF6B154FC92D1EFDE3A75D297F0119581E24D67F895DB7CB252B8BEF5BA93054B69881688CB35FFFB86993251E4C88EA"
			};
			input.ExpectedSignatures = {
				"864CBD674C5657256671C3BBDA0EF50D077FFE50EB03AC2C9C1F435B5BF5D872603BCD0A337665A0EF867397F3985EAD0EB347E6CAE7E0BA4F0F04355D8886899393AC82F1F895414ADDFD683054316EECEEAEA85F9C309666BBCFC922F2728A",
				"B440DAFDABB1BD03EAFD5B230E60468D10D295370A6E1EE918D2231684D22B74780835AD6EB7E7F7734557662923860C183D1BE43BEF1998BAC2175D14D4782110E4E943EAAD8DFC57CCF26BE84B443548CEC761C33894CA20263DCD793B1FAE",
				"986089A075412DAE9522AE2AEC1DA072D89A685848B0A18DA62C9FCFAA4B9706E139ED8EE99D7AC786E8AAC938CAE3FD15687CC666113AEF43AB69234488B646A1AEDDA60D7C1B253E4955ED51572FABCF6394C31C82BD00F794662E1863F27E",
				"8E1831E42FA1C3BFD7A2A346D84356F08D5AFBA01AB2CB472BEB7AFA4012B4113015A8BEB1A04E506D5976C118216156086CE1836AB4BC1AE14CFCC7FF5D32BD11B1E15C5B6CAA11EC57B1C40BE657A8724889CA87E353C8701902EE9FDD8656",
				"A5649669AB0162C7219CC91A5B3BB0DB1DF2CF2B597702C404AEBF3DDCA9CBD4081375C585F38B392BAA443D48F9DE6703D3516D3408B4ABEC2062C5B64CBF611C046A2FE6CED0788D1F7FDE6AE7277FB25257790D5C9713CF0F37A661862576"
			};

			// Sanity:
			EXPECT_EQ(input.InputData.size(), input.RawPrivateKeys.size());
			EXPECT_EQ(input.InputData.size(), input.ExpectedPrivateKeys.size());
			EXPECT_EQ(input.InputData.size(), input.ExpectedPublicKeys.size());
			EXPECT_EQ(input.InputData.size(), input.ExpectedSignatures.size());
			return input;
		}
	}

	TEST(TEST_CLASS, SignPassesTestVectors) {
		// Arrange:
		auto input = GetTestVectorsInput();

		// Act / Assert:
		for (auto i = 0u; i < input.InputData.size(); ++i) {
			// Act:
			auto keyPair = KeyPair::FromString(input.PrivateKeys[i]);
			auto signature = KeyTraits::SignPayload(keyPair, test::ToVector(input.InputData[i]));

			// Assert:
			auto message = "test vector at " + std::to_string(i);
			EXPECT_EQ(input.ExpectedPublicKeys[i], test::ToString(keyPair.publicKey())) << message;
			EXPECT_EQ(input.ExpectedSignatures[i], test::ToString(signature)) << message;
		}
	}

	TEST(TEST_CLASS, VerifyPassesTestVectors) {
		// Arrange:
		auto input = GetTestVectorsInput();

		// Act / Assert:
		for (auto i = 0u; i < input.InputData.size(); ++i) {
			// Act:
			auto keyPair = KeyPair::FromString(input.PrivateKeys[i]);
			auto payload = test::ToVector(input.InputData[i]);
			auto signature = KeyTraits::SignPayload(keyPair, payload);
			auto isVerified = Verify(keyPair.publicKey(), payload, signature);

			// Assert:
			auto message = "test vector at " + std::to_string(i);
			EXPECT_TRUE(isVerified) << message;
		}
	}

	TEST(TEST_CLASS, VerifyPassesBLSTestVectors) {
		// Arrange:
		auto input = GetBLSTestVectorsInput();

		// Act / Assert:
		for (auto i = 0u; i < input.InputData.size(); ++i) {
			// Act:
			auto keyPair = BLSKeyPair::FromRawString(input.RawPrivateKeys[i]);
			auto payload = utils::RawBuffer(reinterpret_cast<const uint8_t*>(input.InputData[i].data()), input.InputData[i].size());
			auto signature = BLSTraits::SignPayload(keyPair, payload);
			auto isVerified = Verify(keyPair.publicKey(), payload, signature);

			// Assert:
			auto message = "test vector at " + std::to_string(i);
			EXPECT_EQ(input.ExpectedPrivateKeys[i], test::ToString(keyPair.privateKey())) << message;
			EXPECT_EQ(input.ExpectedPublicKeys[i], test::ToString(keyPair.publicKey())) << message;
			EXPECT_EQ(input.ExpectedSignatures[i], test::ToString(signature)) << message;
			EXPECT_TRUE(isVerified) << message;
		}
	}

	TEST(TEST_CLASS, SignatureForConsecutiveDataMatchesSignatureForChunkedData) {
		// Arrange:
		auto payload = test::GenerateRandomVector(123);
		auto properSignature = KeyTraits::SignPayload(KeyTraits::GetDefaultKeyPair(), payload);

		// Act:
		{
			Signature result;
			auto partSize = payload.size() / 2;
			ASSERT_NO_THROW(Sign(KeyTraits::GetDefaultKeyPair(), {
				{ payload.data(), partSize },
				{ payload.data() + partSize, payload.size() - partSize }
			}, result));
			EXPECT_EQ(properSignature, result);
		}

		{
			Signature result;
			auto partSize = payload.size() / 3;
			ASSERT_NO_THROW(Sign(KeyTraits::GetDefaultKeyPair(), {
				{ payload.data(), partSize },
				{ payload.data() + partSize, partSize },
				{ payload.data() + 2 * partSize, payload.size() - 2 * partSize }
			}, result));
			EXPECT_EQ(properSignature, result);
		}

		{
			Signature result;
			auto partSize = payload.size() / 4;
			ASSERT_NO_THROW(Sign(KeyTraits::GetDefaultKeyPair(), {
				{ payload.data(), partSize },
				{ payload.data() + partSize, partSize },
				{ payload.data() + 2 * partSize, partSize },
				{ payload.data() + 3 * partSize, payload.size() - 3 * partSize }
			}, result));
			EXPECT_EQ(properSignature, result);
		}
	}

	TEST(TEST_CLASS, AggregateVerify) {
		// Arrange:
		auto input = GetBLSTestVectorsInput();
		std::vector<BLSSignature> signatures(input.InputData.size());
		std::vector<BLSPublicKey> keys(input.InputData.size());
		std::vector<RawBuffer> messages(input.InputData.size());
		std::vector<const BLSSignature*> p_signatures(input.InputData.size());
		std::vector<const BLSPublicKey*> p_keys(input.InputData.size());

		// Act / Assert:
		for (auto i = 0u; i < input.InputData.size(); ++i) {
			// Act:
			auto keyPair = BLSKeyPair::FromRawString(input.RawPrivateKeys[i]);
			keys[i] = keyPair.publicKey();
			p_keys[i] = &keys[i];
			messages[i] = utils::RawBuffer(reinterpret_cast<const uint8_t*>(input.InputData[i].data()), input.InputData[i].size());
			signatures[i] = BLSTraits::SignPayload(keyPair, messages[i]);
			p_signatures[i] = &signatures[i];
		}

		auto aggregatedSig = Aggregate(p_signatures);
		EXPECT_EQ("91C5891B27541EF99BB2441BF9138C59103B78EDBA6BE72BF28576D343B75B95E14749950653FFF86816C9F1654E9D100521023EBB2A664530E6674EA6AE35E7F5184FBFA81436C10C437D50DD1460F0FDCC91160605748164AD21763D0E1462",
				  test::ToString(aggregatedSig));

		EXPECT_TRUE(AggregateVerify(p_keys, messages, aggregatedSig));
	}

	TEST(TEST_CLASS, FastAggregateVerify) {
		// Arrange:
		auto input = GetBLSTestVectorsInput();
		std::vector<BLSSignature> signatures(input.InputData.size());
		std::vector<BLSPublicKey> keys(input.InputData.size());
		std::string text = "It is same message for all signers to verify that fast aggregate verify works properly";
		RawBuffer message = utils::RawBuffer(reinterpret_cast<const uint8_t*>(text.data()), text.size());
		std::vector<const BLSSignature*> p_signatures(input.InputData.size());
		std::vector<const BLSPublicKey*> p_keys(input.InputData.size());

		// Act / Assert:
		for (auto i = 0u; i < input.InputData.size(); ++i) {
			// Act:
			auto keyPair = BLSKeyPair::FromRawString(input.RawPrivateKeys[i]);
			keys[i] = keyPair.publicKey();
			p_keys[i] = &keys[i];
			signatures[i] = BLSTraits::SignPayload(keyPair, message);
			p_signatures[i] = &signatures[i];
		}

		auto aggregatedSig = Aggregate(p_signatures);
		EXPECT_EQ("B98B4E81319EAA7BB4BF3BC123697EDDFC888A52D7CD8FC4F260639D448C6409E4BC4437A3DA69A72D99CA153FB466FF15F2EE567DDF667F4F88A6BCA92C5CAA5B2C359DBF9367703D88D1974501D0CE0A47012AB4269498CBE076B763BE0B83",
				  test::ToString(aggregatedSig));

		auto aggregatedKey = Aggregate(p_keys);
		EXPECT_EQ("B1BB94B73381FE39C7D25C3E1353274D34D09CE698E04548B7F6DD49C062DAF17DA13405C92CB61C04508B991576183D",
				  test::ToString(aggregatedKey));

		EXPECT_TRUE(FastAggregateVerify(p_keys, message, aggregatedSig));
	}
}}
