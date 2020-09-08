/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoOperationTestTraits.h"
#include "mongo/src/MongoStorageContext.h"

namespace catapult { namespace test {

	cache::CatapultCache MongoOperationTestTraits::CreateCatapultCache() {
		return OperationCacheFactory::Create();
	}

	std::unique_ptr<mongo::ExternalCacheStorage> MongoOperationTestTraits::CreateMongoCacheStorage(
			mongo::MongoStorageContext& context) {
		return mongo::plugins::CreateMongoOperationCacheStorage(
				context,
				config::CreateMockConfigurationHolder());
	}
}}
