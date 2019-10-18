/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EmbeddedTransactionHasher.h"
#include "catapult/crypto/Hashes.h"

namespace catapult { namespace model {

	Hash256 CalculateHash(const EmbeddedTransaction& transaction, const GenerationHash& generationHash) {
		Hash256 entityHash;
		crypto::Sha3_256_Builder sha3;
		sha3.update(transaction.Signer);
		sha3.update(generationHash);
		sha3.update({reinterpret_cast<const uint8_t *>(&transaction), transaction.Size});
		sha3.final(entityHash);
		return entityHash;
	}
}}
