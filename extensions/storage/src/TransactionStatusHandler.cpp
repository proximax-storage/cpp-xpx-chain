/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionStatusHandler.h"
#include "catapult/exceptions.h"

namespace catapult { namespace storage {

    void TransactionStatusHandler::addHandler(catapult::Signature& transactionSignature, Callback handler) {
        std::lock_guard<std::mutex> lock(m_mutex);

        CATAPULT_LOG(debug) << "New handler for " << transactionSignature.data();
        m_callbacks[std::move(transactionSignature)] = std::move(handler);
    }

    void TransactionStatusHandler::handle(const catapult::Signature& transactionSignature, const Hash256& hash, uint32_t status) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto callbackIter = m_callbacks.find(transactionSignature);
        if (callbackIter == m_callbacks.end()) {
            CATAPULT_LOG(debug) << "Handler for " << transactionSignature.data() << " not found";
            return;
        }

        callbackIter->second(hash, status);
        m_callbacks.erase(transactionSignature);
    }
}}
