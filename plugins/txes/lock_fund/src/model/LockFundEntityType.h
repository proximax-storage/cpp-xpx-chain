/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif

	/// Lock Fund transaction.
	DEFINE_TRANSACTION_TYPE(LockFund, Lock_Fund_Transfer, 0x1);

	/// Lock Fund transaction.
	DEFINE_TRANSACTION_TYPE(LockFund, Lock_Fund_Cancel_Unlock, 0x2);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
