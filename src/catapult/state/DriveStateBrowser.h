/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Elements.h"
#include "catapult/types.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/NonCopyable.h"
#include <vector>
#include <optional>

namespace catapult { namespace cache {
	class ReadOnlyCatapultCache;
}}

namespace catapult::state {

/// Interface for drive state.
class DriveStateBrowser : public utils::NonCopyable {

public:

	virtual ~DriveStateBrowser() = default;

	virtual uint16_t getOrderedReplicatorsCount(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const = 0;

	virtual Key getDriveOwner(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const = 0;
};
}