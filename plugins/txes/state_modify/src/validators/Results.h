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
/// Defines a namespace validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_STATE_MODIFY_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, ModifyState, DESCRIPTION, CODE, None)

	// region common

	/// Validation failed because the signer is not nemesis.
	DEFINE_STATE_MODIFY_RESULT(Signer_Not_Nemesis, 1);

	/// Validation failed because the cache id is not valid.
	DEFINE_STATE_MODIFY_RESULT(Cache_Id_Invalid, 2);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_STATE_MODIFY_RESULT(Plugin_Config_Malformed, 7);
	// endregion


#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
