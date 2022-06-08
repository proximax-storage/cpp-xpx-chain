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

#include "PtUtils.h"
#include "plugins/txes/aggregate/src/model/AggregateTransaction.h"
#include "catapult/utils/MemoryUtils.h"

namespace catapult { namespace partialtransaction {

	namespace {
		template<typename TCosignatureType>
		void AssignCosignature(model::CosignatureInfo cosignature, TCosignatureType* targetCosignature);

		template<typename TCosignatureType>
		void AssignCosignature(model::CosignatureInfo cosignature, model::CosignatureInfo* targetCosignature)
		{
			*targetCosignature = cosignature;
		}

		template<typename TCosignatureType>
		void AssignCosignature(model::CosignatureInfo cosignature, model::Cosignature<SignatureLayout::Raw>* targetCosignature)
		{
			targetCosignature->Signer = cosignature.Signer;
			targetCosignature->Signature = cosignature.GetRawSignature();
		}

		template<typename TAggregateDescriptor>
		model::UniqueEntityPtr<model::Transaction> StitchAggregateImpl(const model::WeakCosignedTransactionInfo& transactionInfo) {
			uint32_t size = transactionInfo.transaction().Size
							+ sizeof(typename TAggregateDescriptor::CosignatureType) * static_cast<uint32_t>(transactionInfo.cosignatures().size());
			auto pTransactionWithCosignatures = utils::MakeUniqueWithSize<model::AggregateTransaction<TAggregateDescriptor>>(size);

			// copy transaction data
			auto transactionSize = transactionInfo.transaction().Size;
			std::memcpy(static_cast<void*>(pTransactionWithCosignatures.get()), &transactionInfo.transaction(), transactionSize);
			pTransactionWithCosignatures->Size = size;

			// copy cosignatures
			auto* pCosignature = pTransactionWithCosignatures->CosignaturesPtr();
			for (const auto& cosignature : transactionInfo.cosignatures())
				AssignCosignature<TAggregateDescriptor>(cosignature, pCosignature++);

			return std::move(pTransactionWithCosignatures);
		}
	}


	bool IsAggregateV1(model::EntityType type) {
		return model::Entity_Type_Aggregate_Complete_V1 == type || model::Entity_Type_Aggregate_Bonded_V1 == type;
	}

	bool IsAggregateV2(model::EntityType type) {
		return model::Entity_Type_Aggregate_Complete_V2 == type || model::Entity_Type_Aggregate_Bonded_V2 == type;
	}

	model::UniqueEntityPtr<model::Transaction> StitchAggregate(const model::WeakCosignedTransactionInfo& transactionInfo) {
		if(IsAggregateV2(transactionInfo.transaction().Type))
			return std::move(StitchAggregateImpl<model::AggregateTransactionExtendedDescriptor>(transactionInfo));
		return std::move(StitchAggregateImpl<model::AggregateTransactionRawDescriptor>(transactionInfo));
	}

	namespace {
		model::TransactionRange CopyIntoRange(const model::WeakCosignedTransactionInfo& transactionInfo) {
			auto pStitchedTransaction = StitchAggregate(transactionInfo);
			return model::TransactionRange::FromEntity(std::move(pStitchedTransaction));
		}
	}

	
	void SplitCosignedTransactionInfos(
			CosignedTransactionInfos&& transactionInfos,
			const consumer<model::TransactionRange&&>& transactionRangeConsumer,
			const consumer<model::DetachedCosignature&&>& cosignatureConsumer) {
		std::vector<model::TransactionRange> transactionRanges;
		for (const auto& transactionInfo : transactionInfos) {
			if (transactionInfo.pTransaction) {
				transactionRanges.push_back(CopyIntoRange({ transactionInfo.pTransaction.get(), &transactionInfo.Cosignatures }));
				continue;
			}

			for (const auto& cosignature : transactionInfo.Cosignatures)
				cosignatureConsumer(model::DetachedCosignature(cosignature.Signer, cosignature.GetRawSignature(), cosignature.GetDerivationScheme(), transactionInfo.EntityHash));
		}

		if (!transactionRanges.empty())
			transactionRangeConsumer(model::TransactionRange::MergeRanges(std::move(transactionRanges)));
	}
}}
