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
#include "catapult/model/ChainScore.h"
#include "catapult/observers/ObserverTypes.h"
#include <functional>

namespace catapult {
	namespace extensions { struct LocalNodeStateRef; }
	namespace model {
		struct Block;
		struct BlockChainConfiguration;
	}
	namespace plugins { class PluginManager; }
}

namespace catapult { namespace filechain {

	/// A notification observer factory.
	using NotificationObserverFactory = supplier<std::unique_ptr<const observers::NotificationObserver>>;

	/// A block dependent notification observer factory.
	using BlockDependentNotificationObserverFactory =
		std::function<std::unique_ptr<const observers::NotificationObserver> (const model::Block&)>;

	/// Creates a block dependent notification observer factory that calculates an inflection point from \a lastBlock and \a config.
	/// Prior to the inflection point, an observer created by \a permanentObserverFactory is returned.
	/// At and after the inflection point, an observer created by \a transientObserverFactory is returned.
	BlockDependentNotificationObserverFactory CreateBlockDependentNotificationObserverFactory(
			const model::Block& lastBlock,
			const model::BlockChainConfiguration& config,
			const NotificationObserverFactory& transientObserverFactory,
			const NotificationObserverFactory& permanentObserverFactory);

	/// Loads a block chain from storage using the supplied observer factory (\a observerFactory) and plugin manager (\a pluginManager)
	/// and updating \a stateRef starting with the block at \a startHeight.
	model::ChainScore LoadBlockChain(
			const BlockDependentNotificationObserverFactory& observerFactory,
			const plugins::PluginManager& pluginManager,
			const extensions::LocalNodeStateRef& stateRef,
			Height startHeight);
}}
