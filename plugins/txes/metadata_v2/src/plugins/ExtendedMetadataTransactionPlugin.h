/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/plugins.h"
#include <memory>
#include <catapult/config_holder/BlockchainConfigurationHolder.h>

namespace catapult { namespace model { class TransactionPlugin; } }

namespace catapult { namespace plugins {

	/// Creates an account extended metadata transaction plugin.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateAccountExtendedMetadataTransactionPlugin(const std::shared_ptr<config::BlockchainConfigurationHolder>&);

	/// Creates a mosaic extended metadata transaction plugin.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateMosaicExtendedMetadataTransactionPlugin(const std::shared_ptr<config::BlockchainConfigurationHolder>&);

	/// Creates a namespace extended metadata transaction plugin.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateNamespaceExtendedMetadataTransactionPlugin(const std::shared_ptr<config::BlockchainConfigurationHolder>&);
}}
