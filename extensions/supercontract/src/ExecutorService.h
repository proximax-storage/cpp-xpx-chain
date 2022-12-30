/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExecutorConfiguration.h"
#include "catapult/types.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ServiceRegistrar.h"
#include "catapult/ionet/Node.h"
#include <optional>

namespace catapult { namespace model { class Transaction; } }

namespace catapult { namespace contract {

    class ExecutorService {
    public:
    	ExecutorService(ExecutorConfiguration&& executorConfig);
    	~ExecutorService();

    public:
        void start();
        void stop();

	public:

    	void setServiceState(extensions::ServiceState* pServiceState);

    	const Key& executorKey() const;

    	bool isExecutorRegistered(const Key& key);

    private:
        class Impl;

        std::unique_ptr<Impl> m_pImpl;

        crypto::KeyPair m_keyPair;
        ExecutorConfiguration m_config;
        extensions::ServiceState* m_pServiceState;
    };

    /// Creates a registrar for the executor service.
    DECLARE_SERVICE_REGISTRAR(Executor)(std::shared_ptr<ExecutorService> pExecutorService);
}}
