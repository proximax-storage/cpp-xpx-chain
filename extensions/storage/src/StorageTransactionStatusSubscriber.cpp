/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StorageTransactionStatusSubscriber.h"
#include "ReplicatorService.h"

namespace catapult { namespace storage {

	namespace {
		class StorageTransactionStatusSubscriber : public subscribers::TransactionStatusSubscriber {
		public:
			explicit StorageTransactionStatusSubscriber(std::weak_ptr<storage::ReplicatorService> pReplicatorServiceWeak)
				: m_pReplicatorServiceWeak(std::move(pReplicatorServiceWeak))
			{}

		public:
			void notifyStatus(const model::Transaction& transaction, const Height& height, const Hash256& hash, uint32_t status) override {
				auto pReplicatorService = m_pReplicatorServiceWeak.lock();
				if (!pReplicatorService)
					return;

				pReplicatorService->notifyTransactionStatus(transaction, height, hash, status);
			}

			void flush() override {
			}

		private:
			std::weak_ptr<storage::ReplicatorService> m_pReplicatorServiceWeak;
		};
	}

	std::unique_ptr<subscribers::TransactionStatusSubscriber> CreateStorageTransactionStatusSubscriber(
			const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return std::make_unique<StorageTransactionStatusSubscriber>(pReplicatorServiceWeak);
	}
}}
