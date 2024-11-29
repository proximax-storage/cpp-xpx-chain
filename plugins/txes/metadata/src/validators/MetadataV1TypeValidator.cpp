/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::MetadataV1TypeNotification<1>;

	DEFINE_STATELESS_VALIDATOR(MetadataV1Type, [](const auto& notification) {
		return model::MetadataV1Type::Address <= notification.MetadataType && notification.MetadataType <= model::MetadataV1Type::NamespaceId?
			ValidationResult::Success : Failure_Metadata_Invalid_Metadata_Type;
	});
}}
