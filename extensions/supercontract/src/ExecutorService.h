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
#include <catapult/crypto/CurvePoint.h>
#include <set>
#include <optional>

namespace catapult { namespace model { class Transaction; } }

namespace catapult { namespace contract {

    class ExecutorService {
    public:
    	ExecutorService(ExecutorConfiguration&& executorConfig);
    	~ExecutorService();

    public:

        void start();

		void restart();

	public:
		void notifyTransactionStatus(const Hash256& hash, uint32_t status);

	private:

    	void stop();

	public:

    	void setServiceState(extensions::ServiceState* pServiceState);

    	const Key& executorKey() const;

    	bool contractExists(const Key& contractKey);

		std::optional<Height> contractAddedAt(const Key& contractKey);

	public:

		void addManualCall(const Key& contractKey,
						   const Hash256& callId,
						   const std::string& fileName,
						   const std::string& functionName,
						   const std::string& parameters,
						   Amount executionPayment,
						   Amount downloadPayment,
						   const Key& caller,
						   Height height);

		void successfulBatchExecutionPublished(const Key& contractKey,
											   uint64_t batchIndex,
											   const Hash256& driveState,
											   const crypto::CurvePoint& poExVerificationInfo,
											   const std::set<Key>& cosigners);

		void unsuccessfulBatchExecutionPublished(const Key& contractKey,
											   uint64_t batchIndex,
											   const std::set<Key>& cosigners);

		void automaticExecutionBlockPublished(Height height);

		void updateContracts(Height height);

		void batchExecutionSinglePublished(const Key& contractKey, uint64_t batchIndex);

		void synchronizeSinglePublished(const Key& contractKey, uint64_t batchIndex);

		void activateAutomaticExecutions(const Key& contractKey,
										 Height height);

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
