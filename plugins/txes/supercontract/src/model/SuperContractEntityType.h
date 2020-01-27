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

	/// Deploy transaction.
	DEFINE_TRANSACTION_TYPE(SuperContract, Deploy, 0x1);

	/// Execute transaction.
	DEFINE_TRANSACTION_TYPE(SuperContract, Execute, 0x2);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
