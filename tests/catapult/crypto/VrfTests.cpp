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

#include "catapult/crypto/Vrf.h"
#include "catapult/crypto/Hashes.h"
#include "catapult/crypto/SharedKey.h"
#include "catapult/utils/HexParser.h"
#include "tests/test/crypto/CurveUtils.h"
#include "tests/test/nodeps/Alignment.h"
#include "tests/TestHarness.h"

namespace catapult { namespace crypto {

#define TEST_CLASS VrfTests

	// region size + alignment

#define VRF_PROOF_FIELDS FIELD(Gamma) FIELD(VerificationHash) FIELD(Scalar)

	TEST(TEST_CLASS, VrfProofHasExpectedSize) {
		// Arrange:
		auto expectedSize = 0u;

#define FIELD(X) expectedSize += SizeOf32<decltype(VrfProof::X)>();
		VRF_PROOF_FIELDS
#undef FIELD

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(VrfProof));
		EXPECT_EQ(80u, sizeof(VrfProof));
	}

	TEST(TEST_CLASS, VrfProofHasProperAlignment) {
#define FIELD(X) EXPECT_ALIGNED(VrfProof, X);
		VRF_PROOF_FIELDS
#undef FIELD

		EXPECT_EQ(0u, sizeof(VrfProof) % 8);
	}

#undef VRF_PROOF_FIELDS

	// endregion

	// region test vectors

	namespace {
		struct TestVectorsInput {
			std::string SK;
			std::string Alpha;
		};

		struct TestVectorOutput {
			std::string Gamma;
			std::string VerificationHash;
			std::string Scalar;
			std::string Beta;
		};

		// data taken from: https://www.ietf.org/id/draft-irtf-cfrg-vrf-05.txt
		static std::vector<TestVectorsInput> SampleTestVectorsInput() {
			return {
				{ "9D61B19DEFFD5A60BA844AF492EC2CC44449C5697B326919703BAC031CAE7F60", ""},
				{ "4CCD089B28FF96DA9DB6C346EC114E0F5B8A319F35ABA624DA8CF6ED4FB8A6FB", "72" },
				{ "C5AA8DF43F9F837BEDB7442F31DCB7B166D38535076F094B85CE3A2E0B4458F7", "af82" }
			};
		}

		static std::vector<TestVectorOutput> SampleTestVectorsOutput() {
			return {
				{
					"DC8428E49F6DACB67A71DF5BCD255A045C481D135A71BEFCEBABA01C4B5F0996",
					"10D1E45ED7A42EE9633AB062D41AA752",
					"F4AD94FDB5B416689376AB385A9234FEDA788813BAA88F6561879EE89711AA07",
					"3D56E12DC42324FC6AA1F4F59881102103427621C423BD79A53CF1182E74A2DD"
					"4202D4F4409F3026E6F98AB3E17F86BCE3C2924C029A9C37A4F279463620253B"
				},
				{
					"C26C9CA33ED28D485719A3027F96DD9AC8F61BA5E925CE5F9181A90FADEF4427",
					"BC4202D03B7F6BE8F512110DDB81E890",
					"856219FCB9B2B940706CFA4D0D4409DEC4BADFE75A379B17FDCD32DD7D0F0002",
					"366E27982052510ABD3636780B8194A176F5D6BA721FF054E70BDFA4F3AB884C"
					"5C737CCACBEDB717A6677E30028C41C6DF59A3E8995EF2E289094B93FDE3EE35"
				},
				{
					"3CB981D3E7A89661A98D7D046AED877E0B0161C400629D6A819AF9E626098C48",
					"F10DD303DEA54F4791CD1F5307C2D29D",
					"AB45A5861DA4633B09BFD5AC3F6134E5D2A8B730178677E253E2ACDDC976610B",
					"F804E11E9F18163C17D359E15F053FEA47D345A1B3DCD4A70C9D460C356A0BE7"
					"384CBAC7A4DF8C2256170DE831501FEAB2163CB9514DE63B112125ABB0CC3A19"
				}
			};
		}
	}

	TEST(TEST_CLASS, VrfSampleTestVectors) {
		// Arrange:
		auto testVectorsInput = SampleTestVectorsInput();
		auto testVectorsOutput = SampleTestVectorsOutput();

		// Sanity:
		ASSERT_EQ(testVectorsInput.size(), testVectorsOutput.size());

		auto i = 0u;
		for (const auto& input : testVectorsInput) {
			// Arrange:
			auto keyPair = KeyPair::FromString(input.SK);
			auto alpha = test::ToVector(input.Alpha);
			auto message = "at index " + std::to_string(i);

			// Act: compute proof
			auto vrfProof = GenerateVrfProof(alpha, keyPair);

			// Assert:
			auto verificationHash = testVectorsOutput[i].VerificationHash;
			EXPECT_EQ(utils::ParseByteArray<ProofGamma>(testVectorsOutput[i].Gamma), vrfProof.Gamma) << message;
			EXPECT_EQ(utils::ParseByteArray<ProofVerificationHash>(verificationHash), vrfProof.VerificationHash) << message;
			EXPECT_EQ(utils::ParseByteArray<ProofScalar>(testVectorsOutput[i].Scalar), vrfProof.Scalar) << message;

			// Act: verify proof and compute beta
			auto proofHash = VerifyVrfProof(vrfProof, alpha, keyPair.publicKey());

			// Assert:
			EXPECT_EQ(utils::ParseByteArray<Hash512>(testVectorsOutput[i].Beta), proofHash) << message;
			++i;
		}
	}

	// endregion

	// region VerifyVrfProof

	namespace {
		void AssertVerifyVrfProofFailsWhenProofIsCorrupted(const consumer<VrfProof&>& transform) {
			auto keyPair = KeyPair::FromString("9D61B19DEFFD5A60BA844AF492EC2CC44449C5697B326919703BAC031CAE7F60");
			auto alpha = test::ToVector("af82");
			auto vrfProof = GenerateVrfProof(alpha, keyPair);

			// Sanity:
			auto proofHash = VerifyVrfProof(vrfProof, alpha, keyPair.publicKey());
			EXPECT_NE(Hash512(), proofHash);

			// Act: corrupt proof
			transform(vrfProof);
			proofHash = VerifyVrfProof(vrfProof, alpha, keyPair.publicKey());

			// Assert:
			EXPECT_EQ(Hash512(), proofHash);
		}
	}

	TEST(TEST_CLASS, VerifyVrfProofFailsWhenGammaIsNotOnTheCurve) {
		// Act: corrupt Gamma
		AssertVerifyVrfProofFailsWhenProofIsCorrupted([](auto& vrfProof) {
			vrfProof.Gamma = utils::ParseByteArray<ProofGamma>("4F91BE9568552181E01968999EFC09BFEB77A736B8F3188160B7769D7B9B9F6E");
		});
	}

	TEST(TEST_CLASS, VerifyVrfProofFailsWhenGammaIsWrongPointOnCurve) {
		// Act: corrupt Gamma
		AssertVerifyVrfProofFailsWhenProofIsCorrupted([](auto& vrfProof) {
			// valid public key
			vrfProof.Gamma = utils::ParseByteArray<ProofGamma>("C8C6D604F4D7B56B57247E8686168EEBB2BF8AE40DA7B912143773A77555420E");
		});
	}

	TEST(TEST_CLASS, VerifyVrfProofFailsWhenVerificationHashIsWrong) {
		// Act: corrupt VerificationHash
		AssertVerifyVrfProofFailsWhenProofIsCorrupted([](auto& vrfProof) {
			vrfProof.VerificationHash[4] ^= 0xFF;
		});
	}

	TEST(TEST_CLASS, VerifyVrfProofFailsWhenScalarIsWrong) {
		// Act: corrupt Scalar
		AssertVerifyVrfProofFailsWhenProofIsCorrupted([](auto& vrfProof) {
			vrfProof.Scalar[14] ^= 0xFF;
		});
	}

	TEST(TEST_CLASS, VerifyVrfProofFailsWhenScalarIsNotReduced) {
		// Act: add group order to Scalar
		AssertVerifyVrfProofFailsWhenProofIsCorrupted([](auto& vrfProof) {
			test::ScalarAddGroupOrder(vrfProof.Scalar.data());
		});
	}

	// endregion

	// region GenerateVrfProofHash

	TEST(TEST_CLASS, VrfSampleTestVectors_GenerateVrfProofHash) {
		// Arrange:
		auto testVectorsInput = SampleTestVectorsInput();
		auto testVectorsOutput = SampleTestVectorsOutput();

		// Sanity:
		ASSERT_EQ(testVectorsInput.size(), testVectorsOutput.size());

		auto i = 0u;
		for (const auto& input : testVectorsInput) {
			// Arrange:
			auto keyPair = KeyPair::FromString(input.SK);
			auto alpha = test::ToVector(input.Alpha);
			auto message = "at index " + std::to_string(i);

			// - compute proof hash
			auto vrfProof = GenerateVrfProof(alpha, keyPair);
			auto proofHash = VerifyVrfProof(vrfProof, alpha, keyPair.publicKey());

			// Act: compute proof hash from gamma
			auto proofHashFromGamma = GenerateVrfProofHash(vrfProof.Gamma);

			// Assert:
			EXPECT_EQ(proofHash, proofHashFromGamma) << message;
			++i;
		}
	}

	// endregion
}}
