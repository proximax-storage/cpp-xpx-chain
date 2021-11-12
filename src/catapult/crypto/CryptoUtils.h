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
#include "catapult/types.h"

namespace catapult { namespace crypto { class PrivateKey; } }

struct ge25519_t;
using ge25519 = ge25519_t;

namespace catapult { namespace crypto {

	/// Multiplier for scalar multiplication.
	using ScalarMultiplier = uint8_t[32];

	/// bignum256modm type definition.
	using bignum256modm_type = uint64_t[5];

	/// Returns \c true if the y coordinate of \a publicKey is smaller than 2^255 - 19.
	bool IsCanonicalKey(const Key& publicKey);

	/// Returns \c true if \a publicKey is the neutral element of the group.
	bool IsNeutralElement(const Key& publicKey);

	template<DerivationScheme TDerivationScheme>
	void BuildHash(Hash512& hash, std::initializer_list<const RawBuffer> initBuffers, const std::initializer_list<const RawBuffer>& buffersList);
	template<>
	void BuildHash<DerivationScheme::Ed25519_Sha3>(Hash512& hash, std::initializer_list<const RawBuffer> initBuffers, const std::initializer_list<const RawBuffer>& buffersList);
	template<>
	void BuildHash<DerivationScheme::Ed25519_Sha2>(Hash512& hash, std::initializer_list<const RawBuffer> initBuffers, const std::initializer_list<const RawBuffer>& buffersList);

	/// Extracts the \a multiplier used to derive the public key from \a privateKey.
	template<DerivationScheme TDerivationScheme>
	void ExtractMultiplier(const PrivateKey& privateKey, ScalarMultiplier& multiplier);

	/// Generates \a nonce from \a privateKey and a list of buffers (\a buffersList).
	template<DerivationScheme TDerivationScheme>
	void GenerateNonce(const PrivateKey& privateKey, std::initializer_list<const RawBuffer> buffersList, bignum256modm_type& nonce);


	/// Constant time scalar multiplication of \a publicKey with \a multiplier. The result is stored in \a sharedSecret.
	bool ScalarMult(const ScalarMultiplier& multiplier, const Key& publicKey, Key& sharedSecret);

	/// Calculates \a hash of a \a privateKey.
	template<DerivationScheme TDerivationScheme>
	void HashPrivateKey(const PrivateKey& privateKey, Hash512& hash);
	template<>
	void HashPrivateKey<DerivationScheme::Ed25519_Sha2>(const PrivateKey& privateKey, Hash512& hash);
	template<>
	void HashPrivateKey<DerivationScheme::Ed25519_Sha3>(const PrivateKey& privateKey, Hash512& hash);


	/// Unpacks inverse of \a publicKey into \a A and validates that:
	/// - publicKey is canonical
	/// - A is on the curve
	bool UnpackNegative(ge25519& A, const Key& publicKey);

	/// Unpacks inverse of \a publicKey into \a A and validates that:
	/// - publicKey is canonical
	/// - A is on the curve
	/// - A is in main subgroup
	bool UnpackNegativeAndCheckSubgroup(ge25519& A, const Key& publicKey);

	template<DerivationScheme TDerivationScheme>
	void GenerateNonceAndHash(const PrivateKey& privateKey, std::initializer_list<const RawBuffer>& buffersList, bignum256modm_type& nonce, Hash512& hash)
	{
		GenerateNonce<TDerivationScheme>(privateKey, buffersList, nonce);
		HashPrivateKey<TDerivationScheme>(privateKey, hash);
	}


	extern template void HashPrivateKey<DerivationScheme::Ed25519_Sha3>(const PrivateKey& privateKey, Hash512& hash);
	extern template void HashPrivateKey<DerivationScheme::Ed25519_Sha2>(const PrivateKey& privateKey, Hash512& hash);
	extern template void BuildHash<DerivationScheme::Ed25519_Sha2>(Hash512& hash, std::initializer_list<const RawBuffer> initBuffers, const std::initializer_list<const RawBuffer>& buffersList);
	extern template void BuildHash<DerivationScheme::Ed25519_Sha3>(Hash512& hash, std::initializer_list<const RawBuffer> initBuffers, const std::initializer_list<const RawBuffer>& buffersList);
	extern template void GenerateNonce<DerivationScheme::Ed25519_Sha2>(const PrivateKey& privateKey, std::initializer_list<const RawBuffer> buffersList, bignum256modm_type& nonce);
	extern template void GenerateNonce<DerivationScheme::Ed25519_Sha3>(const PrivateKey& privateKey, std::initializer_list<const RawBuffer> buffersList, bignum256modm_type& nonce);
	extern template void ExtractMultiplier<DerivationScheme::Ed25519_Sha3>(const PrivateKey& privateKey, ScalarMultiplier& multiplier);
	extern template void ExtractMultiplier<DerivationScheme::Ed25519_Sha2>(const PrivateKey& privateKey, ScalarMultiplier& multiplier);

}}
