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

/// Defines a property validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_PROPERTY_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Property, DESCRIPTION, CODE, None)

	/// Validation failed because the property type is invalid.
	DEFINE_PROPERTY_RESULT(Invalid_Property_Type, 1);

	/// Validation failed because a modification type is invalid.
	DEFINE_PROPERTY_RESULT(Modification_Type_Invalid, 2);

	/// Validation failed because a modification address is invalid.
	DEFINE_PROPERTY_RESULT(Modification_Address_Invalid, 3);

	/// Validation failed because the operation type is incompatible.
	/// \node This indicates that the values container is non-empty and the operation types for property and notification differ.
	DEFINE_PROPERTY_RESULT(Modification_Operation_Type_Incompatible, 4);

	/// Validation failed because the modification type is unsupported.
	DEFINE_PROPERTY_RESULT(Modify_Unsupported_Modification_Type, 5);

	/// Validation failed because a modification is redundant.
	DEFINE_PROPERTY_RESULT(Modification_Redundant, 6);

	/// Validation failed because a value is not in the container.
	DEFINE_PROPERTY_RESULT(Modification_Not_Allowed, 7);

	/// Validation failed because the transaction has too many modifications.
	DEFINE_PROPERTY_RESULT(Modification_Count_Exceeded, 8);

	/// Validation failed because the resulting property has too many values.
	DEFINE_PROPERTY_RESULT(Values_Count_Exceeded, 9);

	/// Validation failed because the property value is invalid.
	DEFINE_PROPERTY_RESULT(Value_Invalid, 10);

	/// Validation failed because the signer is not allowed to interact with an address involved in the transaction.
	DEFINE_PROPERTY_RESULT(Signer_Address_Interaction_Not_Allowed, 11);

	/// Validation failed because the mosaic transfer is prohibited by the recipient.
	DEFINE_PROPERTY_RESULT(Mosaic_Transfer_Not_Allowed, 12);

	/// Validation failed because the transaction type is not allowed to be initiated by the signer.
	DEFINE_PROPERTY_RESULT(Transaction_Type_Not_Allowed, 13);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
