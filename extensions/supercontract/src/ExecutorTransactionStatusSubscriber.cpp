/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExecutorTransactionStatusSubscriber.h"
#include "ExecutorService.h"

namespace catapult { namespace contract {

	namespace {
		class ExecutorTransactionStatusSubscriber : public subscribers::TransactionStatusSubscriber {
		public:
			explicit ExecutorTransactionStatusSubscriber(std::weak_ptr<contract::ExecutorService> pExecutorServiceWeak)
			: m_pExecutorServiceWeak(std::move(pExecutorServiceWeak))
			{}

		public:
			void notifyStatus(const model::Transaction&, const Height&, const Hash256& hash, uint32_t status) override {
				auto pExecutorService = m_pExecutorServiceWeak.lock();
				if (!pExecutorService)
					return;

				pExecutorService->notifyTransactionStatus(hash, status);
			}

			void flush() override {
			}

		private:
			std::weak_ptr<contract::ExecutorService> m_pExecutorServiceWeak;
		};
	}

	std::unique_ptr<subscribers::TransactionStatusSubscriber> CreateExecutorTransactionStatusSubscriber(
			const std::weak_ptr<contract::ExecutorService>& pExecutorServiceWeak) {
		return std::make_unique<ExecutorTransactionStatusSubscriber>(pExecutorServiceWeak);
	}
}}
