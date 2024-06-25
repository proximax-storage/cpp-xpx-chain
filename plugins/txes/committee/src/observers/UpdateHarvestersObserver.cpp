/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CommitteeCache.h"
#include "src/chain/WeightedVotingCommitteeManager.h"
#include "src/chain/WeightedVotingCommitteeManagerV2.h"
#include "src/chain/WeightedVotingCommitteeManagerV3.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/ImportanceView.h"
#include <boost/math/special_functions/sign.hpp>

namespace catapult { namespace observers {

	namespace {
		void UpdateHarvestersV1(
				const model::BlockCommitteeNotification<1>& notification,
				ObserverContext& context,
				const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager,
				const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector) {
			auto& committeeCache = context.Cache.sub<cache::CommitteeCache>();
			const auto& networkConfig = context.Config.Network;
			auto maxRollbackBlocks = networkConfig.MaxRollbackBlocks;
			if (NotifyMode::Commit == context.Mode && context.Height.unwrap() > maxRollbackBlocks) {
				auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
				auto disabledAccounts = pAccountCollector->disabledAccounts();
				auto disabledAccountsIter = disabledAccounts.find(pruneHeight);
				if (disabledAccounts.end() != disabledAccountsIter) {
					for (const auto& key : disabledAccountsIter->second)
						committeeCache.remove(key);
				}
			}

			if (!networkConfig.EnableWeightedVoting && !networkConfig.EnableDbrbFastFinality)
				return;

			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (UpdateHarvesters)");

			if (Height(1) == context.Height)
				return;

			auto readOnlyCache = context.Cache.toReadOnly();
			cache::ImportanceView importanceView(readOnlyCache.sub<cache::AccountStateCache>());

			pCommitteeManager->reset();
			while (pCommitteeManager->committee().Round < notification.Round)
				pCommitteeManager->selectCommittee(networkConfig, BlockchainVersion(0));
			CATAPULT_LOG(debug) << "block " << context.Height << ": selected committee for round " << notification.Round;
			pCommitteeManager->logCommittee();

			const auto& pluginConfig = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
			auto committee = pCommitteeManager->committee();
			auto& accounts = pCommitteeManager->accounts();
			accounts.at(committee.BlockProposer).ActivityObsolete += pluginConfig.ActivityCommitteeCosignedDelta;
			for (const auto& key : committee.Cosigners)
				accounts.at(key).ActivityObsolete += pluginConfig.ActivityCommitteeCosignedDelta;

			auto iter = committeeCache.find(committee.BlockProposer);
			auto& entry = iter.get();
			entry.setLastSigningBlockHeight(context.Height);
			entry.setGreedObsolete(static_cast<double>(notification.FeeInterest) / notification.FeeInterestDenominator);

			for (const auto& pair : accounts) {
				const auto& key = pair.first;
				auto effectiveBalance = importanceView.getAccountImportanceOrDefault(key, context.Height);
				bool canHarvest = (effectiveBalance.unwrap() >= networkConfig.MinHarvesterBalance.unwrap());

				auto iter = committeeCache.find(key);
				auto& entry = iter.get();
				entry.setEffectiveBalance(effectiveBalance);
				entry.setCanHarvest(canHarvest);

				auto sign = boost::math::sign(pair.second.ActivityObsolete);
				if (!sign)
					sign = 1;
				entry.setActivityObsolete(pair.second.ActivityObsolete - pluginConfig.ActivityDelta * sign);
			}
		}

		template<typename TNotification, typename TWeightedVotingCommitteeManager>
		void UpdateHarvestersV2(
				const TNotification& notification,
				ObserverContext& context,
				const std::shared_ptr<TWeightedVotingCommitteeManager>& pCommitteeManager,
				const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector,
				bool exitOnInvalidCommitteeRound) {
			auto& committeeCache = context.Cache.sub<cache::CommitteeCache>();
			const auto& networkConfig = context.Config.Network;
			auto maxRollbackBlocks = networkConfig.MaxRollbackBlocks;
			if (NotifyMode::Commit == context.Mode && context.Height.unwrap() > maxRollbackBlocks) {
				auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
				auto disabledAccounts = pAccountCollector->disabledAccounts();
				auto disabledAccountsIter = disabledAccounts.find(pruneHeight);
				if (disabledAccounts.end() != disabledAccountsIter) {
					for (const auto& key : disabledAccountsIter->second)
						committeeCache.remove(key);
				}
			}

			if (!networkConfig.EnableWeightedVoting && !networkConfig.EnableDbrbFastFinality)
				return;

			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (UpdateHarvesters)");

			if (Height(1) == context.Height)
				return;

			auto committee = pCommitteeManager->committee();
			if (committee.Round != notification.Round) {
				CATAPULT_LOG(error) << "invalid committee round " << committee.Round << " (expected " << notification.Round << ")";
				if (exitOnInvalidCommitteeRound)
					return;
			}
			CATAPULT_LOG(debug) << "block " << context.Height << ": committee round " << notification.Round;

			const auto& pluginConfig = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
			auto accounts = pCommitteeManager->accounts();

			{
				auto iter = accounts.find(committee.BlockProposer);
				if (iter == accounts.end())
					CATAPULT_THROW_RUNTIME_ERROR_1("block proposer not found", committee.BlockProposer)
				if (!pluginConfig.EnableEqualWeights) {
					iter->second.increaseActivity(pluginConfig.ActivityCommitteeCosignedDeltaInt);
					for (const auto& key : committee.Cosigners) {
						iter = accounts.find(key);
						if (iter == accounts.end())
							CATAPULT_THROW_RUNTIME_ERROR_1("committee member not found", key)
						iter->second.increaseActivity(pluginConfig.ActivityCommitteeCosignedDeltaInt);
					}
				}
			}

			{
				auto iter = committeeCache.find(committee.BlockProposer);
				auto& entry = iter.get();
				entry.setLastSigningBlockHeight(context.Height);
				entry.setFeeInterest(notification.FeeInterest);
				entry.setFeeInterestDenominator(notification.FeeInterestDenominator);
			}

			auto readOnlyCache = context.Cache.toReadOnly();
			cache::ImportanceView importanceView(readOnlyCache.sub<cache::AccountStateCache>());
			for (auto& pair : accounts) {
				const auto& key = pair.first;
				auto& data = pair.second;
				auto effectiveBalance = importanceView.getAccountImportanceOrDefault(key, context.Height);
				bool canHarvest = (effectiveBalance.unwrap() >= networkConfig.MinHarvesterBalance.unwrap());

				auto iter = committeeCache.find(key);
				auto& entry = iter.get();
				if (pluginConfig.EnableBlockchainVersionValidation) {
					entry.setVersion(5);
				} else if (!networkConfig.BootstrapHarvesters.empty()) {
					entry.setVersion(4);
				} else {
					entry.setVersion(3);
				}
				entry.setEffectiveBalance(effectiveBalance);
				entry.setCanHarvest(canHarvest);
				if (!entry.feeInterestDenominator()) {
					entry.setFeeInterest(pluginConfig.MinGreedFeeInterest);
					entry.setFeeInterestDenominator(pluginConfig.MinGreedFeeInterestDenominator);
				}

				if (!pluginConfig.EnableEqualWeights) {
					auto sign = boost::math::sign(data.Activity);
					if (!sign)
						sign = 1;
					data.decreaseActivity(pluginConfig.ActivityDeltaInt * sign);
					entry.setActivity(data.Activity);
				}
			}
		}
	}

	DECLARE_OBSERVER(UpdateHarvestersV1, model::BlockCommitteeNotification<1>)(
			const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager,
			const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector) {
		return MAKE_OBSERVER(UpdateHarvestersV1, model::BlockCommitteeNotification<1>, ([pCommitteeManager, pAccountCollector](const auto& notification, auto& context) {
			UpdateHarvestersV1(notification, context, pCommitteeManager, pAccountCollector);
		}));
	}

	DECLARE_OBSERVER(UpdateHarvestersV2, model::BlockCommitteeNotification<2>)(
			const std::shared_ptr<chain::WeightedVotingCommitteeManagerV2>& pCommitteeManager,
			const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector) {
		return MAKE_OBSERVER(UpdateHarvestersV2, model::BlockCommitteeNotification<2>, ([pCommitteeManager, pAccountCollector](const auto& notification, auto& context) {
			UpdateHarvestersV2(notification, context, pCommitteeManager, pAccountCollector, true);
		}));
	}

	DECLARE_OBSERVER(UpdateHarvestersV3, model::BlockCommitteeNotification<3>)(
			const std::shared_ptr<chain::WeightedVotingCommitteeManagerV3>& pCommitteeManager,
			const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector) {
		return MAKE_OBSERVER(UpdateHarvestersV3, model::BlockCommitteeNotification<3>, ([pCommitteeManager, pAccountCollector](const auto& notification, auto& context) {
			UpdateHarvestersV2(notification, context, pCommitteeManager, pAccountCollector, false);
		}));
	}

	DECLARE_OBSERVER(UpdateHarvestersV4, model::BlockCommitteeNotification<4>)(
			const std::shared_ptr<chain::WeightedVotingCommitteeManagerV3>& pCommitteeManager,
			const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector) {
		return MAKE_OBSERVER(UpdateHarvestersV4, model::BlockCommitteeNotification<4>, ([pCommitteeManager, pAccountCollector](const auto& notification, auto& context) {
			auto& committeeCache = context.Cache.template sub<cache::CommitteeCache>();
			const auto& networkConfig = context.Config.Network;
			auto maxRollbackBlocks = networkConfig.MaxRollbackBlocks;
			if (NotifyMode::Commit == context.Mode && context.Height.unwrap() > maxRollbackBlocks) {
				auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
				auto disabledAccounts = pAccountCollector->disabledAccounts();
				auto disabledAccountsIter = disabledAccounts.find(pruneHeight);
				if (disabledAccounts.end() != disabledAccountsIter) {
					for (const auto& key : disabledAccountsIter->second)
						committeeCache.remove(key);
				}
			}

			if (!networkConfig.EnableWeightedVoting && !networkConfig.EnableDbrbFastFinality)
				return;

			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (UpdateHarvesters)");

			if (Height(1) == context.Height)
				return;

			auto committee = pCommitteeManager->committee();
			CATAPULT_LOG(debug) << "block " << context.Height << ": committee round " << notification.Round;
			auto accounts = pCommitteeManager->accounts();
			auto blockProposerIter = committeeCache.find(committee.BlockProposer);
			auto& entry = blockProposerIter.get();
			entry.setLastSigningBlockHeight(context.Height);
			entry.setFeeInterest(notification.FeeInterest);
			entry.setFeeInterestDenominator(notification.FeeInterestDenominator);

			auto readOnlyCache = context.Cache.toReadOnly();
			cache::ImportanceView importanceView(readOnlyCache.template sub<cache::AccountStateCache>());
			const auto& pluginConfig = networkConfig.template GetPluginConfiguration<config::CommitteeConfiguration>();
			for (auto& [key, data] : accounts) {
				auto iter = committeeCache.find(key);
				auto& entry = iter.get();
				entry.setVersion(6);
				entry.setBanPeriod(data.BanPeriod);
				entry.decrementBanPeriod();
				auto effectiveBalance = importanceView.getAccountImportanceOrDefault(key, context.Height);
				entry.setEffectiveBalance(effectiveBalance);
				entry.setCanHarvest((effectiveBalance.unwrap() >= networkConfig.MinHarvesterBalance.unwrap()));
				if (!entry.feeInterestDenominator()) {
					entry.setFeeInterest(pluginConfig.MinGreedFeeInterest);
					entry.setFeeInterestDenominator(pluginConfig.MinGreedFeeInterestDenominator);
				}
			}
		}));
	}
}}
