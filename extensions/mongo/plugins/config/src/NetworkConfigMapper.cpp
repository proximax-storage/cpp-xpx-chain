/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NetworkConfigMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/config/src/model/NetworkConfigTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamNetworkConfigTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder
				<< "applyHeightDelta" << ToInt64(transaction.ApplyHeightDelta)
				<< "networkConfig" << std::string((const char*)transaction.BlockChainConfigPtr(), transaction.BlockChainConfigSize)
				<< "supportedEntityVersions" << std::string((const char*)transaction.SupportedEntityVersionsPtr(), transaction.SupportedEntityVersionsSize);
	}

	template<typename TTransaction>
	void StreamNetworkConfigAbsoluteHeightTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder
				<< "applyHeight" << ToInt64(transaction.ApplyHeight)
				<< "networkConfig" << std::string((const char*)transaction.BlockChainConfigPtr(), transaction.BlockChainConfigSize)
				<< "supportedEntityVersions" << std::string((const char*)transaction.SupportedEntityVersionsPtr(), transaction.SupportedEntityVersionsSize);
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(NetworkConfig, StreamNetworkConfigTransaction)
	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(NetworkConfigAbsoluteHeight, StreamNetworkConfigAbsoluteHeightTransaction)
}}}
