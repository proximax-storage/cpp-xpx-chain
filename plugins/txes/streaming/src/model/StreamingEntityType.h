/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif

	/// Stream Start transaction.
	DEFINE_TRANSACTION_TYPE(Streaming, StreamStart, 0x1);

	/// Stream Finish transaction.
	DEFINE_TRANSACTION_TYPE(Streaming, StreamFinish, 0x2);

	/// Stream Payment transaction.
	DEFINE_TRANSACTION_TYPE(Streaming, StreamPayment, 0x3);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
