/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <map>
#include <mutex>
#include <cstdint>
#include <functional>
#include "catapult/types.h"

namespace catapult { namespace storage {
    using Callback = std::function<void(const Hash256& hash, uint32_t status)>;

    class TransactionStatusHandler {
    public:
        TransactionStatusHandler() = default;

    public:

        /// addHandler saves a handler in map with transactionSignature key
        void addHandler(catapult::Signature& transactionSignature, Callback handler);

        /// handle finds a handler by transactionSignature and calls it
        void handle(const catapult::Signature& transactionSignature, const Hash256& hash, uint32_t status);

    private:
        std::mutex m_mutex;
        std::map<catapult::Signature, Callback> m_callbacks;
    };
}}
