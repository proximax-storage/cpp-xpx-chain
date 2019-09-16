/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::TransferMosaicsNotification<1>;

	DEFINE_STATELESS_VALIDATOR(TransferMosaics, [](const auto& notification) {
			if (1 > notification.MosaicsCount)
				return ValidationResult::Success;

			if (1 < notification.MosaicsCount)
				return Failure_Service_Multiple_Mosaics;

			if (1 > notification.MosaicsPtr[0].Amount.unwrap())
				return Failure_Service_Zero_Amount;

			return ValidationResult::Success;
		});
	}
}
