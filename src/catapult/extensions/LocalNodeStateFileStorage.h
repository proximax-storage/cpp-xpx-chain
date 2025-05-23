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
#include "catapult/config/CatapultDataDirectory.h"
#include "catapult/types.h"
#include "catapult/model/NetworkConfiguration.h"

namespace catapult {
	namespace cache {
		class CacheStorage;
		class CatapultCache;
		class CatapultCacheDelta;
		struct SupplementalData;
	}
	namespace config { struct NodeConfiguration; }
	namespace extensions { struct LocalNodeStateRef; }
	namespace model { class ChainScore; }
	namespace plugins { class PluginManager; }
	namespace state { struct CatapultState; }
}

namespace catapult { namespace extensions {

	/// Information about state heights.
	struct StateHeights {
		/// Cache height.
		Height Cache;

		/// Storage height.
		Height Storage;
	};

	/// Returns \c true if serialized state is present in \a directory.
	bool HasSerializedState(const config::CatapultDirectory& directory);
	/// Returns \c true if active network config file is present in \a directory.
	bool HasActiveNetworkConfig(const config::CatapultDirectory& directory);

	/// Loads catapult state into \a stateRef from \a directory given \a pluginManager.
	StateHeights LoadStateFromDirectory(
			const config::CatapultDirectory& directory,
			const LocalNodeStateRef& stateRef,
			const plugins::PluginManager& pluginManager);

	/// Retrieves last known height from supplemental data file.
	const Height LoadLatestHeightFromDirectory(const config::CatapultDirectory& directory);
	/// Serializes local node state.
	class LocalNodeStateSerializer {
	public:
		/// Creates a serializer around specified \a directory.
		explicit LocalNodeStateSerializer(const config::CatapultDirectory& directory);

	public:
		/// Saves state composed of \a cache, \a state and \a score.
		void save(const cache::CatapultCache& cache, const state::CatapultState& state, const model::ChainScore& score) const;

		/// Saves state composed of \a cacheDelta, \a state, \a score and \a height using \a cacheStorages.
		void save(
				const cache::CatapultCache& cache,
				const cache::CatapultCacheDelta& cacheDelta,
				const std::vector<std::unique_ptr<const cache::CacheStorage>>& cacheStorages,
				const state::CatapultState& state,
				const model::ChainScore& score,
				Height height) const;

		/// Moves serialized state to \a destinationDirectory.
		void moveTo(const config::CatapultDirectory& destinationDirectory);

	private:
		config::CatapultDirectory m_directory;
	};
	/// Loads the last active network configuration from a file.
	const std::string LoadActiveNetworkConfigString(const config::CatapultDirectory& directory);

	/// Loads the last active network configuration from a file
	const model::NetworkConfiguration LoadActiveNetworkConfig(const config::CatapultDirectory& directory);

	/// Serializes state composed of \a cache, \a state and \a score with checkpointing to \a dataDirectory given \a nodeConfig.
	void SaveStateToDirectoryWithCheckpointing(
			const config::CatapultDataDirectory& dataDirectory,
			const config::NodeConfiguration& nodeConfig,
			const cache::CatapultCache& cache,
			const state::CatapultState& state,
			const model::ChainScore& score);
}}
