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

#include "plugins/txes/namespace/src/config/NamespaceConfiguration.h"
#include "plugins/txes/mosaic/src/config/MosaicConfiguration.h"
#include "plugins/txes/transfer/src/config/TransferConfiguration.h"
#include "plugins/txes/upgrade/src/config/BlockchainUpgradeConfiguration.h"
#include "plugins/txes/config/src/config/NetworkConfigConfiguration.h"
#include "plugins/txes/committee/src/config/CommitteeConfiguration.h"
#include "NemesisCompatibleConfiguration.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/nodeps/MijinConstants.h"

namespace catapult { namespace test {

	namespace {
		model::NetworkConfiguration CreateNetworkConfiguration() {
			auto config = CreatePrototypicalNetworkConfiguration();
			AddNemesisPluginExtensions(config);
			return config;
		}
	}

	void AddNemesisPluginExtensions(model::NetworkConfiguration& config) {
		config.Plugins.emplace(PLUGIN_NAME(transfer), utils::ConfigurationBag({{ "", { { "maxMessageSize", "0" }, { "maxMosaicsSize", "512" } } }}));
		config.Plugins.emplace(PLUGIN_NAME(mosaic), utils::ConfigurationBag({ { "", {
			{ "maxMosaicsPerAccount", "123" },
			{ "maxMosaicDuration", "456d" },
			{ "maxMosaicDivisibility", "6" },

			{ "mosaicRentalFeeSinkPublicKey", Mosaic_Rental_Fee_Sink_Public_Key },
			{ "mosaicRentalFee", "500" }
		} } }));
		config.Plugins.emplace(PLUGIN_NAME(namespace), utils::ConfigurationBag({ { "", {
			{ "maxNameSize", "64" },

			{ "maxNamespaceDuration", "365d" },
			{ "namespaceGracePeriodDuration", "1h" },
			{ "reservedRootNamespaceNames", "cat" },

			{ "namespaceRentalFeeSinkPublicKey", Namespace_Rental_Fee_Sink_Public_Key },
			{ "rootNamespaceRentalFeePerBlock", "10" },
			{ "childNamespaceRentalFee", "10000" },

			{ "maxChildNamespaces", "100" }
		} } }));
		config.Plugins.emplace(PLUGIN_NAME(config), utils::ConfigurationBag({ { "", {
			{ "maxBlockChainConfigSize", "1MB" },
			{ "maxSupportedEntityVersionsSize", "1MB" },
		} } }));
		config.Plugins.emplace(PLUGIN_NAME(upgrade), utils::ConfigurationBag({ { "", {
			{ "minUpgradePeriod", "360" },
		} } }));
		config.Plugins.emplace(PLUGIN_NAME(committee), utils::ConfigurationBag({ { "", {
			{ "enabled", "true" },

			{ "committeeNumber", "21" },
			{ "minGreed", "0.1" },
			{ "initialActivity", "0.367976785" },
			{ "activityDelta", "0.00001" },
			{ "activityCommitteeCosignedDelta", "0.01" },
			{ "activityCommitteeNotCosignedDelta", "0.02" },
		} } }));

		config.template InitPluginConfiguration<config::TransferConfiguration>();
		config.template InitPluginConfiguration<config::MosaicConfiguration>();
		config.template InitPluginConfiguration<config::NamespaceConfiguration>();
		config.template InitPluginConfiguration<config::BlockchainUpgradeConfiguration>();
		config.template InitPluginConfiguration<config::NetworkConfigConfiguration>();
		config.template InitPluginConfiguration<config::CommitteeConfiguration>();
	}

	namespace {
		void AddPluginExtensions(config::ExtensionsConfiguration& config, const std::unordered_set<std::string>& extensionNames) {
			for (const auto& extensionName : extensionNames)
				config.Names.emplace_back("extension." + extensionName);
		}

		void AddCommonPluginExtensions(config::ExtensionsConfiguration& config) {
			AddPluginExtensions(config, { "diagnostics", "networkheight", "packetserver", "sync", "transactionsink" });
		}
	}

	void AddApiPluginExtensions(config::ExtensionsConfiguration& config) {
		AddCommonPluginExtensions(config);
	}

	void AddPeerPluginExtensions(config::ExtensionsConfiguration& config) {
		AddCommonPluginExtensions(config);
		AddPluginExtensions(config, { "eventsource", "harvesting", "syncsource" });
	}

	void AddSimplePartnerPluginExtensions(config::ExtensionsConfiguration& config) {
		AddPluginExtensions(config, { "diagnostics", "packetserver", "sync", "syncsource" });
	}

	void AddRecoveryPluginExtensions(config::ExtensionsConfiguration& config) {
		AddPluginExtensions(config, {});
	}

	config::BlockchainConfiguration CreateBlockchainConfigurationWithNemesisPluginExtensions(const std::string& dataDirectory) {
		return CreatePrototypicalBlockchainConfiguration(CreateNetworkConfiguration(), dataDirectory);
	}
}}
