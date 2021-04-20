/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/storage/src/model/DownloadTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamDownloadTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "consumer" << ToBinary(transaction.Signer);
		builder << "drive" << ToBinary(transaction.DriveKey);
		builder << "transactionFee" << ToInt64(transaction.TransactionFee);
		builder << "downloadSize" << ToInt64(transaction.DownloadSize);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(Download, StreamDownloadTransaction)
}}}
