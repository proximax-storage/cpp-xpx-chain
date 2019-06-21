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
#include "catapult/model/ContainerTypes.h"
#include "catapult/model/EntityInfo.h"

namespace catapult { namespace cache {

	/// Partial transactions change subscriber.
	class PtChangeSubscriber {
	public:
		using TransactionInfos = model::TransactionInfosSet;

	public:
		virtual ~PtChangeSubscriber() = default;

	public:
		/// Indicates transaction infos (\a transactionInfos) were added to partial transactions.
		/// \note This is only aggregate part and will not have any cosignatures.
		virtual void notifyAddPartials(const TransactionInfos& transactionInfos) = 0;

		/// Indicates a cosignature (composed of \a signer and \a signature) was added to a partial transaction (\a parentTransactionInfo).
		virtual void notifyAddCosignature(
				const model::TransactionInfo& parentTransactionInfo,
				const Key& signer,
				const Signature& signature) = 0;

		/// Indicates transaction infos (\a transactionInfos) were removed from partial transactions.
		/// \note This is only aggregate part and will not have any cosignatures.
		virtual void notifyRemovePartials(const TransactionInfos& transactionInfos) = 0;

		/// Flushes all pending partial transactions changes.
		virtual void flush() = 0;
	};
}}
