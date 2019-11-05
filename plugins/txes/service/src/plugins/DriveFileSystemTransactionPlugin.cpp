/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "DriveFileSystemTransactionPlugin.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/DriveFileSystemTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "sdk/src/extensions/ConversionExtensions.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder> &pConfigHolder) {
			return [pConfigHolder](const TTransaction &transaction, const Height &associatedHeight,
								   NotificationSubscriber &sub) {
				auto &blockChainConfig = pConfigHolder->ConfigAtHeightOrLatest(associatedHeight);
				switch (transaction.EntityVersion()) {
					case 1: {
					    sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
						sub.notify(DriveFileSystemNotification<1>(
							transaction.DriveKey,
							transaction.Signer,
							transaction.RootHash,
							transaction.XorRootHash,
							transaction.AddActionsCount,
							transaction.AddActionsPtr(),
							transaction.RemoveActionsCount,
							transaction.RemoveActionsPtr()
						));

						auto addActionsPtr = transaction.AddActionsPtr();
						auto driveAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, blockChainConfig.Immutable.NetworkIdentifier));
						auto streamingMosaicId = UnresolvedMosaicId(blockChainConfig.Immutable.StreamingMosaicId.unwrap());

						for (auto i = 0u; i < transaction.AddActionsCount; ++i, ++addActionsPtr) {
							// TODO: Fix memory leak
							auto deposit = new model::FileUpload{ transaction.DriveKey, addActionsPtr->FileSize };
							sub.notify(BalanceTransferNotification<1>(
								transaction.Signer,
								driveAddress,
								streamingMosaicId,
								UnresolvedAmount(0, UnresolvedAmountType::FileUpload, reinterpret_cast<uint8_t*>(deposit))
							));
						}

						break;
					}
					default:
						CATAPULT_LOG(debug) << "invalid version of DriveFileSystemTransaction: "
											<< transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(DriveFileSystem, Default, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
