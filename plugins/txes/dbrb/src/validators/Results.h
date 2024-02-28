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

		/// Registration attempt is too early, DBRB process is not yet expired.
		DEFINE_DBRB_RESULT(Process_Not_Expired, 1);

		/// Node removal attempt is too far in future.
		DEFINE_DBRB_RESULT(Node_Removal_Too_Far_In_Future, 2);

		/// Node removal attempt is too far in past.
		DEFINE_DBRB_RESULT(Node_Removal_Too_Far_In_Past, 3);

		/// The process to be removed is not in DBRB system.
		DEFINE_DBRB_RESULT(Node_Removal_Subject_Is_Not_In_Dbrb_System, 4);

		/// Node removal request has not enough votes.
		DEFINE_DBRB_RESULT(Node_Removal_Not_Enough_Votes, 5);

		/// Voter is not in DBRB system.
		DEFINE_DBRB_RESULT(Node_Removal_Voter_Is_Not_In_Dbrb_System, 6);

		/// Voter's signature of node removal is invalid.
		DEFINE_DBRB_RESULT(Node_Removal_Invalid_Signature, 7);

#ifndef CUSTOM_RESULT_DEFINITION
	}}
#endif
