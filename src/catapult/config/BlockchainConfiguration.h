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

#pragma once
#include "ExtensionsConfiguration.h"
#include "ImmutableConfiguration.h"
#include "InflationConfiguration.h"
#include "LoggingConfiguration.h"
#include "NodeConfiguration.h"
#include "PeersConfiguration.h"
#include "SupportedEntityVersions.h"
#include "UserConfiguration.h"
#include "catapult/model/NetworkConfiguration.h"
#include <boost/filesystem/path.hpp>

namespace catapult { namespace config {

	/// Comprehensive configuration for a catapult process.
	class BlockchainConfiguration {
	public:
		/// Creates a blockchain configuration around \a immutableConfig, \a networkConfig, \a nodeConfig, \a loggingConfig, \a userConfig,
		/// \a extensionsConfig, \a inflationConfig and \a supportedEntityVersions.
		BlockchainConfiguration(
				ImmutableConfiguration immutableConfig,
				model::NetworkConfiguration networkConfig,
				NodeConfiguration nodeConfig,
				LoggingConfiguration loggingConfig,
				UserConfiguration userConfig,
				ExtensionsConfiguration extensionsConfig,
				SupportedEntityVersions supportedEntityVersions = config::SupportedEntityVersions(),
				Height activationHeight = Height(),
				const BlockchainConfiguration* previousConfig = nullptr );

		DECLARE_CATAPULT_DESTRUCTOR_DEFINITION_CLEANUP_LOG(BlockchainConfiguration)
		DEFINE_CATAPULT_CLEANUP_LOG_FOUR_DEFAULTS(BlockchainConfiguration)
	public:
		/// Immutable network configuration.
		const ImmutableConfiguration Immutable;

		/// Network configuration.
		const model::NetworkConfiguration Network;

		/// Node configuration.
		const NodeConfiguration Node;

		/// Logging configuration.
		const LoggingConfiguration Logging;

		/// User configuration.
		const UserConfiguration User;

		/// Extensions configuration.
		const ExtensionsConfiguration Extensions;

		/// Supported entity versions.
		const config::SupportedEntityVersions SupportedEntityVersions;

		/// Activation Height
		const Height ActivationHeight;

		/// Previous active configuration
		const BlockchainConfiguration* PreviousConfiguration;

	public:
		/// Creates an uninitialized blockchain configuration.
		static BlockchainConfiguration Uninitialized();

		/// Loads a blockchain configuration from \a resourcesPath given the specified extensions host (\a extensionsHost).
		/// \note This function is expected to be called be before logging is enabled.
		static BlockchainConfiguration LoadFromPath(const boost::filesystem::path& resourcesPath, const std::string& extensionsHost);

		/// Loads a blockchain configuration from \a resourcesPath given the specified extensions host (\a extensionsHost).
		/// \note This function is expected to be called be before logging is enabled. This function does not load network configuration or supported entities
		static BlockchainConfiguration LoadLocalFromPath(const boost::filesystem::path& resourcesPath, const std::string& extensionsHost);
	};

	/// Extracts a node representing the local node from \a config.
	ionet::Node ToLocalNode(const BlockchainConfiguration& config);

	/// Gets min transaction fee multiplier.
	BlockFeeMultiplier GetMinFeeMultiplier(const BlockchainConfiguration& config);
}}
