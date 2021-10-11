/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/StreamingConfiguration.h"
#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::StreamStartFolderNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(StreamStartFolder, [](const Notification& notification, const ValidatorContext& context) {
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StreamingConfiguration>();
		return notification.FolderSize > pluginConfig.MaxFolderSize ? Failure_Streaming_Folder_Too_Large : ValidationResult::Success;
	});

}}
