/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a DBRB validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_DBRB_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Dbrb, DESCRIPTION, CODE, None)

		/// View sequence is already in the cache.
		DEFINE_DBRB_RESULT(View_Sequence_Already_Exists, 1);

		/// View sequence with the given hash is not in the cache.
		DEFINE_DBRB_RESULT(View_Sequence_Not_Found, 2);

		/// Supplied sequence is too short.
		DEFINE_DBRB_RESULT(View_Sequence_Size_Insufficient, 3);

		/// Supplied replaced view is not the same as the most recent view stored in the cache.
		DEFINE_DBRB_RESULT(Invalid_Replaced_View, 4);

		/// There are not enough signatures to form a quorum.
		DEFINE_DBRB_RESULT(Signatures_Count_Insufficient, 5);

		/// Supplied signature is invalid.
		DEFINE_DBRB_RESULT(Invalid_Signature, 6);

#ifndef CUSTOM_RESULT_DEFINITION
	}}
#endif
