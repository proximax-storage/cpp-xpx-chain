/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CommitteeCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"

namespace catapult { namespace observers {

	namespace {
		void ActiveHarvestersV1(const model::ActiveHarvestersNotification<1>& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ActiveHarvesters)");

			auto& cache = context.Cache.sub<cache::CommitteeCache>();
			auto pHarvesterKey = notification.HarvesterKeysPtr;
			for (auto i = 0u; i < notification.HarvesterKeysCount; ++i, ++pHarvesterKey) {
				auto iter = cache.find(*pHarvesterKey);
				auto pEntry = iter.tryGet();
				if (pEntry) {
					const auto& config = context.Config.Network;
					pEntry->setExpirationTime(context.Timestamp + Timestamp(config.DbrbRegistrationDuration.millis() + config.DbrbRegistrationGracePeriod.millis()));
					pEntry->setVersion(2);
				}
			}
		}

		void ActiveHarvestersV2(const model::ActiveHarvestersNotification<2>& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ActiveHarvesters)");

			auto& cache = context.Cache.sub<cache::CommitteeCache>();
			auto pHarvesterKey = notification.HarvesterKeysPtr;
			for (auto i = 0u; i < notification.HarvesterKeysCount; ++i, ++pHarvesterKey) {
				auto iter = cache.find(*pHarvesterKey);
				auto pEntry = iter.tryGet();
				if (pEntry) {
					const auto& config = context.Config.Network;
					pEntry->setBootKey(notification.BootKey);
					pEntry->setExpirationTime(context.Timestamp + Timestamp(config.DbrbRegistrationDuration.millis() + config.DbrbRegistrationGracePeriod.millis()));
					pEntry->setVersion(4);
				}
			}
		}

		void ActiveHarvestersV3(const model::ActiveHarvestersNotification<3>& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ActiveHarvesters)");

			auto& cache = context.Cache.sub<cache::CommitteeCache>();
			auto pHarvesterKey = notification.HarvesterKeysPtr;
			for (auto i = 0u; i < notification.HarvesterKeysCount; ++i, ++pHarvesterKey) {
				auto iter = cache.find(*pHarvesterKey);
				auto pEntry = iter.tryGet();
				if (pEntry) {
					const auto& config = context.Config.Network;
					pEntry->setBootKey(notification.BootKey);
					pEntry->setBlockchainVersion(notification.BlockchainVersion);
					pEntry->setExpirationTime(context.Timestamp + Timestamp(config.DbrbRegistrationDuration.millis() + config.DbrbRegistrationGracePeriod.millis()));
					pEntry->setVersion(5);
				}
			}
		}

		void ActiveHarvestersV4(const model::ActiveHarvestersNotification<4>& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ActiveHarvesters)");

			auto& cache = context.Cache.sub<cache::CommitteeCache>();
			auto pHarvesterKey = notification.HarvesterKeysPtr;
			for (auto i = 0u; i < notification.HarvesterKeysCount; ++i, ++pHarvesterKey) {
				auto iter = cache.find(*pHarvesterKey);
				auto pEntry = iter.tryGet();
				if (pEntry) {
					const auto& config = context.Config.Network;
					pEntry->setBootKey(notification.BootKey);
					pEntry->setBlockchainVersion(notification.BlockchainVersion);
					pEntry->setExpirationTime(context.Timestamp + Timestamp(config.DbrbRegistrationDuration.millis() + config.DbrbRegistrationGracePeriod.millis()));
					pEntry->setVersion(6);
				}
			}
		}
	}

	DEFINE_OBSERVER(ActiveHarvestersV1, model::ActiveHarvestersNotification<1>, ActiveHarvestersV1);
	DEFINE_OBSERVER(ActiveHarvestersV2, model::ActiveHarvestersNotification<2>, ActiveHarvestersV2);
	DEFINE_OBSERVER(ActiveHarvestersV3, model::ActiveHarvestersNotification<3>, ActiveHarvestersV3);
	DEFINE_OBSERVER(ActiveHarvestersV4, model::ActiveHarvestersNotification<4>, ActiveHarvestersV4);
}}
