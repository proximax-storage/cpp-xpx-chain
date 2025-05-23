/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::CreditMosaicNotification<1>;

	DEFINE_STATEFUL_VALIDATOR_WITH_LIQUIDITY_PROVIDER(CreditMosaic, [&liquidityProvider](const Notification& notification, const ValidatorContext& context) {
        return liquidityProvider->validateCreditMosaics(context, notification.CurrencyDebtor, notification.MosaicId, notification.MosaicAmount);
    })
}}
