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
#include "RemoteNodeSynchronizer.h"
#include "catapult/handlers/HandlerTypes.h"
#include "catapult/model/RangeTypes.h"

namespace catapult { namespace api { class RemoteTransactionApi; } }

namespace catapult { namespace chain {

	/// Function signature for supplying a range of short hashes.
	using ShortHashesSupplier = supplier<model::ShortHashRange>;

	/// Creates an unconfirmed transactions synchronizer around the specified short hashes supplier (\a shortHashesSupplier)
	/// and transaction range consumer (\a transactionRangeConsumer) for transactions with fee multipliers at least \a minFeeMultiplier.
	RemoteNodeSynchronizer<api::RemoteTransactionApi> CreateUtSynchronizer(
			BlockFeeMultiplier minFeeMultiplier,
			const ShortHashesSupplier& shortHashesSupplier,
			const handlers::TransactionRangeHandler& transactionRangeConsumer);
}}
