/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StreamFinishMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/streaming/src/model/StreamPaymentTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamStreamPaymentTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "driveKey" << ToBinary(transaction.DriveKey);
		builder << "streamId" << ToBinary(transaction.StreamId);
		builder << "additionalUploadSize" << static_cast<int64_t>(transaction.AdditionalUploadSize);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(StreamPayment, StreamStreamPaymentTransaction)
}}}
