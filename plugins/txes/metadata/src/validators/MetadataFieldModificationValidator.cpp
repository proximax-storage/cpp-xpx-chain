/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "plugins/txes/metadata/src/model/MetadataTypes.h"
#include "src/config/MetadataConfiguration.h"
#include "catapult/model/BlockChainConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyMetadataFieldNotification<1>;

	DECLARE_STATELESS_VALIDATOR(MetadataFieldModification, Notification)(const model::BlockChainConfiguration& blockChainConfig) {
		return MAKE_STATELESS_VALIDATOR(MetadataFieldModification, ([&blockChainConfig](const Notification& notification) {
			if (notification.ModificationType > model::MetadataModificationType::Del) {
				return Failure_Metadata_Modification_Type_Invalid;
			}

			const auto& pluginConfig = blockChainConfig.GetPluginConfiguration<config::MetadataConfiguration>("catapult.plugins.metadata");
			if (notification.KeySize <= 0 || notification.KeySize > pluginConfig.MaxFieldKeySize) {
				return Failure_Metadata_Modification_Key_Invalid;
			}

			if ((notification.ModificationType == model::MetadataModificationType::Add && notification.ValueSize <= 0) || notification.ValueSize > pluginConfig.MaxFieldValueSize) {
				return Failure_Metadata_Modification_Value_Invalid;
			}

			return ValidationResult::Success;
		}));
	}
}}
