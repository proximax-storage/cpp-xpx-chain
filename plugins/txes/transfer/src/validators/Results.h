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
/// Defines a transfer validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_TRANSFER_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Transfer, DESCRIPTION, CODE, None)

	/// Validation failed because the message is too large.
	DEFINE_TRANSFER_RESULT(Message_Too_Large, 6);

	/// Validation failed because mosaics are out of order.
	DEFINE_TRANSFER_RESULT(Out_Of_Order_Mosaics, 200);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_TRANSFER_RESULT(Plugin_Config_Malformed, 7);

	/// Validation failed because number of mosaics exceeded the limit.
	DEFINE_TRANSFER_RESULT(Too_Many_Mosaics, 8);

	/// Validation failed because mosaic amount is zero.
	DEFINE_TRANSFER_RESULT(Zero_Amount, 9);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
