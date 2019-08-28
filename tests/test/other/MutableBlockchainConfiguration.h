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
#include "catapult/config/BlockchainConfiguration.h"

namespace catapult { namespace test {

	/// Comprehensive mutable configuration for a catapult process.
	class MutableBlockchainConfiguration {
	public:
		/// Creates a mutable blockchain configuration.
		MutableBlockchainConfiguration()
				: Network(model::NetworkConfiguration::Uninitialized())
				, Node(config::NodeConfiguration::Uninitialized())
				, Logging(config::LoggingConfiguration::Uninitialized())
				, User(config::UserConfiguration::Uninitialized())
				, Extensions(config::ExtensionsConfiguration::Uninitialized())
				, Inflation(config::InflationConfiguration::Uninitialized())
				, SupportedEntityVersions()
		{}

	public:
		/// Network configuration.
		model::NetworkConfiguration Network;

		/// Node configuration.
		config::NodeConfiguration Node;

		/// Logging configuration.
		config::LoggingConfiguration Logging;

		/// User configuration.
		config::UserConfiguration User;

		/// Extensions configuration.
		config::ExtensionsConfiguration Extensions;

		/// Inflation configuration.
		config::InflationConfiguration Inflation;

		/// Supported entity versions.
		config::SupportedEntityVersions SupportedEntityVersions;

	public:
		/// Converts this mutable configuration to a const configuration.
		config::BlockchainConfiguration ToConst() {
			return config::BlockchainConfiguration(
					std::move(Network),
					std::move(Node),
					std::move(Logging),
					std::move(User),
					std::move(Extensions),
					std::move(Inflation),
					std::move(SupportedEntityVersions));
		}
	};
}}
