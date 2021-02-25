/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CommitteeCache.h"
#include "src/chain/WeightedVotingCommitteeManager.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/ImportanceView.h"
#include <boost/math/special_functions/sign.hpp>

namespace catapult { namespace observers {

	using Notification = model::BlockCommitteeNotification<1>;

	namespace {
		void UpdateActivities(
				double activityDelta,
				cache::CommitteeCacheDelta& committeeCache,
				const model::NetworkConfiguration& networkConfig,
				const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager) {
			const auto& committee = pCommitteeManager->selectCommittee(networkConfig);
			for (const auto& key : committee.Cosigners) {
				auto iter = committeeCache.find(key);
				auto& entry = iter.get();
				entry.data().Activity += activityDelta;
			}
			auto iter = committeeCache.find(committee.BlockProposer);
			auto& entry = iter.get();
			entry.data().Activity += activityDelta;
		}

		void UpdateHarvesters(
				const Notification& notification,
				ObserverContext& context,
				const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager) {
			const auto& networkConfig = context.Config.Network;
			if (!networkConfig.EnableWeightedVoting)
				return;

			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (UpdateHarvesters)");

			if (Height(1) == context.Height)
				return;

			auto readOnlyCache = context.Cache.toReadOnly();
			cache::ImportanceView importanceView(readOnlyCache.sub<cache::AccountStateCache>());
			auto& committeeCache = context.Cache.sub<cache::CommitteeCache>();

			pCommitteeManager->reset();

			const auto& pluginConfig = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
			for (auto i = 0; i < notification.Round; ++i) {
				UpdateActivities(-pluginConfig.ActivityCommitteeNotCosignedDelta, committeeCache, networkConfig, pCommitteeManager);
			}
			UpdateActivities(pluginConfig.ActivityCommitteeCosignedDelta, committeeCache, networkConfig, pCommitteeManager);

			auto iter = committeeCache.find(pCommitteeManager->committee().BlockProposer);
			auto& entry = iter.get();
			entry.setLastSigningBlockHeight(context.Height);
			entry.setGreed(static_cast<double>(notification.FeeInterest) / notification.FeeInterestDenominator);

			for (const auto& pair : pCommitteeManager->accountCollector()->accounts()) {
				const auto& key = pair.first;
				auto effectiveBalance = importanceView.getAccountImportanceOrDefault(key, context.Height);
				bool canHarvest = (effectiveBalance.unwrap() >= context.Config.Network.MinHarvesterBalance.unwrap());

				auto iter = committeeCache.find(key);
				auto& entry = iter.get();
				entry.setEffectiveBalance(effectiveBalance);
				entry.setCanHarvest(canHarvest);
				entry.data().Activity -= pluginConfig.ActivityDelta * boost::math::sign(entry.data().Activity);
			}
		}
	}

	DECLARE_OBSERVER(UpdateHarvesters, Notification)(const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager) {
		return MAKE_OBSERVER(UpdateHarvesters, Notification, ([pCommitteeManager](const auto& notification, auto& context) {
			UpdateHarvesters(notification, context, pCommitteeManager);
		}));
	}
}}
