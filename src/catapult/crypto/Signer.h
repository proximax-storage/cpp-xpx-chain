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

	/// Returns the hashing algorythm used in this KeyPair.
	const SignatureVersion GetSignatureVersion(Signature signature);

	const RawSignature& GetRawSignature(const Signature& signature);

	Signature ExpandSignature(RawSignature& signature, SignatureVersion version);

	RawSignature& GetRawSignature(Signature& signature);
	/// Signs data pointed by \a dataBuffer using \a keyPair, placing resulting signature in \a computedSignature.
	/// \note The function will throw if the generated S part of the signature is not less than the group order.
	void Sign(const KeyPair& keyPair, const RawBuffer& dataBuffer, Signature& computedSignature);

	/// Signs data pointed by \a dataBuffer using \a keyPair, placing resulting raw signature in \a computedSignature.
	/// \note The function will throw if the generated S part of the signature is not less than the group order.
	void Sign(const KeyPair& keyPair, const RawBuffer& dataBuffer, RawSignature& computedSignature);

	/// Signs data in \a buffersList using \a keyPair, placing resulting signature in \a computedSignature.
	/// \note The function will throw if the generated S part of the signature is not less than the group order.
	void Sign(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, Signature& computedSignature);

	/// Signs data in \a buffersList using \a keyPair, placing resulting raw signature in \a computedSignature.
	/// \note The function will throw if the generated S part of the signature is not less than the group order.
	void Sign(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, RawSignature& computedSignature);

	/// Signs data in \a buffersList using \a keyPair, placing resulting signature in \a computedSignature based on Ref10 implementation.
	/// \note The function will throw if the generated S part of the signature is not less than the group order.
	void SignRef10(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, Signature& signature);

	/// Verifies that \a signature of data pointed by \a dataBuffer is valid, using public key \a publicKey.
	/// Returns \c true if signature is valid.
	bool Verify(const Key& publicKey, const RawBuffer& dataBuffer, const Signature& signature);


	/// Verifies that \a signature of data in \a buffersList is valid, using public key \a publicKey.
	/// Returns \c true if signature is valid.
	bool Verify(const Key& publicKey, std::initializer_list<const RawBuffer> buffers, const RawSignature& signature, KeyHashingType hashingType);

	/// Verifies that \a signature of data in \a buffersList is valid, using public key \a publicKey based on Ref10 implementation.
	/// Returns \c true if signature is valid.
	bool VerifyRef10(const Key& publicKey, std::initializer_list<const RawBuffer> buffersList, const RawSignature& signature, KeyHashingType hashingType);
}}
