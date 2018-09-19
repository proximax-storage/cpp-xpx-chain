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

#include "LocalNodeChainScore.h"
#include <memory>
#include <src/catapult/cache/CatapultCache.h>
#include <src/catapult/io/BlockStorageCache.h>
#include <src/catapult/state/CatapultState.h>

#pragma once

namespace catapult {
	namespace cache { class CatapultCache; }
	namespace config { class LocalNodeConfiguration; }
	namespace extensions { class LocalNodeChainScore; }
	namespace io { class BlockStorageCache; class BlockStorage; }
	namespace state { struct CatapultState; }
}

namespace catapult { namespace extensions {

	/// A local node's basic state.
	struct LocalNodeState {
	public:

		/// Creates a local node state state composed of
		/// \a config, \a state, \a cache, \a storage and \a score.
		LocalNodeState(
				const config::LocalNodeConfiguration& config,
				std::unique_ptr<io::BlockStorage>&& pStorage)
				: Config(config)
				, State()
				, CurrentCache({})
				, PreviousCache({})
				, Storage(std::move(pStorage))
				, Score()
		{}

		/// Creates a local node state state composed of
		/// \a config, \a state, \a cache, \a storage and \a score.
		LocalNodeState(
				const config::LocalNodeConfiguration& config,
				std::unique_ptr<io::BlockStorage>&& pStorage,
				cache::CatapultCache&& cache)
				: Config(config)
				, State()
				, CurrentCache(std::move(cache))
				, PreviousCache(std::move(cache))
				, Storage(std::move(pStorage))
				, Score()
		{}

	public:
		/// Local node configuration.
		const config::LocalNodeConfiguration& Config;

		/// Local node state.
		state::CatapultState State;

		/// It is cache of current state of local node.
		cache::CatapultCache CurrentCache;

		/// It is cache of previous state of local node. We use it to calculate balance of accounts some blocks below.
		cache::CatapultCache PreviousCache;

		/// Local node storage.
		io::BlockStorageCache Storage;

		/// Local node score.
		LocalNodeChainScore Score;
	};

	/// A reference to a local node's basic state.
	struct LocalNodeStateRef {
	public:
		/// Creates a local node state ref referencing state composed of
		/// \a config, \a state, \a cache, \a storage and \a score.
		LocalNodeStateRef(LocalNodeState& state)
				: Config(state.Config)
				, State(state.State)
				, CurrentCache(state.CurrentCache)
				, PreviousCache(state.PreviousCache)
				, Storage(state.Storage)
				, Score(state.Score)
		{}

	public:
		/// Local node configuration.
		const config::LocalNodeConfiguration& Config;

		/// Local node state.
		state::CatapultState& State;

		/// It is cache of current state of local node.
		cache::CatapultCache& CurrentCache;

		/// It is cache of previous state of local node. We use it to calculate balance of accounts some blocks below.
		cache::CatapultCache& PreviousCache;

		/// Local node storage.
		io::BlockStorageCache& Storage;

		/// Local node score.
		LocalNodeChainScore& Score;
	};

	/// A const reference to a local node's basic state.
	struct LocalNodeStateConstRef {
	public:
		/// Creates a local node state const ref referencing state composed of
		/// \a config, \a state, \a cache, \a storage and \a score.
		LocalNodeStateConstRef(const LocalNodeState& state)
				: Config(state.Config)
				, State(state.State)
				, CurrentCache(state.CurrentCache)
				, PreviousCache(state.PreviousCache)
				, Storage(state.Storage)
				, Score(state.Score)
		{}

	public:
		/// Local node configuration.
		const config::LocalNodeConfiguration& Config;

		/// Local node state.
		const state::CatapultState& State;

		/// It is cache of current state of local node.
		const cache::CatapultCache& CurrentCache;

		/// It is cache of previous state of local node. We use it to calculate balance of accounts some blocks below.
		const cache::CatapultCache& PreviousCache;

		/// Local node storage.
		const io::BlockStorageCache& Storage;

		/// Local node score.
		const LocalNodeChainScore& Score;
	};
}}
