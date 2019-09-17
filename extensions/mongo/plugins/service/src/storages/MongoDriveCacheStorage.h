/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/CacheStorageInclude.h"

namespace catapult { namespace mongo { namespace plugins {

	/// Creates a mongo drive cache storage around \a database, \a bulkWriter and \a networkIdentifier.
	DECLARE_MONGO_CACHE_STORAGE(Drive);
}}}
