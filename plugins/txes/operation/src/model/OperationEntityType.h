/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif

	/// Operation identify transaction.
	DEFINE_TRANSACTION_TYPE(Operation, OperationIdentify, 0x1);

	/// Start operation transaction.
	DEFINE_TRANSACTION_TYPE(Operation, StartOperation, 0x2);

	/// End operation transaction.
	DEFINE_TRANSACTION_TYPE(Operation, EndOperation, 0x3);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
