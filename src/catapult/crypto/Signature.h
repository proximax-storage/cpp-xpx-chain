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
#include "catapult/exceptions.h"
#include <cstring>
#include "KeyPair.h"

namespace catapult { namespace crypto {

	class SignatureFeatureSolver
	{
	public:
		SignatureFeatureSolver() = default;

	public:

		static const DerivationScheme GetDerivationScheme(const Signature& signature);

		static void SetDerivationScheme(Signature& signature, DerivationScheme scheme);

		static const RawSignature& GetRawSignature(const Signature& signature);

		static RawSignature& GetRawSignature(Signature& signature);

		static Signature ExpandSignature(const RawSignature& signature, DerivationScheme scheme);
		/// Signs the \a dataBuffer outputting the \a signature with the given \a keyPair
		static void Sign(const KeyPair& keyPair, const RawBuffer& dataBuffer, RawSignature& signature);

		/// Signs the \a dataBuffer outputting the \a signature with the given \a keyPair
		static void Sign(const KeyPair& keyPair, const RawBuffer& dataBuffer, Signature& signature);

		/// Signs the \a dataBuffer outputting the \a signature with the given \a keyPair
		static void Sign(const KeyPair& keyPair, std::initializer_list<const RawBuffer> dataBuffer, Signature& signature);

		/// Signs the \a buffersList outputting the \a signature with the given \a keyPair
		/// Perf note: Evaluate whether to replace functors with code duplication
		static void Sign(const KeyPair& keyPair, std::initializer_list<const RawBuffer> buffersList, RawSignature& signature);

		/// Verifies the \a buffers with the \a signature for the given \a publicKey
		static bool Verify(const Key& publicKey, std::initializer_list<const RawBuffer> buffers, Signature& signature);

		/// Verifies the \a dataBuffer with the \a signature for the given \a publicKey
		static bool Verify(const Key& publicKey, const RawBuffer& dataBuffer, Signature& signature);

		/// Verifies the \a buffers with the \a signature for the given \a publicKey
		static bool Verify(const Key& publicKey, std::initializer_list<const RawBuffer> buffers, const RawSignature& signature, DerivationScheme derivationScheme);
	};
}}
