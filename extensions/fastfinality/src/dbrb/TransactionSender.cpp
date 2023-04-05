/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionSender.h"
#include "sdk/src/builders/InstallMessageBuilder.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "catapult/model/EntityHasher.h"
#include <boost/dynamic_bitset.hpp>

namespace catapult { namespace dbrb {

//    Hash256 TransactionSender::sendInstallMessageTransaction(InstallMessage& message) {
//        CATAPULT_LOG(debug) << "sending install message transaction";
//
//		// build and send transaction
//        builders::InstallMessageBuilder builder(m_networkIdentifier, m_keyPair.publicKey(), message);
//        auto pTransaction = utils::UniqueToShared(builder.build());
//        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_dbrbConfig.TransactionTimeout.millis());
//        send(pTransaction);
//
//        return model::CalculateHash(*pTransaction, m_generationHash);
//    }
//
//    void TransactionSender::send(std::shared_ptr<model::Transaction> pTransaction) {
//		extensions::TransactionExtensions(m_generationHash).sign(m_keyPair, *pTransaction);
//        auto range = model::TransactionRange::FromEntity(pTransaction);
//        m_transactionRangeHandler({std::move(range), m_keyPair.publicKey()});
//    }
}}
