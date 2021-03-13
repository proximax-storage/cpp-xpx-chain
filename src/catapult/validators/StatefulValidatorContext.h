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
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/model/NetworkInfo.h"
#include "catapult/model/ResolverContext.h"
#include "catapult/types.h"
#include <cstdint>
#include <limits>

namespace catapult { namespace validators {

	/// Contextual information passed to stateful validators.
	struct StatefulValidatorContext {
	public:
		/// Creates a validator context around a \a config, \a height, \a blockTime, \a resolvers and \a cache.
		StatefulValidatorContext(
				const config::BlockchainConfiguration& config,
				catapult::Height height,
				Timestamp blockTime,
				const model::ResolverContext& resolvers,
				const cache::ReadOnlyCatapultCache& cache)
				: Config(config)
                , Height(height)
                , BlockTime(blockTime)
				, NetworkIdentifier(config.Immutable.NetworkIdentifier)
				, Network(config.Network.Info)
				, Resolvers(resolvers)
				, Cache(cache)
		{}

	public:
		/// Blockchain config.
		const config::BlockchainConfiguration& Config;

		/// Current height.
		const catapult::Height Height;

		/// Current block time.
		const Timestamp BlockTime;

		/// Network identifier.
		const model::NetworkIdentifier NetworkIdentifier;

		/// Network info.
		const model::NetworkInfo Network;

		/// Alias resolvers.
		const model::ResolverContext Resolvers;

		/// Catapult cache.
		const cache::ReadOnlyCatapultCache& Cache;
	};
}}
