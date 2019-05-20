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

#include "Validators.h"
#include "src/cache/ContractCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyContractNotification<1>;

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
