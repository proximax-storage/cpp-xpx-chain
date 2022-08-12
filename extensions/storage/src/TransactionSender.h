/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <utility>

#include "StorageConfiguration.h"
#include "src/catapult/crypto/KeyPair.h"
#include "catapult/utils/NetworkTime.h"
#include "catapult/extensions/ServerHooks.h"
#include "catapult/state/StorageState.h"
#include "drive/FlatDrive.h"

namespace catapult { namespace storage {
    class TransactionSender {
    public:
        TransactionSender(
				const crypto::KeyPair& keyPair,
				const config::ImmutableConfiguration& immutableConfig,
				StorageConfiguration storageConfig,
				handlers::TransactionRangeHandler transactionRangeHandler,
				state::StorageState& storageState)
			: m_keyPair(keyPair)
			, m_networkIdentifier(immutableConfig.NetworkIdentifier)
			, m_generationHash(immutableConfig.GenerationHash)
			, m_storageConfig(std::move(storageConfig))
			, m_transactionRangeHandler(std::move(transactionRangeHandler))
			, m_storageState(storageState)
		{}

    public:
        Hash256 sendDataModificationApprovalTransaction(const sirius::drive::ApprovalTransactionInfo& transactionInfo);
        Hash256 sendDataModificationSingleApprovalTransaction(const sirius::drive::ApprovalTransactionInfo& transactionInfo);
        Hash256 sendDownloadApprovalTransaction(const sirius::drive::DownloadApprovalTransactionInfo& transactionInfo);
        Hash256 sendEndDriveVerificationTransaction(const sirius::drive::VerifyApprovalTxInfo& transactionInfo);

	private:
        void send(std::shared_ptr<model::Transaction> pTransaction);

    private:
		const crypto::KeyPair& m_keyPair;
        model::NetworkIdentifier m_networkIdentifier;
        GenerationHash m_generationHash;
        StorageConfiguration m_storageConfig;
        handlers::TransactionRangeHandler m_transactionRangeHandler;
		state::StorageState& m_storageState;
    };
}}
