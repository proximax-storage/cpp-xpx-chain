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

#include "AggregateTransactionTestUtils.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/EntityTestUtils.h"
#include "tests/TestHarness.h"
#include <vector>
#include "catapult/crypto/Signature.h"

namespace catapult { namespace test {


	template<>
	model::DetachedCosignature GenerateValidCosignature(const crypto::KeyPair& keyPair, const Hash256& aggregateHash) {
		RawSignature signature;
		crypto::SignatureFeatureSolver::Sign(keyPair, aggregateHash, signature);
		return { keyPair.publicKey(), signature, keyPair.derivationScheme(), aggregateHash };
	}
	template<>
	model::Cosignature<SignatureLayout::Raw> GenerateValidCosignature(const crypto::KeyPair& keyPair, const Hash256& aggregateHash) {
		RawSignature signature;
		crypto::SignatureFeatureSolver::Sign(keyPair, aggregateHash, signature);
		return { keyPair.publicKey(), signature };
	}
	template<>
	model::Cosignature<SignatureLayout::Extended> GenerateValidCosignature(const crypto::KeyPair& keyPair, const Hash256& aggregateHash) {
		RawSignature signature;
		crypto::SignatureFeatureSolver::Sign(keyPair, aggregateHash, signature);
		return { keyPair.publicKey(), crypto::SignatureFeatureSolver::ExpandSignature(signature, keyPair.derivationScheme()) };
	}

	template<>
	CosignaturesMap ToMap<model::CosignatureInfo>(const std::vector<model::CosignatureInfo>& cosignatures) {
		CosignaturesMap cosignaturesMap;
		for (const auto& cosignature : cosignatures)
			cosignaturesMap.emplace(cosignature.Signer, cosignature.GetRawSignature());

		return cosignaturesMap;
	}

}}
