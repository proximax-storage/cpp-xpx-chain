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

	/// PrepareDrive transaction.
	DEFINE_TRANSACTION_TYPE(Storage, PrepareDrive, 0x1);

	/// DataModification transaction.
	DEFINE_TRANSACTION_TYPE(Storage, DataModification, 0x2);

	/// Download transaction.
	DEFINE_TRANSACTION_TYPE(Storage, Download, 0x3);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
