/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AutomaticExecutionsPaymentMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "src/model/AutomaticExecutionsPaymentTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    template<typename TTransaction>
    void StreamAutomaticExecutionsPaymentTransaction(bson_stream::document& builder, const TTransaction& transaction) {
        builder << "contractKey" << ToBinary(transaction.ContractKey)
                << "automaticExecutionsNumber" << ToInt32(transaction.AutomaticExecutionsNumber)
                << "signer" << ToBinary(transaction.Signer);
    }

    DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(AutomaticExecutionsPayment, StreamAutomaticExecutionsPaymentTransaction)
}}}
