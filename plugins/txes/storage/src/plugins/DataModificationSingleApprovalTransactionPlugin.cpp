/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "DataModificationSingleApprovalTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/DataModificationSingleApprovalTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(DataModificationSingleApprovalNotification<1>(
							transaction.Signer,
							transaction.DriveKey,
							transaction.DataModificationId,
							transaction.PublicKeysCount,
							transaction.PublicKeysPtr(),
							transaction.OpinionsPtr()
					));

					auto pPublicKeys = std::unique_ptr<Key[]>(new Key[transaction.PublicKeysCount + 1]);
					pPublicKeys[0] = transaction.Signer;
					std::copy(transaction.PublicKeysPtr(), transaction.PublicKeysPtr() + transaction.PublicKeysCount, &pPublicKeys[1]);	// TODO: Double-check

					sub.notify(DataModificationApprovalDownloadWorkNotification<1>(
							transaction.DriveKey,
							transaction.DataModificationId,
							1,
							pPublicKeys.get()
					));

					auto pOpinionIndices = std::unique_ptr<uint8_t[]>(new uint8_t[1]);
					pOpinionIndices[0] = 0;
					const auto presentOpinionByteCount = (transaction.PublicKeysCount + 7) / 8;
					auto pPresentOpinions = std::unique_ptr<uint8_t[]>(new uint8_t[presentOpinionByteCount]);
					for (auto i = 0u; i < presentOpinionByteCount; ++i) {
						boost::dynamic_bitset<uint8_t> byte(8, 0u);
						for (auto j = 0u; j < std::min(8u, transaction.PublicKeysCount - i*8); ++j)
							byte[j] = true;
						boost::to_block_range(byte, &pPresentOpinions[i]);
					}

					sub.notify(DataModificationApprovalUploadWorkNotification<1>(
							transaction.DriveKey,
							1,
							0,
							transaction.PublicKeysCount,
							pPublicKeys.get(),
							pPresentOpinions.get(),
							transaction.OpinionsPtr()
					));

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of DataModificationSingleApprovalTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(DataModificationSingleApproval, Default, CreatePublisher, config::ImmutableConfiguration)
}}
