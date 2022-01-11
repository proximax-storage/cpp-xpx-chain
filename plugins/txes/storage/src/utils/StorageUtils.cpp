/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StorageUtils.h"

namespace catapult { namespace utils {

	// TODO: Clean up repeating code

	void SwapMosaics(
			const Key& sender,
			const Key& receiver,
			const std::vector<model::UnresolvedMosaic>& mosaics,
			model::NotificationSubscriber& sub,
			const config::ImmutableConfiguration& immutableCfg,
			SwapOperation operation) {
		auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(immutableCfg);
		for (auto& mosaic : mosaics) {
			switch (operation) {
			case SwapOperation::Buy:
				sub.notify(model::BalanceDebitNotification<1>(sender, currencyMosaicId, mosaic.Amount));
				sub.notify(model::BalanceCreditNotification<1>(receiver, mosaic.MosaicId, mosaic.Amount));
				break;
			case SwapOperation::Sell:
				sub.notify(model::BalanceDebitNotification<1>(sender, mosaic.MosaicId, mosaic.Amount));
				sub.notify(model::BalanceCreditNotification<1>(receiver, currencyMosaicId, mosaic.Amount));
				break;
			default:
				CATAPULT_THROW_INVALID_ARGUMENT_1("unsupported operation", operation);
			}
		}
	}

	void SwapMosaics(
			const Key& sender,
			const Key& receiver,
			const std::vector<std::pair<UnresolvedMosaicId, UnresolvedAmount>>& mosaics,
			model::NotificationSubscriber& sub,
			const config::ImmutableConfiguration& immutableCfg,
			SwapOperation operation) {
		auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(immutableCfg);
		for (auto& mosaic : mosaics) {
			switch (operation) {
			case SwapOperation::Buy:
				sub.notify(model::BalanceDebitNotification<1>(sender, currencyMosaicId, mosaic.second));
				sub.notify(model::BalanceCreditNotification<1>(receiver, mosaic.first, mosaic.second));
				break;
			case SwapOperation::Sell:
				sub.notify(model::BalanceDebitNotification<1>(sender, mosaic.first, mosaic.second));
				sub.notify(model::BalanceCreditNotification<1>(receiver, currencyMosaicId, mosaic.second));
				break;
			default:
				CATAPULT_THROW_INVALID_ARGUMENT_1("unsupported operation", operation);
			}
		}
	}

	void SwapMosaics(
			const Key& account,
			const std::vector<model::UnresolvedMosaic>& mosaics,
			model::NotificationSubscriber& sub,
			const config::ImmutableConfiguration& immutableCfg,
			SwapOperation operation) {
		return SwapMosaics(account, account, mosaics, sub, immutableCfg, operation);
	}

	void SwapMosaics(
			const Key& account,
			const std::vector<std::pair<UnresolvedMosaicId, UnresolvedAmount>>& mosaics,
			model::NotificationSubscriber& sub,
			const config::ImmutableConfiguration& immutableCfg,
			SwapOperation operation) {
		return SwapMosaics(account, account, mosaics, sub, immutableCfg, operation);
	}

	double CalculateDrivePriority(const state::BcDriveEntry& driveEntry, const uint16_t& Rmin) {
		const auto& N = driveEntry.replicatorCount();
		const auto& R = driveEntry.replicators().size();	// TODO: - driveEntry.offboardingReplicators().size();

		return R < Rmin ? (R + 1)/Rmin : (N - R)/(2*Rmin*(N - Rmin));
	}
}}
