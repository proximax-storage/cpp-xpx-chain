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
#include "catapult/model/BlockUtils.h"
#include "catapult/model/TransactionSelectionStrategy.h"

namespace catapult {
	namespace cache { class MemoryUtCache; }
	namespace harvesting { class HarvestingUtFacadeFactory; }
}

namespace catapult { namespace harvesting {

	/// Information about transactions.
	struct TransactionsInfo {
		/// Optimal fee multiplier.
		BlockFeeMultiplier FeeMultiplier;

		/// Transactions (shared pointers) (ordered).
		model::Transactions Transactions;

		/// Transaction hashes (ordered).
		std::vector<Hash256> TransactionHashes;

		/// Aggregate transactions hash.
		Hash256 TransactionsHash;
	};

	/// Supplies a transactions info composed of a maximum number of transactions for a block with specified time.
	using TransactionsInfoSupplier = std::function<TransactionsInfo (Timestamp, uint32_t)>;

	/// Creates a default transactions info supplier around \a utFacadeFactory and \a utCache for specified transaction \a strategy.
	TransactionsInfoSupplier CreateTransactionsInfoSupplier(
			model::TransactionSelectionStrategy strategy,
			const HarvestingUtFacadeFactory& utFacadeFactory,
			const cache::MemoryUtCache& utCache);
}}
