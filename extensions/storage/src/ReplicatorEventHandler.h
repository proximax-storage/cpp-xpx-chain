/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "drive/Replicator.h"
#include <memory>
#include <catapult/crypto/KeyPair.h>
#include <catapult/thread/IoThreadPool.h>
#include <catapult/cache/ReadOnlyCatapultCache.h>

namespace catapult {
	namespace storage {
		class TransactionSender;
		class TransactionStatusHandler;
	}
	namespace state {
		class StorageState;
	}
}

namespace catapult { namespace storage {

	class ReplicatorEventHandler : public sirius::drive::ReplicatorEventHandler {
	public:
		void setReplicator(const std::shared_ptr<sirius::drive::Replicator>& pReplicator){
			m_pReplicator = pReplicator;
		}

	protected:
		std::weak_ptr<sirius::drive::Replicator> m_pReplicator;
	};

    std::unique_ptr<ReplicatorEventHandler> CreateReplicatorEventHandler(
		TransactionSender&& transactionSender,
		state::StorageState& storageState,
		TransactionStatusHandler& m_transactionStatusHandler,
		const crypto::KeyPair& keyPair,
		const cache::ReadOnlyCatapultCache& cache);
}}
