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
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a contract validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_CONTRACT_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Contract, DESCRIPTION, CODE, None)

	/// Validation failed because the modification type is unsupported.
	DEFINE_CONTRACT_RESULT(Modify_Customer_Unsupported_Modification_Type, 1);

	/// Validation failed because customer is specified to be both added and removed.
	DEFINE_CONTRACT_RESULT(Modify_Customer_In_Both_Sets, 2);

	/// Validation failed because redundant modifications are present.
	DEFINE_CONTRACT_RESULT(Modify_Customer_Redundant_Modifications, 3);

	/// Validation failed because the modification type is unsupported.
	DEFINE_CONTRACT_RESULT(Modify_Executor_Unsupported_Modification_Type, 4);

	/// Validation failed because executor is specified to be both added and removed.
	DEFINE_CONTRACT_RESULT(Modify_Executor_In_Both_Sets, 5);

	/// Validation failed because redundant modifications are present.
	DEFINE_CONTRACT_RESULT(Modify_Executor_Redundant_Modifications, 6);

	/// Validation failed because the modification type is unsupported.
	DEFINE_CONTRACT_RESULT(Modify_Verifier_Unsupported_Modification_Type, 7);

	/// Validation failed because verifier is specified to be both added and removed.
	DEFINE_CONTRACT_RESULT(Modify_Verifier_In_Both_Sets, 8);

	/// Validation failed because redundant modifications are present.
	DEFINE_CONTRACT_RESULT(Modify_Verifier_Redundant_Modifications, 9);

	/// Validation failed because customer to be removed is not present.
	DEFINE_CONTRACT_RESULT(Modify_Not_A_Customer, 10);

	/// Validation failed because account to be added is already a customer.
	DEFINE_CONTRACT_RESULT(Modify_Already_A_Customer, 11);

	/// Validation failed because executor to be removed is not present.
	DEFINE_CONTRACT_RESULT(Modify_Not_A_Executor, 12);

	/// Validation failed because account to be added is already a executor.
	DEFINE_CONTRACT_RESULT(Modify_Already_A_Executor, 13);

	/// Validation failed because verifier to be removed is not present.
	DEFINE_CONTRACT_RESULT(Modify_Not_A_Verifier, 14);

	/// Validation failed because account to be added is already a verifier.
	DEFINE_CONTRACT_RESULT(Modify_Already_A_Verifier, 15);

	/// Validation failed because duration is invalid.
	DEFINE_CONTRACT_RESULT(Modify_Invalid_Duration, 16);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
