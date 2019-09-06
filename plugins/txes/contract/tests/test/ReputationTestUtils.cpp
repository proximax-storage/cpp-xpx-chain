/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReputationTestUtils.h"
#include "src/cache/ReputationCache.h"
#include "catapult/cache/CatapultCacheDelta.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	std::vector<Key> GenerateKeys(size_t count) {
		return test::GenerateRandomDataVector<Key>(count);
	}

	model::UniqueEntityPtr<model::EmbeddedModifyContractTransaction> CreateModifyContractTransaction(
			const Key& signer,
			const std::vector<model::CosignatoryModificationType>& modificationTypes) {
		using TransactionType = model::EmbeddedModifyContractTransaction;
		auto numModifications = static_cast<uint8_t>(modificationTypes.size());
		uint32_t entitySize = sizeof(TransactionType) + numModifications * sizeof(model::CosignatoryModification);
		auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
		pTransaction->Size = entitySize;
		pTransaction->ExecutorModificationCount = numModifications;
		pTransaction->Type = model::Entity_Type_Modify_Contract;
		pTransaction->Signer = signer;

		auto* pModification = pTransaction->ExecutorModificationsPtr();
		for (auto i = 0u; i < numModifications; ++i) {
			pModification->ModificationType = modificationTypes[i];
			test::FillWithRandomData(pModification->CosignatoryPublicKey);
			++pModification;
		}

		return pTransaction;
	}

	namespace {
		state::ReputationEntry& GetOrCreateEntry(cache::ReputationCacheDelta& reputationCache, const Key& key) {
			if (!reputationCache.contains(key))
				reputationCache.insert(state::ReputationEntry(key));

			return reputationCache.find(key).get();
		}
	}

	void MakeReputation(
			cache::CatapultCacheDelta& cache,
			const Key& key,
			uint64_t positiveInteractions,
			uint64_t negativeInteractions) {
		auto& reputationCache = cache.sub<cache::ReputationCache>();

		auto& reputationEntry = GetOrCreateEntry(reputationCache, key);
		reputationEntry.setPositiveInteractions(Reputation{positiveInteractions});
		reputationEntry.setNegativeInteractions(Reputation{negativeInteractions});
	}
}}
