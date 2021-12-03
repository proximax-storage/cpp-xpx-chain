/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <map>
#include <mutex>
#include <cstdint>
#include <functional>
#include "catapult/types.h"

namespace catapult { namespace storage {
    using Callback = std::function<void(const Hash256& hash, uint32_t status)>;

    struct OperationContainer {
    public:
        void Emplace(catapult::Signature& signature, Callback callback) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_callbacks[std::move(signature)] = std::move(callback);
        }

        void Call(const catapult::Signature& signature, const Hash256& hash, uint32_t status) {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto callbackIter = m_callbacks.find(signature);
            if (callbackIter == m_callbacks.end())
                return;

            callbackIter->second(hash, status);
            m_callbacks.erase(signature);
        }

    private:
        std::mutex m_mutex;
        std::map<catapult::Signature, Callback> m_callbacks;
    };
}}
