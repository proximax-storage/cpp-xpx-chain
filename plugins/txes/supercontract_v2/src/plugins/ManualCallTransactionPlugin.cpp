/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/crypto/Hashes.h>
#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "ManualCallTransactionPlugin.h"
#include "catapult/model/SupercontractNotifications.h"
#include "src/model/ManualCallTransaction.h"
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

					auto contractKey = Key(transaction.ContractKey);
					sub.notify(AccountPublicKeyNotification<1>(contractKey));

					Hash256 paymentHash;
					crypto::Sha3_256_Builder sha3;
					sha3.update({contractKey, config.GenerationHash});
					sha3.final(paymentHash);
					auto contractExecutionPaymentKey = Key(paymentHash.array());

					const std::string fileName(reinterpret_cast<const char*>(transaction.FileNamePtr()), transaction.FileNameSize);
					const std::string functionName(reinterpret_cast<const char*>(transaction.FunctionNamePtr()), transaction.FunctionNameSize);
					const std::string actualArguments(reinterpret_cast<const char*>(transaction.ActualArgumentsPtr()), transaction.ActualArgumentsSize);

					const auto* const pServicePayment = transaction.ServicePaymentsPtr();
					std::vector<UnresolvedMosaic> servicePayments;
					servicePayments.reserve(transaction.ServicePaymentsCount);
					for (auto i = 0U; i < transaction.ServicePaymentsCount; i++) {
						servicePayments.push_back(pServicePayment[i]);
					}

					const auto contractAddress = extensions::CopyToUnresolvedAddress(
							PublicKeyToAddress(contractKey, config.NetworkIdentifier));
					for (const auto& servicePayment : servicePayments) {
						sub.notify(BalanceTransferNotification<1>(
								transaction.Signer, contractAddress, servicePayment.MosaicId, servicePayment.Amount));
					}

					sub.notify(ManualCallNotification<1>(
							contractKey,
							txHash,
							transaction.Signer,
							fileName,
							functionName,
							actualArguments,
							transaction.ExecutionCallPayment,
							transaction.DownloadCallPayment,
							servicePayments));

					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);
					const auto scMosaicId = config::GetUnresolvedSuperContractMosaicId(config);

					const auto pCallExecutionWork =
							sub.mempool().malloc(model::ExecutorWork(contractKey, transaction.ExecutionCallPayment));
					utils::SwapMosaics(
							transaction.Signer,
							contractExecutionPaymentKey,
							{ std::make_pair(
									scMosaicId,
									UnresolvedAmount(
											0, UnresolvedAmountType::ExecutorWork, pCallExecutionWork)) },
							sub,
							config,
							utils::SwapOperation::Buy);

					const auto pCallDownloadWork =
							sub.mempool().malloc(model::ExecutorWork(contractKey, transaction.ExecutionCallPayment));
					utils::SwapMosaics(
							transaction.Signer,
							contractExecutionPaymentKey,
							{ std::make_pair(
									streamingMosaicId,
									UnresolvedAmount(
											0, UnresolvedAmountType::ExecutorWork, pCallDownloadWork)) },
							sub,
							config,
							utils::SwapOperation::Buy);
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of ManualCallTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(ManualCall, Default, CreatePublisher, config::ImmutableConfiguration)
}}
