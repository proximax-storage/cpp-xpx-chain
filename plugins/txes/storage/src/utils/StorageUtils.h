/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SwapOperation.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/NotificationSubscriber.h"

namespace catapult { namespace utils {

	void SwapMosaics(
			const Key& account,
			const std::vector<model::UnresolvedMosaic>& mosaics,
			model::NotificationSubscriber& sub,
			const config::ImmutableConfiguration& immutableCfg,
			SwapOperation operation) {
		auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(immutableCfg);
		for (auto& mosaic : mosaics) {
			switch (operation) {
			case SwapOperation::Buy:
				sub.notify(model::BalanceDebitNotification<1>(account, currencyMosaicId, mosaic.Amount));
				sub.notify(model::BalanceCreditNotification<1>(account, mosaic.MosaicId, mosaic.Amount));
				break;
			case SwapOperation::Sell:
				sub.notify(model::BalanceDebitNotification<1>(account, mosaic.MosaicId, mosaic.Amount));
				sub.notify(model::BalanceCreditNotification<1>(account, currencyMosaicId, mosaic.Amount));
				break;
			default:
				CATAPULT_THROW_INVALID_ARGUMENT_1("unsupported operation", operation);
			}
		}
	}
}}
