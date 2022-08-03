/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/BlockStorageSubscription.h"
#include "src/ReplicatorService.h"
#include "src/StorageTransactionStatusSubscriber.h"
#include "src/notification_handlers/NotificationHandlers.h"
#include "catapult/notification_handlers/DemuxHandlerBuilder.h"

namespace catapult { namespace storage {

	namespace {
		void RegisterExtension(extensions::ProcessBootstrapper& bootstrapper) {
			const auto& config = bootstrapper.config();
			auto keyPair = crypto::KeyPair::FromString(config.User.BootKey); // TODO: get the key from the storage config.
			auto storageConfig = StorageConfiguration::LoadFromPath(bootstrapper.resourcesPath());
			auto replicatorsFile = boost::filesystem::path(bootstrapper.resourcesPath()) / "replicators.json";
			auto bootstrapReplicators = config::LoadPeersFromPath(replicatorsFile.generic_string(), bootstrapper.config().Immutable.NetworkIdentifier);
			auto pReplicatorService = std::make_shared<ReplicatorService>(
				std::move(keyPair),
				std::move(storageConfig),
				std::move(bootstrapReplicators));

			notification_handlers::DemuxHandlerBuilder builder;
			builder
				.add(notification_handlers::CreateDataModificationApprovalHandler(pReplicatorService))
				.add(notification_handlers::CreateDataModificationCancelHandler(pReplicatorService))
				.add(notification_handlers::CreateDataModificationHandler(pReplicatorService))
				.add(notification_handlers::CreateDataModificationSingleApprovalHandler(pReplicatorService))
				.add(notification_handlers::CreateDownloadApprovalHandler(pReplicatorService))
				.add(notification_handlers::CreateDownloadHandler(pReplicatorService))
				.add(notification_handlers::CreateDownloadPaymentHandler(pReplicatorService))
				.add(notification_handlers::CreateDriveClosureHandler(pReplicatorService))
				.add(notification_handlers::CreateEndDriveVerificationHandler(pReplicatorService))
				.add(notification_handlers::CreateFinishDownloadHandler(pReplicatorService))
				.add(notification_handlers::CreatePeriodicStoragePaymentHandler(pReplicatorService))
				.add(notification_handlers::CreatePeriodicDownloadPaymentHandler(pReplicatorService))
				.add(notification_handlers::CreatePrepareDriveHandler(pReplicatorService))
				.add(notification_handlers::CreateReplicatorOffboardingHandler(pReplicatorService))
				.add(notification_handlers::CreateReplicatorOnboardingHandler(pReplicatorService))
				.add(notification_handlers::CreateVerificationHandler(pReplicatorService));

			bootstrapper.subscriptionManager().addPostBlockCommitSubscriber(
				CreateBlockStorageSubscription(bootstrapper, builder.build()));

			bootstrapper.extensionManager().addServiceRegistrar(CreateReplicatorServiceRegistrar(pReplicatorService));
			bootstrapper.subscriptionManager().addTransactionStatusSubscriber(CreateStorageTransactionStatusSubscriber(pReplicatorService));
		}
	}
}}

extern "C" PLUGIN_API
void RegisterExtension(catapult::extensions::ProcessBootstrapper& bootstrapper) {
	catapult::storage::RegisterExtension(bootstrapper);
}
