/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <map>
#include <mutex>
#include <cstdint>
#include "catapult/functions.h"
#include "catapult/types.h"

namespace catapult { namespace contract {

    class TransactionStatusHandler {
    public:
        TransactionStatusHandler() = default;

    public:

        /// addHandler saves a handler in map with transactionSignature key
        void addHandler(const Hash256& hash, consumer<uint32_t> handler);

        /// handle finds a handler by transactionSignature and calls it
        void handle(const Hash256& hash, uint32_t status);

    private:
        std::mutex m_mutex;
        std::map<Hash256, consumer<uint32_t>> m_callbacks;
    };
}}
