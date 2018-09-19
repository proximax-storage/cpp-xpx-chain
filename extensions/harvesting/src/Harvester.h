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

#include "catapult/cache/CatapultCache.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/model/Elements.h"
#include "catapult/model/EntityInfo.h"
#include "TransactionsInfo.h"
#include "UnlockedAccounts.h"
#include <src/catapult/extensions/LocalNodeStateRef.h>

namespace catapult { namespace harvesting {

	/// A class that creates new blocks.
	class Harvester {
	public:
		/// Creates a harvester around a catapult \a localNodeState, an unlocked accounts set (\a unlockedAccounts)
		/// and a transactions info supplier (\a transactionsInfoSupplier).
		explicit Harvester(
				const extensions::LocalNodeStateRef& localNodeState,
				const UnlockedAccounts& unlockedAccounts,
				const TransactionsInfoSupplier& transactionsInfoSupplier);

	public:
		/// Creates the best block (if any) harvested by any unlocked account.
		/// Created block will have \a lastBlockElement as parent and \a timestamp as timestamp.
		std::unique_ptr<model::Block> harvest(const model::BlockElement& lastBlockElement, Timestamp timestamp);

	private:
		const extensions::LocalNodeStateRef& m_localNodeState;
		const UnlockedAccounts& m_unlockedAccounts;
		TransactionsInfoSupplier m_transactionsInfoSupplier;
	};
}}
