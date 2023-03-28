/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include <catapult/crypto/Signer.h>

namespace catapult { namespace validators {

	using Notification = model::OpinionSignatureNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(OpinionSignature, [](const Notification& notification, const ValidatorContext& context) {

		for (const auto& opinion: notification.Opinions) {
			std::vector<uint8_t> dataToVerify(notification.CommonData);
			dataToVerify.insert(dataToVerify.end(), opinion.Data.begin(), opinion.Data.end());
			if (!crypto::Verify(opinion.PublicKey, dataToVerify, opinion.Sign)) {
				return Failure_SuperContract_v2_Invalid_Signature;
			};
		}

		return ValidationResult::Success;
	})

}}
