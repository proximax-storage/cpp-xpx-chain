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
#include "src/model/ModifyMultisigAccountAndReputationTransaction.h"
#include "catapult/model/Cosignature.h"
#include "catapult/utils/HexFormatter.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache { class CatapultCacheDelta; } }

namespace catapult { namespace test {

	/// Generates \a count random keys.
	std::vector<Key> GenerateKeys(size_t count);

	/// Creates a modify multisig account and reputation transaction from \a signer with \a modificationTypes.
	std::unique_ptr<model::EmbeddedModifyMultisigAccountAndReputationTransaction> CreateModifyMultisigAccountAndReputationTransaction(
			const Key& signer,
			const std::vector<model::CosignatoryModificationType>& modificationTypes);

	/// Makes \a key in \a cache with \a positiveInteractions and \a negativeInteractions.
	void MakeReputation(
			cache::CatapultCacheDelta& cache,
			const Key& key,
			uint64_t positiveInteractions = 0,
			uint64_t negativeInteractions = 0);
}}
