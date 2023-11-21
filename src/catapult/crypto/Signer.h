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
#include "KeyPair.h"
#include <vector>

namespace catapult { namespace crypto {

	/// Signs data pointed by \a dataBuffer using \a keyPair, placing resulting signature in \a computedSignature.
	/// \note The function will throw if the generated S part of the signature is not less than the group order.
	void Sign(const KeyPair& keyPair, const RawBuffer& dataBuffer, Signature& computedSignature);

	/// Signs data in \a buffersList using \a keyPair, placing resulting signature in \a computedSignature.
	/// \note The function will throw if the generated S part of the signature is not less than the group order.
	void Sign(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, Signature& computedSignature);

	/// Signs data in \a message using \a keyPair by BLS algorithm, placing resulting BLS signature in \a computedSignature.
	void Sign(const BLSKeyPair& keyPair, const RawBuffer& message, BLSSignature& computedSignature);

	/// Verifies that \a signature of data pointed by \a dataBuffer is valid, using public key \a publicKey.
	/// Returns \c true if signature is valid.
	bool Verify(const Key& publicKey, const RawBuffer& dataBuffer, const Signature& signature);

	/// Verifies that \a signature of data in \a buffersList is valid, using public key \a publicKey.
	/// Returns \c true if signature is valid.
	bool Verify(const Key& publicKey, std::initializer_list<const RawBuffer> buffersList, const Signature& signature);

	/// Verifies that \a BLS signature of data pointed by \a dataBuffer is valid, using BLS public key \a publicKey.
	/// Returns \c true if signature is valid.
	bool Verify(const BLSPublicKey& publicKey, const RawBuffer& dataBuffer, const BLSSignature& signature);

	/// Aggregates \a BLS signatures to one BLS signature.
	/// Returns \c aggregated BLS signature.
	BLSSignature Aggregate(const std::vector<const BLSSignature*>& signatures);

	/// Aggregates \a BLS public keys to one BLS public key.
	/// Returns \c aggregated BLS public key.
	BLSPublicKey Aggregate(const std::vector<const BLSPublicKey*>& pubKeys);

	/// Verifies that BLS \a signature of data pointed by \a dataBuffers is valid, using BLS public keys \a publicKeys.
	/// Returns \c true if signature is valid.
	bool AggregateVerify(const std::vector<const BLSPublicKey*>& publicKeys, const std::vector<RawBuffer>& dataBuffers, const BLSSignature& signature);

	/// Verifies that BLS \a signature of data pointed by \a dataBuffer is valid, using BLS public keys \a publicKeys.
	/// Returns \c true if signature is valid.
	bool FastAggregateVerify(const std::vector<const BLSPublicKey*>& publicKeys, const RawBuffer& dataBuffer, const BLSSignature& signature);
}}
