/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyMetadataV1FieldNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(MetadataV1FieldModification, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(MetadataV1FieldModification, ([](const Notification& notification, const auto& context) {
			if (notification.ModificationType > model::MetadataV1ModificationType::Del) {
				return Failure_Metadata_Modification_Type_Invalid;
			}

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::MetadataV1Configuration>();
			if (notification.KeySize <= 0 || notification.KeySize > pluginConfig.MaxFieldKeySize) {
				return Failure_Metadata_Modification_Key_Invalid;
			}

			if ((notification.ModificationType == model::MetadataV1ModificationType::Add && notification.ValueSize <= 0) || notification.ValueSize > pluginConfig.MaxFieldValueSize) {
				return Failure_Metadata_Modification_Value_Invalid;
			}

			return ValidationResult::Success;
		}));
	}
}}
