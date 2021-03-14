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
#include "catapult/model/NetworkInfo.h"
#include "catapult/types.h"
#include <cstdint>
#include <limits>

namespace catapult { namespace validators {

	/// Contextual information passed to stateless validators.
	struct StatelessValidatorContext {
	public:
		/// Creates a validator context around a \a config.
		StatelessValidatorContext(const config::BlockchainConfiguration& config)
				: Config(config)
				, NetworkIdentifier(config.Immutable.NetworkIdentifier)
				, Network(config.Network.Info)
		{}

	public:
		/// Blockchain config.
		const config::BlockchainConfiguration& Config;

		/// Network identifier.
		const model::NetworkIdentifier& NetworkIdentifier;

		/// Network info.
		const model::NetworkInfo& Network;
	};
}}
