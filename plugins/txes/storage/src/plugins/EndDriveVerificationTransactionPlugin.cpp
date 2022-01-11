/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndDriveVerificationTransactionPlugin.h"
#include "src/model/InternalStorageNotifications.h"
#include "src/model/EndDriveVerificationTransaction.h"
#include "src/utils/StorageUtils.h"
#include "catapult/model/TransactionPluginFactory.h"
#include <boost/dynamic_bitset.hpp>

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
        template<typename TTransaction>
        auto CreatePublisher(const config::ImmutableConfiguration& config) {
            return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
                switch (transaction.EntityVersion()) {
                    case 1: {
                        sub.notify(EndDriveVerificationNotification<1>(
							transaction.DriveKey,
							transaction.VerificationTrigger,
							transaction.ShardId,
							transaction.KeyCount,
							transaction.JudgingKeyCount,
							transaction.PublicKeysPtr(),
							transaction.SignaturesPtr(),
							transaction.OpinionsPtr()));

						auto numBits = transaction.KeyCount * transaction.JudgingKeyCount;
						boost::dynamic_bitset<uint8_t> presentOpinions(numBits);
						// All opinions are set except about itself.
						auto multiplier = transaction.KeyCount + 1;
						for (auto i = 0u; i < numBits; ++i)
							presentOpinions[i] = (i % multiplier == 0) ? 0 : 1;

						auto* pPresentOpinions = sub.mempool().malloc<uint8_t>((transaction.KeyCount * transaction.JudgingKeyCount + 7u) / 8u);
						boost::to_block_range(presentOpinions, pPresentOpinions);

						sub.notify(OpinionNotification<1>(
							Key_Size + Hash256_Size + sizeof(uint16_t),
							0u,
							transaction.JudgingKeyCount,
							transaction.KeyCount - transaction.JudgingKeyCount,
							sizeof(uint8_t),
							transaction.DriveKey.data(),
							transaction.PublicKeysPtr(),
							transaction.SignaturesPtr(),
							pPresentOpinions,
							transaction.OpinionsPtr()));

                        break;
                    }

                    default:
                        CATAPULT_LOG(debug) << "invalid version of EndDriveVerificationTransaction: " << transaction.EntityVersion();
                }
            };
        }
    }

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(EndDriveVerification, Default, CreatePublisher, config::ImmutableConfiguration)
}}
