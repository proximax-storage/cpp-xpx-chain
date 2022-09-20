/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MapperInclude.h"
#include <functional>
#include <memory>

namespace catapult { namespace state { struct AccountState; } }

namespace catapult { namespace mongo { namespace mappers {

	/// Maps a staking pair (\a stakingPair) to the corresponding db model value.
	bsoncxx::document::value ToDbModel(const state::StakingRecord& stakingRecord);
}}}
