/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

namespace catapult { namespace config { class BlockchainConfigurationHolder; } }

namespace catapult { namespace cache {

	bool SuperContractPluginEnabled(std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder, const Height& height);
}}
