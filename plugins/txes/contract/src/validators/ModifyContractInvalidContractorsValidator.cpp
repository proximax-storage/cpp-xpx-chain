/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/ContractCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyContractNotification;

#define DEFINE_INVALID_CONTRACTORS_VALIDATOR(CONTRACTOR_TYPE) \
	DEFINE_STATEFUL_VALIDATOR(ModifyContractInvalid##CONTRACTOR_TYPE##s, [](const auto& notification, const ValidatorContext& context) { \
		const auto& contractCache = context.Cache.sub<cache::ContractCache>(); \
		if (!contractCache.contains(notification.Multisig)) { \
			return ValidationResult::Success; \
		} \
		const auto* pModifications = notification.CONTRACTOR_TYPE##ModificationsPtr; \
		const auto& contractEntry = contractCache.find(notification.Multisig).get(); \
		for (auto i = 0u; i < notification.CONTRACTOR_TYPE##ModificationCount; ++i) { \
			auto isAdded = contractEntry.has##CONTRACTOR_TYPE(pModifications[i].CosignatoryPublicKey); \
			auto isAdding = model::CosignatoryModificationType::Add == pModifications[i].ModificationType; \
			if (!isAdding && !isAdded) \
				return Failure_Contract_Modify_Not_A_##CONTRACTOR_TYPE; \
			if (isAdding && isAdded) \
				return Failure_Contract_Modify_Already_A_##CONTRACTOR_TYPE; \
		} \
		return ValidationResult::Success; \
	});

	DEFINE_INVALID_CONTRACTORS_VALIDATOR(Customer)
	DEFINE_INVALID_CONTRACTORS_VALIDATOR(Executor)
	DEFINE_INVALID_CONTRACTORS_VALIDATOR(Verifier)
}}
