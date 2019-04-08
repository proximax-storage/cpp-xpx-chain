/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "plugins/txes/metadata/src/model/MetadataNotifications.h"
#include "catapult/utils/UnresolvedAddress.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to metadata field modification and validates that
	DECLARE_STATELESS_VALIDATOR(MetadataFieldModification, model::ModifyMetadataFieldNotification)(uint8_t maxKeySize, uint16_t maxValueSize);
}}
