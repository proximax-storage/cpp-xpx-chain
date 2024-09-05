/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif

	/// Add DBRB process transaction.
	DEFINE_TRANSACTION_TYPE(Dbrb, AddDbrbProcess, 0x1);

	/// Remove DBRB process transaction.
	DEFINE_TRANSACTION_TYPE(Dbrb, RemoveDbrbProcess, 0x2);

	/// Remove DBRB process by network transaction.
	DEFINE_TRANSACTION_TYPE(Dbrb, RemoveDbrbProcessByNetwork, 0x3);

	/// Add or update DBRB process transaction.
	DEFINE_TRANSACTION_TYPE(Dbrb, AddOrUpdateDbrbProcess, 0x4);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
	}}
#endif
