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
#include "catapult/functions.h"

namespace catapult {
	namespace cache { class CatapultCache; }
	namespace io {
		class BlockChangeSubscriber;
		class BlockStorageCache;
	}
	namespace model {
		struct BlockChainConfiguration;
		struct BlockElement;
	}
	namespace plugins { class PluginManager; }
	namespace subscribers { class StateChangeSubscriber; }
}

namespace catapult { namespace local {

	/// Raises nemesis block notifications.
	class NemesisBlockNotifier {
	public:
		/// Creates a notifier around \a config, \a cache, \a storage and \a pluginManager.
		NemesisBlockNotifier(
				const model::BlockChainConfiguration& config,
				const cache::CatapultCache& cache,
				const io::BlockStorageCache& storage,
				const plugins::PluginManager& pluginManager);

	public:
		/// Raises and forwards block change notifications to \a subscriber.
		void raise(io::BlockChangeSubscriber& subscriber);

		/// Raises and forwards state change notifications to \a subscriber.
		void raise(subscribers::StateChangeSubscriber& subscriber);

	private:
		void raise(const consumer<model::BlockElement>& action);

	private:
		const model::BlockChainConfiguration& m_config;
		const cache::CatapultCache& m_cache;
		const io::BlockStorageCache& m_storage;
		const plugins::PluginManager& m_pluginManager;
	};
}}
