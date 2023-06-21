/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/crypto/Hashes.h>
#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "DeployContractTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "catapult/model/SupercontractNotifications.h"
#include "src/model/DeployContractTransaction.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/EntityHasher.h"
#include "src/utils/SwapUtils.h"

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

					sub.notify(AccountPublicKeyNotification<1>(transaction.Assignee));

					Hash256 paymentHash;
					crypto::Sha3_256_Builder sha3;
					sha3.update({contractKey, config.GenerationHash});
					sha3.final(paymentHash);
					auto contractExecutionPaymentKey = Key(paymentHash.array());
					sub.notify(AccountPublicKeyNotification<1>(contractExecutionPaymentKey));

					sub.notify(OwnerManagementProhibitionNotification<1>(
							transaction.Signer,
							transaction.DriveKey
					));

					const std::string automaticExecutionsFileName(reinterpret_cast<const char*>(transaction.AutomaticExecutionsFileNamePtr()), transaction.AutomaticExecutionsFileNameSize);
					const std::string automaticExecutionsFunctionName(reinterpret_cast<const char*>(transaction.AutomaticExecutionsFunctionNamePtr()), transaction.AutomaticExecutionsFunctionNameSize);

					sub.notify(DeploySupercontractNotification<1>(
							transaction.Signer,
							contractKey,
							transaction.DriveKey,
							contractExecutionPaymentKey,
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

					for (const auto& servicePayment : servicePayments) {
						sub.notify(BalanceDebitNotification<1>(
								transaction.Signer, servicePayment.MosaicId, servicePayment.Amount));
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

					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);
					const auto scMosaicId = config::GetUnresolvedSuperContractMosaicId(config);

					const auto pAutomaticExecutionsWork = sub.mempool().malloc(
							model::AutomaticExecutorWork(contractKey, transaction.AutomaticExecutionsNumber));
					utils::SwapMosaics(
							transaction.Signer,
							contractExecutionPaymentKey,
							{ std::make_pair(
									scMosaicId,
									UnresolvedAmount(
											0,
											UnresolvedAmountType::AutomaticExecutionsWork,
											pAutomaticExecutionsWork)) },
							sub,
							config,
							utils::SwapOperation::Buy);

					const auto pAutomaticDownloadWork = sub.mempool().malloc(
							model::AutomaticExecutorWork(contractKey, transaction.AutomaticExecutionsNumber));
					utils::SwapMosaics(
							transaction.Signer,
							contractExecutionPaymentKey,
							{ std::make_pair(
									streamingMosaicId,
									UnresolvedAmount(
											0, UnresolvedAmountType::AutomaticExecutionsWork, pAutomaticDownloadWork)) },
							sub,
							config,
							utils::SwapOperation::Buy);

					const auto pInitializerExecutionWork =
							sub.mempool().malloc(model::ExecutorWork(contractKey, transaction.ExecutionCallPayment));
					utils::SwapMosaics(
							transaction.Signer,
							contractExecutionPaymentKey,
							{ std::make_pair(
									scMosaicId,
									UnresolvedAmount(
											0, UnresolvedAmountType::ExecutorWork, pInitializerExecutionWork)) },
							sub,
							config,
							utils::SwapOperation::Buy);

					const auto pInitializerDownloadWork =
							sub.mempool().malloc(model::ExecutorWork(contractKey, transaction.ExecutionCallPayment));
					utils::SwapMosaics(
							transaction.Signer,
							contractExecutionPaymentKey,
							{ std::make_pair(
									streamingMosaicId,
									UnresolvedAmount(
											0, UnresolvedAmountType::ExecutorWork, pInitializerDownloadWork)) },
							sub,
							config,
							utils::SwapOperation::Buy);
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of DeployContractTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(DeployContract, Default, CreatePublisher, config::ImmutableConfiguration)
}}
