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
#include "PtTypes.h"
#include "catapult/model/RangeTypes.h"
#include "catapult/model/WeakCosignedTransactionInfo.h"

namespace catapult { namespace partialtransaction {

	/// Stitches a weak cosigned transaction \a info into a full aggregate transaction.
	model::UniqueEntityPtr<model::Transaction> StitchAggregate(const model::WeakCosignedTransactionInfo& transactionInfo);

	/// Splits up cosigned transaction infos (\a transactionInfos) and forwards transactions as a single range
	/// to \a transactionRangeConsumer and cosignatures individually to \a cosignatureConsumer.
	void SplitCosignedTransactionInfos(
			CosignedTransactionInfos&& transactionInfos,
			const consumer<model::TransactionRange&&>& transactionRangeConsumer,
			const consumer<model::DetachedCosignature&&>& cosignatureConsumer);
}}
