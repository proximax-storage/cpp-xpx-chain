/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config/BlockchainConfiguration.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/model/NetworkInfo.h"
#include "catapult/model/ResolverContext.h"
#include "catapult/types.h"
#include <cstdint>
#include <limits>

namespace catapult { namespace notification_handlers {

	/// Contextual information passed to stateful handlers.
	struct HandlerContext {
	public:
		/// Creates a handler context around a \a config, \a height, \a blockTime, \a resolvers and \a cache.
		HandlerContext(
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
