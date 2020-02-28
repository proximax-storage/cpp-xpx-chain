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

	/// Start execute transaction.
	DEFINE_TRANSACTION_TYPE(SuperContract, StartExecute, 0x2);

	/// End execute transaction.
	DEFINE_TRANSACTION_TYPE(SuperContract, EndExecute, 0x3);

	/// Upload file transaction.
	DEFINE_TRANSACTION_TYPE(SuperContract, UploadFile, 0x4);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
