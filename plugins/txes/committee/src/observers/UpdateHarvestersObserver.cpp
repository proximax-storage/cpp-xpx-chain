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
		void UpdateHarvesters(
				const Notification& notification,
				ObserverContext& context,
				const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager,
				const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector) {
			auto& committeeCache = context.Cache.sub<cache::CommitteeCache>();
			const auto& networkConfig = context.Config.Network;
			auto maxRollbackBlocks = networkConfig.MaxRollbackBlocks;
			if (NotifyMode::Commit == context.Mode && context.Height.unwrap() > maxRollbackBlocks) {
				auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
				auto& disabledAccounts = pAccountCollector->disabledAccounts();
				auto disabledAccountsIter = disabledAccounts.find(pruneHeight);
				if (disabledAccounts.end() != disabledAccountsIter) {
					for (const auto& key : disabledAccountsIter->second)
						committeeCache.remove(key);
				}
			}

			if (!networkConfig.EnableWeightedVoting)
				return;

			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (UpdateHarvesters)");

			if (Height(1) == context.Height)
				return;

			auto readOnlyCache = context.Cache.toReadOnly();
			cache::ImportanceView importanceView(readOnlyCache.sub<cache::AccountStateCache>());

			CATAPULT_LOG(debug) << "committee round " << pCommitteeManager->committee().Round << ", notification round " << notification.Round;
			pCommitteeManager->reset();
			while (pCommitteeManager->committee().Round < notification.Round)
				pCommitteeManager->selectCommittee(networkConfig);

			const auto& pluginConfig = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
			const auto& committee = pCommitteeManager->committee();
			auto& accounts = pCommitteeManager->accounts();
			accounts.at(committee.BlockProposer).Activity += pluginConfig.ActivityCommitteeCosignedDelta;
			for (const auto& key : committee.Cosigners) {
				accounts.at(key).Activity += pluginConfig.ActivityCommitteeCosignedDelta;
			}

			auto iter = committeeCache.find(committee.BlockProposer);
			auto& entry = iter.get();
			entry.setLastSigningBlockHeight(context.Height);
			entry.setGreed(static_cast<double>(notification.FeeInterest) / notification.FeeInterestDenominator);

			for (const auto& pair : accounts) {
				const auto& key = pair.first;
				auto effectiveBalance = importanceView.getAccountImportanceOrDefault(key, context.Height);
				bool canHarvest = (effectiveBalance.unwrap() >= networkConfig.MinHarvesterBalance.unwrap());

				auto iter = committeeCache.find(key);
				auto& entry = iter.get();
				entry.setEffectiveBalance(effectiveBalance);
				entry.setCanHarvest(canHarvest);

				auto sign = boost::math::sign(pair.second.Activity);
				if (!sign)
					sign = 1;
				entry.setActivity(pair.second.Activity - pluginConfig.ActivityDelta * sign);
			}
		}
	}

	DECLARE_OBSERVER(UpdateHarvesters, Notification)(
			const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager,
			const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector) {
		return MAKE_OBSERVER(UpdateHarvesters, Notification, ([pCommitteeManager, pAccountCollector](const auto& notification, auto& context) {
			UpdateHarvesters(notification, context, pCommitteeManager, pAccountCollector);
		}));
	}
}}
