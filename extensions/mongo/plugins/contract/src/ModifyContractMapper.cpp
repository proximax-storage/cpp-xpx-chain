/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ModifyContractMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/contract/src/model/ModifyContractTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		void StreamContractModification(bson_stream::array_context& context, model::CosignatoryModificationType type, const Key& key) {
			context
					<< bson_stream::open_document
					<< "type" << utils::to_underlying_type(type)
					<< "cosignatoryPublicKey" << ToBinary(key)
					<< bson_stream::close_document;
		}

		void StreamModifications(
				bson_stream::document& builder,
				const model::CosignatoryModification* pModification,
				size_t numModifications,
				const std::string& arrayName) {
			auto modificationsArray = builder << arrayName << bson_stream::open_array;
			for (auto i = 0u; i < numModifications; ++i, ++pModification)
				StreamContractModification(modificationsArray, pModification->ModificationType, pModification->CosignatoryPublicKey);

			modificationsArray << bson_stream::close_array;
		}
	}

	template<typename TTransaction>
	void StreamContractTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder
				<< "durationDelta" << transaction.DurationDelta
				<< "hash" << ToBinary(transaction.Hash);
		StreamModifications(builder, transaction.CustomerModificationsPtr(), transaction.CustomerModificationCount, "customers");
		StreamModifications(builder, transaction.ExecutorModificationsPtr(), transaction.ExecutorModificationCount, "executors");
		StreamModifications(builder, transaction.VerifierModificationsPtr(), transaction.VerifierModificationCount, "verifiers");
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(ModifyContract, StreamContractTransaction)
}}}
