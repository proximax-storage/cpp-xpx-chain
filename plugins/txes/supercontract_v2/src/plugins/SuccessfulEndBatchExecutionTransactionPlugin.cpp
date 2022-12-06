/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/crypto/Hashes.h>
#include "SuccessfulEndBatchExecutionTransactionPlugin.h"
#include "catapult/model/SupercontractNotifications.h"
#include "src/model/SuccessfulEndBatchExecutionTransaction.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/NotificationSubscriber.h"
#include "src/utils/SwapUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {

		template<class T>
		void pushBytes(std::vector<uint8_t>& buffer, const T& data) {
			const auto* pBegin = reinterpret_cast<const uint8_t*>(data);
			buffer.insert(buffer.end(), pBegin, pBegin + sizeof(data));
		}

		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {

					Key ContractKey;

					uint64_t BatchId;

					Hash256 StorageHash;

					std::array<uint8_t, 32> ProofOfExecutionVerificationInformation;

					std::vector<uint8_t> commonData;
					commonData.insert(
							commonData.end(),
							reinterpret_cast<const uint8_t*>(&transaction.ContractKey),
							sizeof(transaction.ContractKey));

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of SuccessfulEndBatchExecutionTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(SuccessfulEndBatchExecution, Default, CreatePublisher, config::ImmutableConfiguration)
}}
