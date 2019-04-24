/**
*** Copyright (c) 2018-present,
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
