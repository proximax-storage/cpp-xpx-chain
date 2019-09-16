/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::MosaicIdNotification<1>;

	DEFINE_STATELESS_VALIDATOR(MosaicId, [](const auto& notification) {
			if (notification.PaidMosaicId.unwrap() != notification.ValidMosaicId.unwrap())
				return Failure_Service_Invalid_Mosaic;

			return ValidationResult::Success;
		});
	}
}
