/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DeactivateMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/DeactivateTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamDeactivateTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "superContract" << ToBinary(transaction.SuperContract);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(Deactivate, StreamDeactivateTransaction)
}}}
