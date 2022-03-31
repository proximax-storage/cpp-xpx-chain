/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ManualRateChangeMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/liquidity_provider//src/model/ManualRateChangeTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamManualRateChangeTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "providerMosaicId" << static_cast<int64_t>(transaction.ProviderMosaicId.unwrap());
		builder << "currencyBalanceIncrease" << transaction.CurrencyBalanceIncrease;
		builder << "currencyBalanceChange" << static_cast<int64_t>(transaction.CurrencyBalanceChange.unwrap());
		builder << "mosaicBalanceIncrease" << transaction.MosaicBalanceIncrease;
		builder << "mosaicBalanceChange" << static_cast<int64_t>(transaction.MosaicBalanceChange.unwrap());
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(ManualRateChange, StreamManualRateChangeTransaction)
}}}
