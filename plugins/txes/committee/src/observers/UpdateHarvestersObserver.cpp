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
		class LogLevelSetter {
		public:
			explicit LogLevelSetter(std::shared_ptr<chain::WeightedVotingCommitteeManager> pCommitteeManager)
				: m_pCommitteeManager(std::move(pCommitteeManager)) {
				m_pCommitteeManager->setLogLevel(utils::LogLevel::Trace);
			}
			~LogLevelSetter() {
				m_pCommitteeManager->setLogLevel(utils::LogLevel::Debug);
			}

		private:
			std::shared_ptr<chain::WeightedVotingCommitteeManager> m_pCommitteeManager;
		};

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

			LogLevelSetter logLevelSetter(pCommitteeManager);
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
				bool canHarvest = (effectiveBalance.unwrap() >= context.Config.Network.MinHarvesterBalance.unwrap());

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

	DECLARE_OBSERVER(UpdateHarvesters, Notification)(const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager) {
		return MAKE_OBSERVER(UpdateHarvesters, Notification, ([pCommitteeManager](const auto& notification, auto& context) {
			UpdateHarvesters(notification, context, pCommitteeManager);
		}));
	}
}}
