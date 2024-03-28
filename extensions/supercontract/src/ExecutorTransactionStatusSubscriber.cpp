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
			explicit ExecutorTransactionStatusSubscriber(
					std::weak_ptr<TransactionStatusHandler>&& pTransactionStatusHandlerWeak)
				: m_pTransactionStatusHandlerWeak(std::move(pTransactionStatusHandlerWeak)) {}

		public:
			void notifyStatus(const model::Transaction&, const Height&, const Hash256& hash, uint32_t status) override {
				auto pTransactionStatusHandler = m_pTransactionStatusHandlerWeak.lock();
				if (!pTransactionStatusHandler)
					return;

				pTransactionStatusHandler->handle(hash, status);
			}

			void flush() override {}

		private:
			std::weak_ptr<TransactionStatusHandler> m_pTransactionStatusHandlerWeak;
		};
	} // namespace

	std::unique_ptr<subscribers::TransactionStatusSubscriber> CreateExecutorTransactionStatusSubscriber(
			std::weak_ptr<TransactionStatusHandler> pTransactionStatusHandlerWeak) {
		return std::make_unique<ExecutorTransactionStatusSubscriber>(std::move(pTransactionStatusHandlerWeak));
	}
}} // namespace catapult::contract
