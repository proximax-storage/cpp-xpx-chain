/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "TransactionRegistryFactory.h"
#include "catapult/plugins/NetworkConfigTransactionPlugin.h"
#include "catapult/plugins/BlockchainUpgradeTransactionPlugin.h"
#include "catapult/plugins/MosaicAliasTransactionPlugin.h"
#include "catapult/plugins/MosaicDefinitionTransactionPlugin.h"
#include "catapult/plugins/MosaicSupplyChangeTransactionPlugin.h"
#include "catapult/plugins/RegisterNamespaceTransactionPlugin.h"
#include "catapult/plugins/TransferTransactionPlugin.h"
#include "mosaic/src/config/MosaicConfiguration.h"
#include "namespace/src/config/NamespaceConfiguration.h"

namespace catapult { namespace tools { namespace nemgen {

	model::TransactionRegistry CreateTransactionRegistry() {
		auto mosaicConfig = config::MosaicConfiguration::Uninitialized();
		auto namespaceConfig = config::NamespaceConfiguration::Uninitialized();
		auto networkConfig = model::NetworkConfiguration::Uninitialized();
		networkConfig.SetPluginConfiguration(mosaicConfig);

		networkConfig.SetPluginConfiguration(namespaceConfig);
		config::BlockchainConfiguration config{
			config::ImmutableConfiguration::Uninitialized(),
			std::move(networkConfig),
			config::NodeConfiguration::Uninitialized(),
			config::LoggingConfiguration::Uninitialized(),
			config::UserConfiguration::Uninitialized(),
			config::ExtensionsConfiguration::Uninitialized(),
			config::InflationConfiguration::Uninitialized(),
			config::SupportedEntityVersions()
		};

		auto pConfigHolder = std::make_shared<config::BlockchainConfigurationHolder>(config);
		model::TransactionRegistry registry;
		registry.registerPlugin(plugins::CreateMosaicAliasTransactionPlugin());
		registry.registerPlugin(plugins::CreateMosaicDefinitionTransactionPlugin(pConfigHolder));
		registry.registerPlugin(plugins::CreateMosaicSupplyChangeTransactionPlugin());
		registry.registerPlugin(plugins::CreateRegisterNamespaceTransactionPlugin(pConfigHolder));
		registry.registerPlugin(plugins::CreateTransferTransactionPlugin());
		registry.registerPlugin(plugins::CreateNetworkConfigTransactionPlugin());
		registry.registerPlugin(plugins::CreateBlockchainUpgradeTransactionPlugin());
		return registry;
	}
}}}
