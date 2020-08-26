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
	/// Add harvester transaction.
	DEFINE_TRANSACTION_TYPE(Committee, AddHarvester, 0x1);

	/// Remove harvester transaction.
	DEFINE_TRANSACTION_TYPE(Committee, RemoveHarvester, 0x2);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
