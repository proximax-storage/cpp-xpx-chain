/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyMetadataFieldNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(MetadataFieldModification, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(MetadataFieldModification, ([](const Notification& notification, const auto& context) {
			if (notification.ModificationType > model::MetadataModificationType::Del) {
				return Failure_Metadata_Modification_Type_Invalid;
			}

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::MetadataConfiguration>();
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
