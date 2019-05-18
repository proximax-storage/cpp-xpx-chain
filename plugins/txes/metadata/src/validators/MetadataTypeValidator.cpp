/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::MetadataTypeNotification<1>;

	DEFINE_STATELESS_VALIDATOR(MetadataType, [](const auto& notification) {
		return model::MetadataType::Address <= notification.MetadataType && notification.MetadataType <= model::MetadataType::NamespaceId?
			ValidationResult::Success : Failure_Metadata_Invalid_Metadata_Type;
	});
}}
