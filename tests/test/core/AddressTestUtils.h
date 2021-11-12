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
#include "catapult/crypto/KeyPair.h"
#include "catapult/crypto/PrivateKey.h"
#include "catapult/model/ContainerTypes.h"
#include "catapult/model/NetworkInfo.h"
#include <memory>
#include "catapult/types.h"

namespace catapult { namespace test {

	/// Generates a random private key.
	crypto::PrivateKey GenerateRandomPrivateKey();

	/// Generates a random key pair based on an account version
	crypto::KeyPair GenerateKeyPair(uint32_t version = 1);

	/// Generates a random key pair based on a derivation scheme
	crypto::KeyPair GenerateKeyPair(DerivationScheme type);

	/// Generates a random VRF key pair based on a KeyHashingType
	crypto::KeyPair GenerateVrfKeyPair();

	/// Generates a random address.
	Address GenerateRandomAddress();

	/// Generates a random address for a given network with id (\a networkIdentifier).
	Address GenerateRandomAddress(model::NetworkIdentifier networkIdentifier);

	/// Generates a random (unresolved) address.
	UnresolvedAddress GenerateRandomUnresolvedAddress();

	/// Generates a random (unresolved) address for a given network with id (\a networkIdentifier).
	UnresolvedAddress GenerateRandomUnresolvedAddress(model::NetworkIdentifier networkIdentifier);

	/// Generates a random set of \a count (unresolved) addresses.
	std::shared_ptr<model::UnresolvedAddressSet> GenerateRandomUnresolvedAddressSetPointer(size_t count);
}}
