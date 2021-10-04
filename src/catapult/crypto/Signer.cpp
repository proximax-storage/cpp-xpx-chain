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

#include "Signer.h"
#include "CryptoUtils.h"
#include "catapult/exceptions.h"
#include <cstring>
#include <ref10/crypto_verify_32.h>
#include <donna/catapult.h>
#include "SecureZero.h"
#include "catapult/utils/SignatureVersionToKeyTypeResolver.h"

extern "C" {
#include <ref10/ge.h>
#include <ref10/sc.h>
}

#ifdef _MSC_VER
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif

namespace catapult { namespace crypto {

	namespace {
		const size_t Encoded_Size = Signature_Size / 2;
		static_assert(Encoded_Size * 2 == Hash512_Size, "hash must be big enough to hold two encoded elements");

		/// Indicates that the encoded S part of the signature is less than the group order.
		constexpr int Is_Reduced = 1;

		/// Indicates that the encoded S part of the signature is zero.
		constexpr int Is_Zero = 2;

		int ValidateEncodedSPart(const uint8_t* encodedS) {
			uint8_t encodedBuf[RawSignature::Size];
			uint8_t *RESTRICT encodedTempR = encodedBuf;
			uint8_t *RESTRICT encodedZero = encodedBuf + Encoded_Size;

			std::memset(encodedZero, 0, Encoded_Size);
			if (0 == std::memcmp(encodedS, encodedZero, Encoded_Size))
				return Is_Zero | Is_Reduced;

			std::memcpy(encodedTempR, encodedS, Encoded_Size);
			sc_reduce(encodedBuf);

			return std::memcmp(encodedTempR, encodedS, Encoded_Size) ? 0 : Is_Reduced;
		}

		bool IsCanonicalS(const uint8_t* encodedS) {
			return Is_Reduced == ValidateEncodedSPart(encodedS);
		}

		void CheckEncodedS(const uint8_t* encodedS) {
			if (0 == (ValidateEncodedSPart(encodedS) & Is_Reduced))
			CATAPULT_THROW_OUT_OF_RANGE("S part of signature invalid");
		}

	}

	const RawSignature& GetRawSignature(const Signature& signature) {
		return *reinterpret_cast<const RawSignature*>(signature.data()+1);
	}

	const SignatureVersion GetSignatureVersion(Signature signature) {
		return *reinterpret_cast<const SignatureVersion*>(signature.begin());
	}

	RawSignature& GetRawSignature(Signature& signature) {
		return *reinterpret_cast<RawSignature*>(signature.data()+1);
	}

	void Sign(const KeyPair& keyPair, const RawBuffer& dataBuffer, Signature& signature) {
		Sign(keyPair, { dataBuffer }, GetRawSignature(signature));
	}
	void Sign(const KeyPair& keyPair, const RawBuffer& dataBuffer, RawSignature& signature) {
		Sign(keyPair, { dataBuffer }, signature);
	}
	void SignRef10(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, RawSignature& signature) {
		uint8_t *RESTRICT encodedR = signature.data();
		uint8_t *RESTRICT encodedS = signature.data() + Encoded_Size;
		*signature.begin() = (uint8_t)keyPair.hashingType();

		// Hash the private key to improve randomness. Hashing always follows the keypair hashing system!
		Hash512 privHash;
		if(keyPair.hashingType() == KeyHashingType::Sha2)
			HashPrivateKey<KeyHashingType::Sha2>(keyPair.privateKey(), privHash);
		else
			HashPrivateKey<KeyHashingType::Sha3>(keyPair.privateKey(), privHash);
		// r = H(privHash[256:512] || data)
		// "EdDSA avoids these issues by generating r = H(h_b, ..., h_2b?1, M), so that
		//  different messages will lead to different, hard-to-predict values of r."
		Hash512 r;
		if(keyPair.hashingType() == KeyHashingType::Sha2)
			BuildHash<KeyHashingType::Sha2>(r, {RawBuffer(privHash.data() + Hash512::Size / 2, Hash512::Size / 2 )}, buffersList);
		else
			BuildHash<KeyHashingType::Sha3>(r, {RawBuffer(privHash.data() + Hash512::Size / 2, Hash512::Size / 2 )}, buffersList);

		// Reduce size of r since we are calculating mod group order anyway
		sc_reduce(r.data());

		// R = rModQ * base point
		ge_p3 rMulBase;
		ge_scalarmult_base(&rMulBase, r.data());
		ge_p3_tobytes(encodedR, &rMulBase);

		// h = H(encodedR || public || data)
		Hash512 h;
		if(keyPair.hashingType() == KeyHashingType::Sha2)
			BuildHash<KeyHashingType::Sha2>(h, { { encodedR, Encoded_Size }, keyPair.publicKey() }, buffersList);
		else
			BuildHash<KeyHashingType::Sha3>(h, { { encodedR, Encoded_Size }, keyPair.publicKey() }, buffersList);

		// h = h mod group order
		sc_reduce(h.data());

		// a = fieldElement(privHash[0:256])
		privHash[0] &= 0xF8;
		privHash[31] &= 0x7F;
		privHash[31] |= 0x40;

		// S = (r + h * a) mod group order
		sc_muladd(encodedS, h.data(), privHash.data(), r.data());

		// Signature is (encodedR, encodedS)

		// Throw if encodedS is not less than the group order, don't fail in case encodedS == 0
		// (this should only throw if there is a bug in the signing code)
		CheckEncodedS(encodedS);

	}

	void Sign(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, Signature& signature) {
		uint8_t *RESTRICT encodedR = GetRawSignature(signature).data();
		uint8_t *RESTRICT encodedS = GetRawSignature(signature).data() + Encoded_Size;
		*signature.begin() = (uint8_t)keyPair.hashingType();

		// r = H(privHash[256:512] || data)
		// "EdDSA avoids these issues by generating r = H(h_b, ..., h_2b-1, M), so that
		//  different messages will lead to different, hard-to-predict values of r."
		bignum256modm r;


		// hash the private key to improve randomness
		Hash512 privHash;

		if(keyPair.hashingType() == KeyHashingType::Sha2)
		{
			GenerateNonce<KeyHashingType::Sha2>(keyPair.privateKey(), buffersList, r);
			HashPrivateKey<KeyHashingType::Sha2>(keyPair.privateKey(), privHash);
		}
		else
		{
			GenerateNonce<KeyHashingType::Sha3>(keyPair.privateKey(), buffersList, r);
			HashPrivateKey<KeyHashingType::Sha3>(keyPair.privateKey(), privHash);
		}


		// R = rModQ * base point
		ge25519 ALIGN(16) R;
		ge25519_scalarmult_base_niels(&R, ge25519_niels_base_multiples, r);
		ge25519_pack(encodedR, &R);

		// h = H(encodedR || public || data)
		Hash512 hash_h;
		if(keyPair.hashingType() == KeyHashingType::Sha2)
			BuildHash<KeyHashingType::Sha2>(hash_h, { { encodedR, Encoded_Size }, keyPair.publicKey() }, buffersList);
		else
			BuildHash<KeyHashingType::Sha3>(hash_h, { { encodedR, Encoded_Size }, keyPair.publicKey() }, buffersList);

		bignum256modm h;
		expand256_modm(h, hash_h.data(), 64);

		// a = fieldElement(privHash[0:256])
		privHash[0] &= 0xF8;
		privHash[31] &= 0x7F;
		privHash[31] |= 0x40;

		bignum256modm a;
		expand256_modm(a, privHash.data(), 32);

		// S = (r + h * a) mod group order
		bignum256modm S;
		mul256_modm(S, h, a);
		add256_modm(S, S, r);
		contract256_modm(encodedS, S);

		// signature is (encodedR, encodedS)

		// throw if encodedS is not less than the group order, don't fail in case encodedS == 0
		// (this should only throw if there is a bug in the signing code)
		CheckEncodedS(encodedS);

		SecureZero(privHash);
		SecureZero(r);
		SecureZero(a);
	}
	bool Verify(const Key& publicKey, const RawBuffer& dataBuffer, const Signature& signature) {
		return Verify(publicKey, { dataBuffer }, GetRawSignature(signature), catapult::utils::ResolveKeyHashingTypeFromSignatureVersion(GetSignatureVersion(signature)));
	}

	bool Verify(const Key& publicKey, std::initializer_list<const RawBuffer> buffers, const RawSignature& signature, KeyHashingType hashingType) {
		const uint8_t *RESTRICT encodedR = signature.data();
		const uint8_t *RESTRICT encodedS = signature.data() + Encoded_Size;

		// reject if not canonical
		if (!IsCanonicalS(encodedS))
			return false;

		// reject zero public key, which is known weak key
		if (Key() == publicKey)
			return false;

		// h = H(encodedR || public || data)
		Hash512 hash_h;
		if(hashingType == KeyHashingType::Sha2)
			BuildHash<KeyHashingType::Sha2>(hash_h, { { encodedR, Encoded_Size }, publicKey }, buffers);
		else
			BuildHash<KeyHashingType::Sha3>(hash_h, { { encodedR, Encoded_Size }, publicKey }, buffers);

		bignum256modm h;
		expand256_modm(h, hash_h.data(), 64);

		// A = -pub
		ge25519 ALIGN(16) A;
		if (!UnpackNegativeAndCheckSubgroup(A, publicKey))
			return false;

		bignum256modm S;
		expand256_modm(S, encodedS, 32);

		// R = encodedS * B - h * A
		ge25519 ALIGN(16) R;
		ge25519_double_scalarmult_vartime(&R, &A, h, S);

		// compare calculated R to given R
		uint8_t checkr[Encoded_Size];
		ge25519_pack(checkr, &R);
		return 1 != ed25519_verify(encodedR, checkr, 32);
	}

	bool VerifyRef10(const Key& publicKey, std::initializer_list<const RawBuffer> buffersList, const RawSignature& signature, KeyHashingType hashingType) {
		const uint8_t *RESTRICT encodedR = signature.data();
		const uint8_t *RESTRICT encodedS = signature.data() + Encoded_Size;


		// reject if not canonical
		if (!IsCanonicalS(encodedS))
			return false;

		// reject zero public key, which is known weak key
		if (Key() == publicKey)
			return false;

		// h = H(encodedR || public || data)
		Hash512 h;
		if(hashingType == KeyHashingType::Sha2)
			BuildHash<KeyHashingType::Sha2>(h, { { encodedR, Encoded_Size }, publicKey }, buffersList);
		else
			BuildHash<KeyHashingType::Sha3>(h, { { encodedR, Encoded_Size }, publicKey }, buffersList);

		// h = h mod group order
		sc_reduce(h.data());

		// A = -pub
		ge_p3 A;
		if (0 != ge_frombytes_negate_vartime(&A, publicKey.data()))
			return false;

		// R = encodedS * B - h * A
		ge_p2 R;
		ge_double_scalarmult_vartime(&R, h.data(), &A, encodedS);

		// Compare calculated R to given R.
		uint8_t checkr[Encoded_Size];
		ge_tobytes(checkr, &R);
		return 0 == crypto_verify_32(checkr, encodedR);
	}
}}
