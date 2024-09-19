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
			auto storageConfig = StorageConfiguration::LoadFromPath(bootstrapper.resourcesPath());
			auto replicatorsFile = boost::filesystem::path(bootstrapper.resourcesPath()) / "replicators.json";
			auto bootstrapReplicators = config::LoadPeersFromPath(replicatorsFile.generic_string(), bootstrapper.config().Immutable.NetworkIdentifier);
			auto pReplicatorService = std::make_shared<ReplicatorService>(std::move(storageConfig), std::move(bootstrapReplicators));

			notification_handlers::DemuxHandlerBuilder builder;
			builder.add(notification_handlers::CreateHealthCheckHandler(pReplicatorService));
			auto& subscriptionManager = bootstrapper.subscriptionManager();
			subscriptionManager.addPostBlockCommitSubscriber(CreateBlockStorageSubscription(bootstrapper, builder.build()));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateDataModificationApprovalServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateDataModificationCancelServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateDataModificationServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateDataModificationSingleApprovalServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateDownloadApprovalServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateDownloadPaymentServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateDownloadRewardServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateDownloadServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateDrivesUpdateServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateEndDriveVerificationServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreatePrepareDriveServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateReplicatorOnboardingServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateStartDriveVerificationServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateStreamFinishServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateStreamPaymentServiceHandler(pReplicatorService));
			subscriptionManager.addNotificationSubscriber(notification_handlers::CreateStreamStartServiceHandler(pReplicatorService));

			bootstrapper.extensionManager().addServiceRegistrar(CreateReplicatorServiceRegistrar(pReplicatorService));
			subscriptionManager.addTransactionStatusSubscriber(CreateStorageTransactionStatusSubscriber(pReplicatorService));
		}
	}
}}

extern "C" PLUGIN_API
void RegisterExtension(catapult::extensions::ProcessBootstrapper& bootstrapper) {
	catapult::storage::RegisterExtension(bootstrapper);
}
