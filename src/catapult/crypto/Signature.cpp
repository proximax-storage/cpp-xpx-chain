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

#pragma once

#include "CryptoUtils.h"
#include "Signature.h"
#include "catapult/exceptions.h"
#include <cstring>
#include "external/donna/ed25519-donna.h"
#include "external/ref10/crypto_verify_32.h"
#include "external/donna/catapult.h"
#include "KeyPair.h"
#include "SecureZero.h"

extern "C" {
#include <external/ref10/ge.h>
#include <external/ref10/sc.h>
}
#ifdef _MSC_VER
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif
namespace catapult { namespace crypto {

	namespace {
		/// Current common signature code for all versions

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

		template<typename TFirstHashFunction, typename TSecondHashFunction>
		void SignInternal(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, RawSignature& signature, TFirstHashFunction nonceAndHash, TSecondHashFunction buildHash)
		{
			uint8_t *RESTRICT encodedR = signature.data();
			uint8_t *RESTRICT encodedS = signature.data() + Encoded_Size;

			// r = H(privHash[256:512] || data)
			// "EdDSA avoids these issues by generating r = H(h_b, ..., h_2b-1, M), so that
			//  different messages will lead to different, hard-to-predict values of r."
			bignum256modm r;


			// hash the private key to improve randomness
			Hash512 privHash;

			nonceAndHash(keyPair.privateKey(), buffersList, r, privHash);

			// R = rModQ * base point
			ge25519 ALIGN(16) R;
			ge25519_scalarmult_base_niels(&R, ge25519_niels_base_multiples, r);
			ge25519_pack(encodedR, &R);

			// h = H(encodedR || public || data)
			Hash512 hash_h;
			buildHash(hash_h, { { encodedR, Encoded_Size }, keyPair.publicKey() }, buffersList);

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

		template<typename TFirstHashFunction, typename TSecondHashFunction>
		void SignInternalRef10(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, RawSignature& signature, TFirstHashFunction hashPrivateKey, TSecondHashFunction buildHash)
		{
			uint8_t *RESTRICT encodedR = signature.data();
			uint8_t *RESTRICT encodedS = signature.data() + Encoded_Size;

			// Hash the private key to improve randomness. Hashing always follows the keypair hashing system!
			Hash512 privHash;
			hashPrivateKey(keyPair.privateKey(), privHash);

			// r = H(privHash[256:512] || data)
			// "EdDSA avoids these issues by generating r = H(h_b, ..., h_2b?1, M), so that
			//  different messages will lead to different, hard-to-predict values of r."
			Hash512 r;
			buildHash(r, {RawBuffer(privHash.data() + Hash512::Size / 2, Hash512::Size / 2 )}, buffersList);

			// Reduce size of r since we are calculating mod group order anyway
			sc_reduce(r.data());

			// R = rModQ * base point
			ge_p3 rMulBase;
			ge_scalarmult_base(&rMulBase, r.data());
			ge_p3_tobytes(encodedR, &rMulBase);

			// h = H(encodedR || public || data)
			Hash512 h;
			buildHash(h, { { encodedR, Encoded_Size }, keyPair.publicKey() }, buffersList);

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

		template<typename TBuildHashFunction>
		bool VerifyInternal(const Key& publicKey, std::initializer_list<const RawBuffer> buffers, const RawSignature& signature, TBuildHashFunction buildHash) {
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
			buildHash(hash_h, { { encodedR, Encoded_Size }, publicKey }, buffers);

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
			return 1 == ed25519_verify(encodedR, checkr, 32);
		}

		template<typename TBuildHashFunction>
		bool VerifyInternalRef10(const Key& publicKey, std::initializer_list<const RawBuffer> buffers, const RawSignature& signature, DerivationScheme derivationScheme, TBuildHashFunction buildHash) {
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
			buildHash(h, { { encodedR, Encoded_Size }, publicKey }, buffers);

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
	}


	const DerivationScheme SignatureFeatureSolver::GetDerivationScheme(const Signature& signature) {
		return *reinterpret_cast<const DerivationScheme*>(signature.data());
	}

	void SignatureFeatureSolver::SetDerivationScheme(Signature& signature, DerivationScheme scheme) {
		*reinterpret_cast<DerivationScheme*>(signature.data()) = scheme;
	}

	const RawSignature& SignatureFeatureSolver::GetRawSignature(const Signature& signature) {
		return *reinterpret_cast<const RawSignature*>(signature.data()+1);
	}

	RawSignature& SignatureFeatureSolver::GetRawSignature(Signature& signature) {
		return *reinterpret_cast<RawSignature*>(signature.data()+1);
	}
	Signature SignatureFeatureSolver::ExpandSignature(const RawSignature& signature, DerivationScheme scheme)
	{
		auto result = Signature();
		auto data = result.data()+1;
		*reinterpret_cast<RawSignature*>(data) = signature;
		*result.data() = scheme;
		//SecureZero(signature);
		return result;
	}
	/// Signs the \a dataBuffer outputting the \a signature with the given \a keyPair
	void SignatureFeatureSolver::Sign(const KeyPair& keyPair, const RawBuffer& dataBuffer, RawSignature& signature) {
		Sign(keyPair, { dataBuffer }, signature);
	}

	/// Signs the \a dataBuffer outputting the \a signature with the given \a keyPair
	void SignatureFeatureSolver::Sign(const KeyPair& keyPair, const RawBuffer& dataBuffer, Signature& signature) {
		Sign(keyPair, { dataBuffer }, GetRawSignature(signature));
		SetDerivationScheme(signature, keyPair.derivationScheme());
	}

	/// Signs the \a dataBuffer outputting the \a signature with the given \a keyPair
	void SignatureFeatureSolver::Sign(const KeyPair& keyPair, std::initializer_list<const RawBuffer> dataBuffer, Signature& signature) {
		Sign(keyPair, dataBuffer, GetRawSignature(signature));
		SetDerivationScheme(signature, keyPair.derivationScheme());
	}

	/// Signs the \a buffersList outputting the \a signature with the given \a keyPair
	/// Perf note: Evaluate whether to replace functors with code duplication
	void SignatureFeatureSolver::Sign(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, RawSignature& signature)
	{
		switch(keyPair.derivationScheme())
		{
			case DerivationScheme::Ed25519_Sha3:
			{
				SignInternal(keyPair, buffersList, signature, GenerateNonceAndHash<DerivationScheme::Ed25519_Sha3>, BuildHash<DerivationScheme::Ed25519_Sha3>);
				break;
			}
			case DerivationScheme::Ed25519_Sha2:
			{
				SignInternal(keyPair, buffersList, signature, GenerateNonceAndHash<DerivationScheme::Ed25519_Sha2>, BuildHash<DerivationScheme::Ed25519_Sha2>);
				break;
			}
			default:
			{
				CATAPULT_THROW_RUNTIME_ERROR("Unable to recognize derivation scheme used in this key!");
			}
		}
	}

	/// Verifies the \a buffers with the \a signature for the given \a publicKey
	bool SignatureFeatureSolver::Verify(const Key& publicKey, std::initializer_list<const RawBuffer> buffers, Signature& signature) {
		return Verify(publicKey, buffers, GetRawSignature(signature), GetDerivationScheme(signature));
	}

	/// Verifies the \a dataBuffer with the \a signature for the given \a publicKey
	bool SignatureFeatureSolver::Verify(const Key& publicKey, const RawBuffer& dataBuffer, Signature& signature) {
		return Verify(publicKey, { dataBuffer }, GetRawSignature(signature), GetDerivationScheme(signature));
	}

	/// Verifies the \a buffers with the \a signature for the given \a publicKey
	bool SignatureFeatureSolver::Verify(const Key& publicKey, std::initializer_list<const RawBuffer> buffers, const RawSignature& signature, DerivationScheme derivationScheme)
	{
		switch(derivationScheme)
		{
			case DerivationScheme::Unset:
			case DerivationScheme::Ed25519_Sha3:
			{
				return VerifyInternal(publicKey, buffers, signature, BuildHash<DerivationScheme::Ed25519_Sha3>);
			}
			case DerivationScheme::Ed25519_Sha2:
			{
				return VerifyInternal(publicKey, buffers, signature, BuildHash<DerivationScheme::Ed25519_Sha2>);
			}
			default:
			{
				CATAPULT_THROW_RUNTIME_ERROR("Unable to recognize derivation scheme used in this signature!");
			}
		}
	}

}}
