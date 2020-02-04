/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RECEIPT_TYPE_DEFINITION
#include "catapult/model/ReceiptType.h"

namespace catapult { namespace model {

#endif

	/// Operation creation.
	DEFINE_RECEIPT_TYPE(Operation, Operation, Operation_Started, 1);

	/// Operation completion.
	DEFINE_RECEIPT_TYPE(Operation, Operation, Operation_Ended, 2);

	/// Operation expiration.
	DEFINE_RECEIPT_TYPE(Operation, Operation, Operation_Expired, 3);

#ifndef CUSTOM_RECEIPT_TYPE_DEFINITION
}}
#endif
