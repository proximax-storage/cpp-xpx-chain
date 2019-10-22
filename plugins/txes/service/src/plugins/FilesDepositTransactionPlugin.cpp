/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "FilesDepositTransactionPlugin.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/FilesDepositTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		UnresolvedAddress CopyToUnresolvedAddress(const Address& address) {
			UnresolvedAddress dest;
			std::memcpy(dest.data(), address.data(), address.size());
			return dest;
		}

		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder> &pConfigHolder) {
			return [pConfigHolder](const TTransaction &transaction, const Height &associatedHeight,
								   NotificationSubscriber &sub) {
				auto &blockChainConfig = pConfigHolder->ConfigAtHeightOrLatest(associatedHeight);
				switch (transaction.EntityVersion()) {
					case 3: {
						sub.notify(FilesDepositNotification<1>(
							transaction.DriveKey,
							transaction.Signer,
							transaction.FilesCount,
							transaction.FilesPtr()
						));

						auto filesPtr = transaction.FilesPtr();
						auto driveAddress = CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, blockChainConfig.Immutable.NetworkIdentifier));
						auto streamingMosaicId = UnresolvedMosaicId(blockChainConfig.Immutable.StreamingMosaicId.unwrap());

						for (auto i = 0u; i < transaction.FilesCount; ++i, ++filesPtr) {
							// TODO: Fix memory leak
							auto deposit = new model::FileDeposit{ transaction.DriveKey, filesPtr->FileHash };
							sub.notify(BalanceTransferNotification<1>(
								transaction.Signer,
								driveAddress,
								streamingMosaicId,
								UnresolvedAmount(0, UnresolvedAmountType::FileDeposit, reinterpret_cast<uint8_t*>(deposit))
							));
						}

						break;
					}

					default:
						CATAPULT_LOG(debug) << "invalid version of FilesDepositTransaction: "
											<< transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(FilesDeposit, Default, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
