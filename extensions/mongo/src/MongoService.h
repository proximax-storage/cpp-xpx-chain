/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/extensions/ServiceRegistrar.h"

namespace catapult { namespace mongo {
	struct MongoStorageContext;
	struct MongoTransactionRegistry;
}}

namespace catapult { namespace mongo {

	/// Creates a registrar for a mongo service.
	DECLARE_SERVICE_REGISTRAR(Mongo)(const std::shared_ptr<MongoStorageContext>& pContext, const std::shared_ptr<const MongoTransactionRegistry>& pRegistry);
}}
