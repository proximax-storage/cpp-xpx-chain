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

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// A cosignature.
	template<uint32_t TVersion>
	struct Cosignature;

	template<>
	struct Cosignature<1> {
		/// Cosigner public key.
		Key Signer;

		/// Cosigner signature.
		catapult::RawSignature Signature;
	};

	template<>
	struct Cosignature<2> {
		/// Cosigner public key.
		Key Signer;

		/// Cosigner signature.
		catapult::Signature Signature;
	};

	/// A detached cosignature.
	template<uint32_t TVersion>
	struct DetachedCosignature : public Cosignature<TVersion> {
	public:
		/// Creates a detached cosignature around \a signer, \a signature and \a parentHash.
		DetachedCosignature(const Key& signer, const catapult::Signature& signature, const Hash256& parentHash)
				: Cosignature<TVersion>{ signer, signature }
				, ParentHash(parentHash)
		{}

	public:
		/// Hash of the corresponding parent.
		Hash256 ParentHash;
	};

#pragma pack(pop)
}}
