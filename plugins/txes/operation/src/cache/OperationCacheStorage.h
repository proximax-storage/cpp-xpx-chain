/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationCacheTypes.h"
#include "src/state/OperationEntrySerializer.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheStorage.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading operation cache data.
	struct OperationCacheStorage : public LockInfoCacheStorage<OperationCacheDescriptor, state::OperationEntrySerializer> {};
}}
