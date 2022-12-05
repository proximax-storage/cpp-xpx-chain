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

	/// Deploy contract transaction.
	DEFINE_TRANSACTION_TYPE(SuperContract_v2, DeployContractTransaction, 0x1);

	/// Manual call transaction.
	DEFINE_TRANSACTION_TYPE(SuperContract_v2, ManualCallTransaction, 0x2);

	/// Automatic Executions Payment transaction.
	DEFINE_TRANSACTION_TYPE(SuperContract_v2, AutomaticExecutionsPaymentTransaction, 0x3);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
