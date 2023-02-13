/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <optional>
#include <src/state/SuperContractEntry.h>
#include <catapult/config/BlockchainConfiguration.h>

namespace catapult::utils {

	std::optional<Height> automaticExecutionsEnabledSince(
			const state::SuperContractEntry& contractEntry,
			const Height& actualHeight,
			const config::BlockchainConfiguration& config);

}