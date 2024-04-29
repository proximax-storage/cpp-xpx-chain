/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/dbrb/DbrbDefinitions.h"

namespace catapult::observers { struct ObserverContext; }

namespace catapult::observers {

class DbrbProcessUpdateListener {
public:
	virtual ~DbrbProcessUpdateListener() = default;

	virtual void OnDbrbProcessRemoved(ObserverContext& context, const dbrb::ProcessId& processId) const = 0;
};
}