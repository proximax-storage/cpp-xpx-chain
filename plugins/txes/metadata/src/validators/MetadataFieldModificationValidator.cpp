/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "plugins/txes/metadata/src/model/MetadataTypes.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyMetadataFieldNotification;

	DECLARE_STATELESS_VALIDATOR(MetadataFieldModification, Notification)(uint8_t maxKeySize, uint16_t maxValueSize) {
		return MAKE_STATELESS_VALIDATOR(MetadataFieldModification, ([maxKeySize, maxValueSize](const Notification& notification) {
			if (notification.ModificationType > model::MetadataModificationType::Del) {
				return Failure_Metadata_Modification_Type_Invalid;
			}

			if (notification.KeySize <= 0 || notification.KeySize > maxKeySize) {
				return Failure_Metadata_Modification_Key_Invalid;
			}

			if ((notification.ModificationType == model::MetadataModificationType::Add && notification.ValueSize <= 0) || notification.ValueSize > maxValueSize) {
				return Failure_Metadata_Modification_Value_Invalid;
			}

			return ValidationResult::Success;
		}));
	}
}}
