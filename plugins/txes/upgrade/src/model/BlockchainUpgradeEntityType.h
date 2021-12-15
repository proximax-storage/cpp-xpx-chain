/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif
	/// Blockchain upgrade transaction.
	DEFINE_TRANSACTION_TYPE(BlockchainUpgrade, Blockchain_Upgrade, 0x1);

	/// Account V1 to V2 upgrade transaction.
	DEFINE_TRANSACTION_TYPE(BlockchainUpgrade, AccountV2_Upgrade, 0x2);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
