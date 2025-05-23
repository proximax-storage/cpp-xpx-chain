/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CreateLiquidityProviderMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/liquidityprovider//src/model/CreateLiquidityProviderTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamCreateLiquidityProviderTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "providerMosaicId" << static_cast<int64_t>(transaction.ProviderMosaicId.unwrap());
		builder << "currencyDeposit" << static_cast<int64_t>(transaction.CurrencyDeposit.unwrap());
		builder << "initialMosaicsMinting" << static_cast<int64_t>(transaction.InitialMosaicsMinting.unwrap());
		builder << "slashingPeriod" << static_cast<int32_t>(transaction.SlashingPeriod);
		builder << "windowSize" << static_cast<int32_t>(transaction.WindowSize);
		builder << "slashingAccount" << ToBinary(transaction.SlashingAccount);
		builder << "alpha" << static_cast<int32_t>(transaction.Alpha);
		builder << "beta" << static_cast<int32_t>(transaction.Beta);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(CreateLiquidityProvider, StreamCreateLiquidityProviderTransaction)
}}}
