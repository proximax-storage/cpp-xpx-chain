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

namespace catapult { namespace chain {

#endif
/// Defines a chain validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_CHAIN_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Chain, DESCRIPTION, CODE, None)

	/// Validation failed because a block was received that did not link with the existing chain.
	DEFINE_CHAIN_RESULT(Unlinked, 102);

	/// Validation failed because a block was received that is not a hit.
	DEFINE_CHAIN_RESULT(Block_Not_Hit, 104);

	/// Validation failed because a block was received that has an inconsistent state hash.
	DEFINE_CHAIN_RESULT(Block_Inconsistent_State_Hash, 105);

	/// Validation failed because a block was received that has an inconsistent receipts hash.
	DEFINE_CHAIN_RESULT(Block_Inconsistent_Receipts_Hash, 106);

	/// Validation failed because the unconfirmed cache is too full.
	DEFINE_CHAIN_RESULT(Unconfirmed_Cache_Too_Full, 201);

	/// Validation failed because the transaction is expired.
	DEFINE_CHAIN_RESULT(Transaction_Expired, 202);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
