/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/ModifyContractTransaction.h"
#include "catapult/model/Cosignature.h"
#include "catapult/model/EntityPtr.h"
#include "catapult/utils/HexFormatter.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache { class CatapultCacheDelta; } }

namespace catapult { namespace test {

	/// Generates \a count random keys.
	std::vector<Key> GenerateKeys(size_t count);

	/// Creates a modify contract transaction from \a signer with \a modificationTypes.
	model::UniqueEntityPtr<model::EmbeddedModifyContractTransaction> CreateModifyContractTransaction(
			const Key& signer,
			const std::vector<model::CosignatoryModificationType>& modificationTypes);

	/// Makes \a key in \a cache with \a positiveInteractions and \a negativeInteractions.
	void MakeReputation(
			cache::CatapultCacheDelta& cache,
			const Key& key,
			uint64_t positiveInteractions = 0,
			uint64_t negativeInteractions = 0);
}}
