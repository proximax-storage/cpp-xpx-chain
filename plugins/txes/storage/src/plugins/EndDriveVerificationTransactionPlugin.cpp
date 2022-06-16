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
#include "catapult/model/EntityHasher.h"
#include <boost/dynamic_bitset.hpp>

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
        template<typename TTransaction>
        auto CreatePublisher(const config::ImmutableConfiguration& config) {
            return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
                switch (transaction.EntityVersion()) {
                    case 1: {
						auto hashSeed = CalculateHash(transaction, config.GenerationHash);
                        sub.notify(EndDriveVerificationNotification<1>(
							transaction.DriveKey,
							hashSeed,
							transaction.VerificationTrigger,
							transaction.ShardId,
							transaction.KeyCount,
							transaction.JudgingKeyCount,
							transaction.PublicKeysPtr(),
							transaction.SignaturesPtr(),
							transaction.OpinionsPtr()));

						auto totalOpinions = transaction.KeyCount * transaction.JudgingKeyCount;
						auto opinionByteCount = (totalOpinions + 7u) / 8u;
						boost::dynamic_bitset<uint8_t> opinionsBitset(transaction.OpinionsPtr(), transaction.OpinionsPtr() + opinionByteCount);
						auto* const pOpinionsBegin = sub.mempool().malloc<uint8_t>(opinionByteCount);
						auto* pOpinions = pOpinionsBegin;
						for (uint i = 0; i < totalOpinions; i++) {
							*pOpinions = opinionsBitset[i];
							pOpinions++;
						}

						const auto commonDataSize = Key_Size + Hash256_Size + sizeof(uint16_t);
						auto* const pCommonDataBegin = sub.mempool().malloc<uint8_t>(commonDataSize);
						auto* pCommonData = pCommonDataBegin;
						utils::WriteToByteArray(pCommonData, transaction.DriveKey);
						utils::WriteToByteArray(pCommonData, transaction.VerificationTrigger);
						utils::WriteToByteArray(pCommonData, transaction.ShardId);

						sub.notify(OpinionNotification<1>(
							commonDataSize,
							0u,
							transaction.JudgingKeyCount,
							transaction.KeyCount - transaction.JudgingKeyCount,
							sizeof(uint8_t),
							pCommonDataBegin,
							transaction.PublicKeysPtr(),
							transaction.SignaturesPtr(),
							nullptr,
							pOpinionsBegin));

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
