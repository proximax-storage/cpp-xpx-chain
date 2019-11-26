/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/plugins.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include <memory>

namespace catapult { namespace model { class TransactionPlugin; } }

namespace catapult { namespace plugins {

	/// Creates a files deposit transaction plugin.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateFilesDepositTransactionPlugin(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
}}
