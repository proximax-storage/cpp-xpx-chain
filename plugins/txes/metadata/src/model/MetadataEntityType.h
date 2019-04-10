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

	/// Address metadata transaction.
	DEFINE_TRANSACTION_TYPE(Metadata, Address_Metadata, 0x1);

	/// Mosaic metadata transaction.
	DEFINE_TRANSACTION_TYPE(Metadata, Mosaic_Metadata, 0x2);

	/// Namespace metadata transaction.
	DEFINE_TRANSACTION_TYPE(Metadata, Namespace_Metadata, 0x3);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
