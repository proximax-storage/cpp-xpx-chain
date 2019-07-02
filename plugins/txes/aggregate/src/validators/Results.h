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
/// Defines an aggregate validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_AGGREGATE_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Aggregate, DESCRIPTION, CODE, None)

	/// Validation failed because aggregate has too many transactions.
	DEFINE_AGGREGATE_RESULT(Too_Many_Transactions, 0x0001);

	/// Validation failed because aggregate does not have any transactions.
	DEFINE_AGGREGATE_RESULT(No_Transactions, 0x0002);

	/// Validation failed because aggregate has too many cosignatures.
	DEFINE_AGGREGATE_RESULT(Too_Many_Cosignatures, 0x0003);

	/// Validation failed because redundant cosignatures are present.
	DEFINE_AGGREGATE_RESULT(Redundant_Cosignatures, 0x0004);

	/// Validation failed because at least one cosigner is ineligible.
	DEFINE_AGGREGATE_RESULT(Ineligible_Cosigners, 0x1001);

	/// Validation failed because at least one required cosigner is missing.
	DEFINE_AGGREGATE_RESULT(Missing_Cosigners, 0x1002);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_AGGREGATE_RESULT(Plugin_Config_Malformed, 0x0005);

	/// Validation failed because aggregate bonded transaction is not enabled.
	DEFINE_AGGREGATE_RESULT(Bonded_Not_Enabled, 0x0006);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
