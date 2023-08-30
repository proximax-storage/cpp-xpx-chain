/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/BlockStorageSubscription.h"
#include "src/ExecutorService.h"
#include "src/ExecutorTransactionStatusSubscriber.h"
#include "src/notification_handlers/NotificationHandlers.h"
#include "catapult/notification_handlers/DemuxHandlerBuilder.h"

namespace catapult { namespace contract {

	namespace {
		void RegisterExtension(extensions::ProcessBootstrapper& bootstrapper) {
			const auto& config = bootstrapper.config();

			auto storageConfig = ExecutorConfiguration::LoadFromPath(bootstrapper.resourcesPath());
			auto pTransactionStatusHandler = std::make_shared<TransactionStatusHandler>();
			auto pExecutorService = std::make_shared<ExecutorService>(
					std::move(storageConfig), pTransactionStatusHandler);
			notification_handlers::DemuxHandlerBuilder builder;
			builder
			.add(notification_handlers::CreateManualCallHandler(pExecutorService))
			.add(notification_handlers::CreateAutomaticExecutionsReplenishmentHandler(pExecutorService))
			.add(notification_handlers::CreateSuccessfulBatchExecutionHandler(pExecutorService))
			.add(notification_handlers::CreateUnsuccessfulBatchExecutionHandler(pExecutorService))
			.add(notification_handlers::CreateBatchExecutionSingleHandler(pExecutorService))
			.add(notification_handlers::CreateSynchronizeSingleHandler(pExecutorService))
			.add(notification_handlers::CreateAutomaticExecutionsBlockHandler(pExecutorService))
			.add(notification_handlers::CreateContractsUpdateHandler(pExecutorService));


			bootstrapper.subscriptionManager().addPostBlockCommitSubscriber(
				CreateBlockStorageSubscription(bootstrapper, builder.build()));

			bootstrapper.extensionManager().addServiceRegistrar(CreateExecutorServiceRegistrar(pExecutorService));
			bootstrapper.subscriptionManager().addTransactionStatusSubscriber(CreateExecutorTransactionStatusSubscriber(pTransactionStatusHandler));
		}
	}
}}

extern "C" PLUGIN_API
void RegisterExtension(catapult::extensions::ProcessBootstrapper& bootstrapper) {
	catapult::contract::RegisterExtension(bootstrapper);
}
