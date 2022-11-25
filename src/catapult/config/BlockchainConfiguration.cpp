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

#include "BlockchainConfiguration.h"
#include "ConfigurationFileLoader.h"
#include "catapult/crypto/KeyPair.h"

namespace catapult { namespace config {

	// region BlockchainConfiguration

	namespace {
		std::string Qualify(const std::string& name) {
			std::ostringstream out;
			out << "config-" << name << ".properties";
			return out.str();
		}

		std::string HostQualify(const std::string& name, const std::string& host) {
			std::ostringstream out;
			out << "config-" << name << "-" << host << ".properties";
			return out.str();
		}
	}

	BlockchainConfiguration::BlockchainConfiguration(
			ImmutableConfiguration immutableConfig,
			model::NetworkConfiguration networkConfig,
			NodeConfiguration nodeConfig,
			LoggingConfiguration loggingConfig,
			UserConfiguration userConfig,
			ExtensionsConfiguration extensionsConfig,
			InflationConfiguration inflationConfig,
			config::SupportedEntityVersions supportedEntityVersions)
			: Immutable(std::move(immutableConfig))
			, Network(std::move(networkConfig))
			, Node(std::move(nodeConfig))
			, Logging(std::move(loggingConfig))
			, User(std::move(userConfig))
			, Extensions(std::move(extensionsConfig))
			, Inflation(std::move(inflationConfig))
			, SupportedEntityVersions(std::move(supportedEntityVersions))
	{}

	BlockchainConfiguration BlockchainConfiguration::LoadFromPath(
			const boost::filesystem::path& resourcesPath,
			const std::string& extensionsHost) {
		return BlockchainConfiguration(
				LoadIniConfiguration<ImmutableConfiguration>(resourcesPath / Qualify("immutable")),
				LoadIniConfiguration<model::NetworkConfiguration>(resourcesPath / Qualify("network")),
				LoadIniConfiguration<NodeConfiguration>(resourcesPath / Qualify("node")),
				LoadIniConfiguration<LoggingConfiguration>(resourcesPath / HostQualify("logging", extensionsHost)),
				LoadIniConfiguration<UserConfiguration>(resourcesPath / Qualify("user")),
				LoadIniConfiguration<ExtensionsConfiguration>(resourcesPath / HostQualify("extensions", extensionsHost)),
				LoadIniConfiguration<InflationConfiguration>(resourcesPath / Qualify("inflation")),
				LoadSupportedEntityVersions(resourcesPath / "supported-entities.json"));
	}

	BlockchainConfiguration BlockchainConfiguration::Uninitialized() {
		return BlockchainConfiguration(
			ImmutableConfiguration::Uninitialized(),
			model::NetworkConfiguration::Uninitialized(),
			NodeConfiguration::Uninitialized(),
			LoggingConfiguration::Uninitialized(),
			UserConfiguration::Uninitialized(),
			ExtensionsConfiguration::Uninitialized(),
			InflationConfiguration::Uninitialized(),
			config::SupportedEntityVersions()
		);
	}

	// endregion

	ionet::Node ToLocalNode(const BlockchainConfiguration& config) {
		const auto& localNodeConfig = config.Node.Local;

		auto identityKey = crypto::KeyPair::FromString(config.User.BootKey).publicKey();

		auto endpoint = ionet::NodeEndpoint();
		endpoint.Host = localNodeConfig.Host;
		endpoint.Port = config.Node.Port;

		auto metadata = ionet::NodeMetadata(config.Immutable.NetworkIdentifier);
		metadata.Name = localNodeConfig.FriendlyName;
		metadata.Version = ionet::NodeVersion(localNodeConfig.Version);
		metadata.Roles = localNodeConfig.Roles;

		return ionet::Node(identityKey, endpoint, metadata);
	}

	ionet::Node ToLocalDbrbNode(const BlockchainConfiguration& config) {
		const auto& localNodeConfig = config.Node.Local;

		auto identityKey = crypto::KeyPair::FromString(config.User.BootKey).publicKey();

		auto endpoint = ionet::NodeEndpoint();
		endpoint.Host = localNodeConfig.Host;
		endpoint.Port = config.Node.DbrbPort;

		auto metadata = ionet::NodeMetadata(config.Immutable.NetworkIdentifier);
		metadata.Name = localNodeConfig.FriendlyName;
		metadata.Version = ionet::NodeVersion(localNodeConfig.Version);
		metadata.Roles = localNodeConfig.Roles;

		return ionet::Node(identityKey, endpoint, metadata);
	}

	BlockFeeMultiplier GetMinFeeMultiplier(const BlockchainConfiguration& config) {
		return config.Network.EnableUnconfirmedTransactionMinFeeValidation ? config.Node.MinFeeMultiplier : BlockFeeMultiplier{0};
	}
}}
