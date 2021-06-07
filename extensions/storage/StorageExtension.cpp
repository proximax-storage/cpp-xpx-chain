/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/BlockStorageSubscription.h"
#include "src/ReplicatorService.h"
#include "src/notification_handlers/NotificationHandlers.h"
#include "catapult/notification_handlers/DemuxHandlerBuilder.h"

namespace catapult { namespace storage {

	namespace {
		void RegisterExtension(extensions::ProcessBootstrapper& bootstrapper) {
			const auto& config = bootstrapper.config();
			auto keyPair = crypto::KeyPair::FromString(config.User.BootKey);
			auto storageConfig = StorageConfiguration::LoadFromPath(bootstrapper.resourcesPath());
			auto pReplicatorService = std::make_shared<ReplicatorService>(
				std::move(keyPair),
				config.Immutable.NetworkIdentifier,
				config.Immutable.GenerationHash,
				std::move(storageConfig));

			notification_handlers::DemuxHandlerBuilder builder;
			builder
				.add(notification_handlers::CreatePrepareDriveHandler(pReplicatorService))
				.add(notification_handlers::CreateDataModificationHandler(pReplicatorService))
				.add(notification_handlers::CreateDataModificationCancelHandler(pReplicatorService))
				.add(notification_handlers::CreateDownloadHandler(pReplicatorService))
				.add(notification_handlers::CreateReplicatorOnboardingHandler(pReplicatorService));

			bootstrapper.subscriptionManager().addPostBlockCommitSubscriber(
				CreateBlockStorageSubscription(bootstrapper, builder.build()));

			bootstrapper.extensionManager().addServiceRegistrar(CreateReplicatorServiceRegistrar(pReplicatorService));
		}
	}
}}

extern "C" PLUGIN_API
void RegisterExtension(catapult::extensions::ProcessBootstrapper& bootstrapper) {
	catapult::storage::RegisterExtension(bootstrapper);
}
