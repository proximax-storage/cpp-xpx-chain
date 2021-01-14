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
#include "Hashes.h"
#include "catapult/exceptions.h"
#include <cstring>
#include <ref10/crypto_verify_32.h>

extern "C" {
#include <ref10/ge.h>
#include <ref10/sc.h>
#include <blst/blst.hpp>
}

#ifdef _MSC_VER
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif

namespace catapult { namespace crypto {

	namespace {
		const std::string FILECOIN_DST("BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_");
		const std::string ETH2_DST("BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_");

		const size_t Encoded_Size = Signature_Size / 2;
		static_assert(Encoded_Size * 2 == Hash512_Size, "hash must be big enough to hold two encoded elements");

		/// Indicates that the encoded S part of the signature is less than the group order.
		constexpr int Is_Reduced = 1;

		/// Indicates that the encoded S part of the signature is zero.
		constexpr int Is_Zero = 2;

		int ValidateEncodedSPart(const uint8_t* encodedS) {
			uint8_t encodedBuf[Signature_Size];
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

#ifdef SIGNATURE_SCHEME_NIS1
		using HashBuilder = Keccak_512_Builder;
#else
		using HashBuilder = Sha3_512_Builder;
#endif
	}

	void Sign(const KeyPair& keyPair, const RawBuffer& dataBuffer, Signature& computedSignature) {
		Sign(keyPair, { dataBuffer }, computedSignature);
	}

	void Sign(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, Signature& computedSignature) {
		uint8_t *RESTRICT encodedR = computedSignature.data();
		uint8_t *RESTRICT encodedS = computedSignature.data() + Encoded_Size;

		// Hash the private key to improve randomness.
		Hash512 privHash;
		HashPrivateKey(keyPair.privateKey(), privHash);

		// r = H(privHash[256:512] || data)
		// "EdDSA avoids these issues by generating r = H(h_b, ..., h_2b?1, M), so that
		//  different messages will lead to different, hard-to-predict values of r."
		Hash512 r;
		HashBuilder hasher_r;
		hasher_r.update({ privHash.data() + Hash512_Size / 2, Hash512_Size / 2 });
		hasher_r.update(buffersList);
		hasher_r.final(r);

		// Reduce size of r since we are calculating mod group order anyway
		sc_reduce(r.data());

		// R = rModQ * base point
		ge_p3 rMulBase;
		ge_scalarmult_base(&rMulBase, r.data());
		ge_p3_tobytes(encodedR, &rMulBase);

		// h = H(encodedR || public || data)
		Hash512 h;
		HashBuilder hasher_h;
		hasher_h.update({ { encodedR, Encoded_Size }, keyPair.publicKey() });
		hasher_h.update(buffersList);
		hasher_h.final(h);

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

	void Sign(const BLSKeyPair& keyPair, const RawBuffer& message, BLSSignature& computedSignature) {
		blst::blst_p2 hash_point;
		blst::blst_hash_to_g2(&hash_point, message.pData, message.Size,
							  reinterpret_cast<const uint8_t*>(FILECOIN_DST.data()), FILECOIN_DST.size());
		const auto* temp = reinterpret_cast<const blst::blst_scalar*>(&keyPair.privateKey().m_array);
		blst::blst_p2_affine out_point;
		blst::blst_sign_pk2_in_g1(nullptr, &out_point, &hash_point, temp);
		blst::blst_p2_affine_compress(computedSignature.m_array, &out_point);
	}

	bool Verify(const Key& publicKey, const RawBuffer& dataBuffer, const Signature& signature) {
		return Verify(publicKey, { dataBuffer }, signature);
	}

	bool Verify(const Key& publicKey, std::initializer_list<const RawBuffer> buffersList, const Signature& signature) {
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
		HashBuilder hasher_h;
		hasher_h.update({ { encodedR, Encoded_Size }, publicKey });
		hasher_h.update(buffersList);
		hasher_h.final(h);

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

	bool Verify(const BLSPublicKey& publicKey, const RawBuffer& dataBuffer, const BLSSignature& signature) {
		blst::P2_Affine sig;
		auto res = sig.uncompress(signature.m_array);
		if (res != blst::BLST_SUCCESS) {
			CATAPULT_LOG(error) << "can't uncompress signature " << signature;
			return false;
		}
		blst::P1_Affine pk;
		res = pk.uncompress(publicKey.m_array);
		if (res != blst::BLST_SUCCESS) {
			CATAPULT_LOG(error) << "can't uncompress public key " << publicKey;
			return false;
		}
		return sig.core_verify(pk, true /* hash_or_encode */, dataBuffer.pData, dataBuffer.Size, FILECOIN_DST, nullptr, NULL) == blst::BLST_SUCCESS;
	}

	BLSSignature Aggregate(const std::vector<const BLSSignature*>& signatures) {
		if (signatures.empty() || !signatures[0])
			return BLSSignature();

		blst::P2_Affine sig;
		auto res = sig.uncompress(signatures[0]->m_array);
		if (res != blst::BLST_SUCCESS) {
			CATAPULT_LOG(error) << "can't uncompress signature during aggregate 0 " << *signatures[0];
			return BLSSignature();
		}

		blst::P2 agg_point(sig);
		for (auto i = 1u; i < signatures.size(); ++i) {
			if (!signatures[i])
				return BLSSignature();

			res = sig.uncompress(signatures[i]->m_array);
			if (res != blst::BLST_SUCCESS) {
				CATAPULT_LOG(error) << "can't uncompress signature during aggregate " << i << ' ' << *signatures[i];
				return BLSSignature();
			}
			agg_point.aggregate(sig);
		}

		BLSSignature result;
		agg_point.to_affine().compress(result.m_array);
		return result;
	}

	BLSPublicKey Aggregate(const std::vector<const BLSPublicKey*>& pubKeys) {
		if (pubKeys.empty() || !pubKeys[0])
			return BLSPublicKey();

		blst::P1_Affine pk;
		auto res = pk.uncompress(pubKeys[0]->m_array);
		if (res != blst::BLST_SUCCESS) {
			CATAPULT_LOG(error) << "can't uncompress signature during aggregate 0 " << *pubKeys[0];
			return BLSPublicKey();
		}

		blst::P1 agg_point(pk);
		for (auto i = 1u; i < pubKeys.size(); ++i) {
			if (!pubKeys[i])
				return BLSPublicKey();

			res = pk.uncompress(pubKeys[i]->m_array);
			if (res != blst::BLST_SUCCESS) {
				CATAPULT_LOG(error) << "can't uncompress signature during aggregate " << i << ' ' << *pubKeys[i];
				return BLSPublicKey();
			}
			agg_point.aggregate(pk);
		}

		BLSPublicKey result;
		agg_point.to_affine().compress(result.m_array);
		return result;
	}

	bool AggregateVerify(const std::vector<const BLSPublicKey*>& publicKeys,
						 const std::vector<RawBuffer>& dataBuffers,
						 const BLSSignature& signature) {
		if (publicKeys.size() != dataBuffers.size())
			return false;

		blst::blst_p2_affine sig;
		auto res = blst::blst_p2_uncompress(&sig, signature.m_array);
		if (res != blst::BLST_SUCCESS) {
			CATAPULT_LOG(error) << "can't uncompress signature during aggregate verify " << signature;
			return false;
		}

		uint64_t mempool[blst::blst_pairing_sizeof() / sizeof(uint64_t)];
		memset(mempool, 0, sizeof(mempool));
		blst::blst_pairing* pairing = reinterpret_cast<blst::blst_pairing*>(&mempool);
		blst::blst_pairing_init(pairing, true, reinterpret_cast<const uint8_t*>(FILECOIN_DST.data()), FILECOIN_DST.size());
		for (auto i = 0u; i < publicKeys.size(); ++i) {
			if (!publicKeys[i])
				return false;

			blst::blst_p1_affine pk;
			res = blst::blst_p1_uncompress(&pk, publicKeys[i]->m_array);
			if (res != blst::BLST_SUCCESS) {
				CATAPULT_LOG(error) << "can't uncompress pk during aggregate verify " << i << ' ' << publicKeys[i];
				return false;
			}
			blst::blst_pairing_aggregate_pk_in_g1(pairing, &pk, nullptr, dataBuffers[i].pData, dataBuffers[i].Size);
		}

		blst::blst_pairing_commit(pairing);
		blst::blst_fp12 pt;
		blst::blst_aggregated_in_g2(&pt, &sig);
		return blst::blst_pairing_finalverify(pairing, &pt);
	}

	bool FastAggregateVerify(const std::vector<const BLSPublicKey*>& publicKeys, const RawBuffer& dataBuffer, const BLSSignature& signature) {
		auto aggregatedPK = Aggregate(publicKeys);
		return Verify(aggregatedPK, dataBuffer, signature);
	}
}}
