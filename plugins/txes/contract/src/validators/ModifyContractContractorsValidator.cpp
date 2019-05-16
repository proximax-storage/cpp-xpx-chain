/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/utils/ArraySet.h"
#include <unordered_set>

namespace catapult { namespace validators {

	using Notification = model::ModifyContractNotification;

	namespace {
		constexpr bool IsValidModificationType(model::CosignatoryModificationType type) {
			return model::CosignatoryModificationType::Add == type || model::CosignatoryModificationType::Del == type;
		}
	}

#define DEFINE_CONTRACTORS_VALIDATOR(CONTRACTOR_TYPE) \
	DEFINE_STATELESS_VALIDATOR(ModifyContract##CONTRACTOR_TYPE##s, [](const auto& notification) { \
		utils::KeyPointerSet addedAccounts; \
		utils::KeyPointerSet removedAccounts; \
		const auto* pModifications = notification.CONTRACTOR_TYPE##ModificationsPtr; \
		for (auto i = 0u; i < notification.CONTRACTOR_TYPE##ModificationCount; ++i) { \
			if (!IsValidModificationType(pModifications[i].ModificationType)) \
				return Failure_Contract_Modify_##CONTRACTOR_TYPE##_Unsupported_Modification_Type; \
			auto& accounts = model::CosignatoryModificationType::Add == pModifications[i].ModificationType \
					? addedAccounts \
					: removedAccounts; \
			const auto& oppositeAccounts = &accounts == &addedAccounts ? removedAccounts : addedAccounts; \
			auto& key = pModifications[i].CosignatoryPublicKey; \
			if (oppositeAccounts.end() != oppositeAccounts.find(&key)) \
				return Failure_Contract_Modify_##CONTRACTOR_TYPE##_In_Both_Sets; \
			accounts.insert(&key); \
		} \
		if (notification.CONTRACTOR_TYPE##ModificationCount != addedAccounts.size() + removedAccounts.size()) \
			return Failure_Contract_Modify_##CONTRACTOR_TYPE##_Redundant_Modifications; \
		return ValidationResult::Success; \
	});

	DEFINE_CONTRACTORS_VALIDATOR(Customer)
	DEFINE_CONTRACTORS_VALIDATOR(Executor)
	DEFINE_CONTRACTORS_VALIDATOR(Verifier)
}}
