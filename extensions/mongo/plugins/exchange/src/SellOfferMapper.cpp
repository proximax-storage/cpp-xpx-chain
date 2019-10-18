/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SellOfferMapper.h"
#include "ExchangeMapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/exchange/src/model/SellOfferTransaction.h"

namespace catapult { namespace mongo { namespace plugins {

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(SellOffer, StreamOfferTransaction)
}}}
