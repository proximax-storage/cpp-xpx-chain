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

	using Notification = model::BlockCosignaturesNotification<1>;

	namespace {
		void UpdateHarvesters(
				const Notification& notification,
				ObserverContext& context,
				const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (UpdateHarvesters)");

			auto readOnlyCache = context.Cache.toReadOnly();
			cache::ImportanceView importanceView(readOnlyCache.sub<cache::AccountStateCache>());
			auto& committeeCache = context.Cache.sub<cache::CommitteeCache>();

			model::PublicKeySet cosignersSigned;
			auto cosignersNotSigned = pCommitteeManager->committee().Cosigners;
			const auto* pCosignature = notification.CosignaturesPtr;
			for (auto i = 0u; i < notification.NumCosignatures; ++i, ++pCosignature) {
				cosignersSigned.insert(pCosignature->Signer);
				cosignersNotSigned.erase(pCosignature->Signer);
			}

			const auto& config = context.Config.Network.GetPluginConfiguration<config::CommitteeConfiguration>();
			for (const auto& pair : pCommitteeManager->accountCollector()->accounts()) {
				const auto& key = pair.first;
				auto effectiveBalance = importanceView.getAccountImportanceOrDefault(key, context.Height);
				bool canHarvest = (effectiveBalance.unwrap() >= context.Config.Network.MinHarvesterBalance.unwrap());

				auto iter = committeeCache.find(key);
				auto& entry = iter.get();
				entry.setEffectiveBalance(effectiveBalance);
				entry.setCanHarvest(canHarvest);

				auto activity = entry.activity();

				if (cosignersSigned.find(key) != cosignersSigned.end()) {
					activity += config.ActivityCommitteeCosignedDelta;
				} else if (cosignersNotSigned.find(key) != cosignersNotSigned.end()) {
					activity -= config.ActivityCommitteeNotCosignedDelta;
				} else if (key == notification.Signer) {
					entry.setLastSigningBlockHeight(context.Height);
					entry.setGreed(static_cast<double>(notification.FeeInterest) / notification.FeeInterestDenominator);
					activity += config.ActivityCommitteeCosignedDelta;
				}

				activity -= config.ActivityDelta * boost::math::sign(activity);
				entry.setActivity(activity);
			}

			pCommitteeManager->reset();
		}
	}

	DECLARE_OBSERVER(UpdateHarvesters, Notification)(const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager) {
		return MAKE_OBSERVER(UpdateHarvesters, Notification, ([pCommitteeManager](const auto& notification, auto& context) {
			UpdateHarvesters(notification, context, pCommitteeManager);
		}));
	}
}}
