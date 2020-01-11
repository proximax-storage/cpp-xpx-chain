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

#include <memory>

namespace catapult {
	namespace cache { class CatapultCache; }
	namespace config { class BlockchainConfigurationHolder; }
	namespace extensions { class LocalNodeChainScore; }
	namespace io { class BlockStorageCache; }
	namespace state { struct CatapultState; }
}

namespace catapult { namespace extensions {

	/// A reference to a local node's basic state.
	struct LocalNodeStateRef {
	public:
		/// Creates a local node state ref referencing state composed of
		/// \a configHolder, \a state, \a cache, \a storage and \a score.
		LocalNodeStateRef(
				const std::shared_ptr<config::BlockchainConfigurationHolder>& configHolder,
				state::CatapultState& state,
				cache::CatapultCache& cache,
				io::BlockStorageCache& storage,
				LocalNodeChainScore& score)
				: ConfigHolder(configHolder)
				, State(state)
				, Cache(cache)
				, Storage(storage)
				, Score(score)
		{}

	public:
		/// Blockchain configuration.
		std::shared_ptr<config::BlockchainConfigurationHolder> ConfigHolder;

		/// Local node state.
		state::CatapultState& State;

		/// Local node cache.
		cache::CatapultCache& Cache;

		/// Local node storage.
		io::BlockStorageCache& Storage;

		/// Local node score.
		LocalNodeChainScore& Score;
	};
}}
