/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <plugins/txes/supercontract_v2/src/config/SuperContractV2Configuration.h>
#include "ContractUtils.h"

namespace catapult::utils {

	std::optional<Height> automaticExecutionsEnabledSince(
			const state::SuperContractEntry& contractEntry,
			const Height& actualHeight,
			const config::BlockchainConfiguration& config) {
		const auto& prepaidSinceOpt = contractEntry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince;
		if (!prepaidSinceOpt) {
			return {};
		}
		auto prepaidSince = *prepaidSinceOpt;

		const auto& pluginConfig = config.Network.template GetPluginConfiguration<config::SuperContractV2Configuration>();
		Height enabledLimit;
		if (actualHeight > pluginConfig.AutomaticExecutionsDeadline) {
			enabledLimit = actualHeight - pluginConfig.AutomaticExecutionsDeadline;
		}
		return std::max(prepaidSince, enabledLimit);
	}

} // namespace catapult::utils