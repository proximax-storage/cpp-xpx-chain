/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "DeployContractTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "catapult/model/SupercontractNotifications.h"
#include "src/model/DeployContractTransaction.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/EntityHasher.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {

					auto txHash = CalculateHash(transaction, config.GenerationHash);

					auto contractKey = Key(txHash.array());
					sub.notify(AccountPublicKeyNotification<1>(contractKey));
					sub.notify(DeploySupercontractDriveNotification<1>(
							transaction.Signer,
							transaction.DriveKey
					));

					const std::string automaticExecutionsFileName(reinterpret_cast<const char*>(transaction.AutomaticExecutionFileNamePtr()), transaction.AutomaticExecutionFileNameSize);
					const std::string automaticExecutionsFunctionName(reinterpret_cast<const char*>(transaction.AutomaticExecutionFunctionNamePtr()), transaction.AutomaticExecutionFunctionNameSize);

					sub.notify(DeploySupercontractNotification<1>(
							contractKey,
							transaction.DriveKey,
							transaction.Assignee,
							automaticExecutionsFileName,
							automaticExecutionsFunctionName,
							transaction.AutomaticExecutionCallPayment,
							transaction.AutomaticDownloadCallPayment));

					const std::string initializerFileName(reinterpret_cast<const char*>(transaction.FileNamePtr()), transaction.FileNameSize);
					const std::string initializerFunctionName(reinterpret_cast<const char*>(transaction.FunctionNamePtr()), transaction.FunctionNameSize);
					const std::string initializerActualArguments(reinterpret_cast<const char*>(transaction.ActualArgumentsPtr()), transaction.ActualArgumentsSize);

					const auto* const pServicePayment = transaction.ServicePaymentsPtr();
					std::vector<UnresolvedMosaic> servicePayments;
					servicePayments.reserve(transaction.ServicePaymentsCount);
					for (auto i = 0U; i < transaction.ServicePaymentsCount; i++) {
						servicePayments.push_back(pServicePayment[i]);
					}

					sub.notify(AutomaticExecutionsReplenishmentNotification<1>(
							contractKey, transaction.AutomaticExecutionsNumber));

					sub.notify(ManualCallNotification<1>(
							contractKey,
							txHash,
							transaction.Signer,
							initializerFileName,
							initializerFunctionName,
							initializerActualArguments,
							transaction.ExecutionCallPayment,
							transaction.DownloadCallPayment,
							servicePayments));

					//					const auto driveAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(driveKey, config.NetworkIdentifier));
//					const auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);
//					const auto storageMosaicId = config::GetUnresolvedStorageMosaicId(config);
//					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);

//					sub.notify(BalanceTransferNotification<1>(
//							transaction.Signer, driveAddress, currencyMosaicId, transaction.VerificationFeeAmount));
//					utils::SwapMosaics(
//					 		transaction.Signer,
//					 		driveKey,
//					 		{ {storageMosaicId, Amount(transaction.DriveSize * transaction.ReplicatorCount)} },
//					 		sub,
//					 		config,
//					 		utils::SwapOperation::Buy);
//					utils::SwapMosaics(
//					 		transaction.Signer,
//					 		driveKey,
//					 		{ {streamingMosaicId, Amount(2 * transaction.DriveSize * transaction.ReplicatorCount)} },
//					 		sub,
//					 		config,
//					 		utils::SwapOperation::Buy);

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of PrepareBcDriveTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(DeployContract, Default, CreatePublisher, config::ImmutableConfiguration)
}}
