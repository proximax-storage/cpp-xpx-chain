/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionStatusHandler.h"
#include "catapult/exceptions.h"

namespace catapult { namespace storage {

    void TransactionStatusHandler::addHandler(const Hash256& hash, consumer<uint32_t> handler) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_callbacks[hash] = std::move(handler);
    }

    void TransactionStatusHandler::handle(const Hash256& hash, uint32_t status) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto callbackIter = m_callbacks.find(hash);
        if (callbackIter == m_callbacks.end())
            return;

        callbackIter->second(status);
        m_callbacks.erase(callbackIter);
    }
}}
