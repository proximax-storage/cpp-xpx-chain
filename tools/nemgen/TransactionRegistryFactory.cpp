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
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/plugins/MosaicAliasTransactionPlugin.h"
#include "catapult/plugins/MosaicDefinitionTransactionPlugin.h"
#include "catapult/plugins/MosaicSupplyChangeTransactionPlugin.h"
#include "catapult/plugins/RegisterNamespaceTransactionPlugin.h"
#include "catapult/plugins/TransferTransactionPlugin.h"
#include "mosaic/src/config/MosaicConfiguration.h"
#include "namespace/src/config/NamespaceConfiguration.h"

namespace catapult { namespace tools { namespace nemgen {

	namespace {
		struct SupportedVersionSupplier {
			SupportedVersionSupplier(VersionSet supportedVersions) : m_supportedVersions(supportedVersions)
			{}

			const VersionSet& operator()() const {
				return m_supportedVersions;
			}

		private:
			VersionSet m_supportedVersions;
		};
	}

	model::TransactionRegistry CreateTransactionRegistry() {
		auto mosaicConfig = config::MosaicConfiguration::Uninitialized();
		auto namespaceConfig = config::NamespaceConfiguration::Uninitialized();
		auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
		blockChainConfig.SetPluginConfiguration("catapult.plugins.mosaic", mosaicConfig);
		blockChainConfig.SetPluginConfiguration("catapult.plugins.namespace", namespaceConfig);
		model::TransactionRegistry registry;
		registry.registerPlugin(plugins::CreateMosaicAliasTransactionPlugin(SupportedVersionSupplier({ 1 })));
		registry.registerPlugin(plugins::CreateMosaicDefinitionTransactionPlugin(blockChainConfig, SupportedVersionSupplier({ 3 })));
		registry.registerPlugin(plugins::CreateMosaicSupplyChangeTransactionPlugin(SupportedVersionSupplier({ 2 })));
		registry.registerPlugin(plugins::CreateRegisterNamespaceTransactionPlugin(blockChainConfig, SupportedVersionSupplier({ 2 })));
		registry.registerPlugin(plugins::CreateTransferTransactionPlugin(SupportedVersionSupplier({ 3 })));
		return registry;
	}
}}}
