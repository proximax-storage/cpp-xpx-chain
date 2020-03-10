/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/OperationCacheTypes.h"
#include "src/storages/MongoOperationCacheStorage.h"
#include "tests/test/OperationTestUtils.h"

namespace catapult { namespace mongo { class MongoStorageContext; } }

namespace catapult { namespace test {

	/// Mongo traits for an operation.
	struct MongoOperationTestTraits : public BasicOperationTestTraits {
		/// Number of additional fields.
		static constexpr size_t Num_Additional_Fields = 3;

		/// Creates a catapult cache.
		static cache::CatapultCache CreateCatapultCache();

		/// Creates a mongo operation cache storage around \a context.
		static std::unique_ptr<mongo::ExternalCacheStorage> CreateMongoCacheStorage(mongo::MongoStorageContext& context);
	};
}}
