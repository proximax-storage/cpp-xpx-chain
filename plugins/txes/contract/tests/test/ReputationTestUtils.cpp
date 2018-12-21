/**
*** Copyright (c) 2018-present,
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

#include "ReputationTestUtils.h"
#include "src/cache/ReputationCache.h"
#include "catapult/cache/CatapultCacheDelta.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	std::vector<Key> GenerateKeys(size_t count) {
		return test::GenerateRandomDataVector<Key>(count);
	}

	std::unique_ptr<model::EmbeddedModifyContractTransaction> CreateModifyContractTransaction(
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

		auto* pModification = pTransaction->ExecutorsPtr();
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
