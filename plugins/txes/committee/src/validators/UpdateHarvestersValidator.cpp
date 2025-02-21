/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/chain/WeightedVotingCommitteeManagerV3.h"

namespace catapult { namespace validators {

	using Notification = model::BlockCommitteeNotification<4>;

	DECLARE_STATEFUL_VALIDATOR(Committee, Notification)(const std::shared_ptr<chain::WeightedVotingCommitteeManagerV3>& pCommitteeManager) {
		return MAKE_STATEFUL_VALIDATOR(Committee, [pCommitteeManager](const auto& notification, const ValidatorContext& context) {
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::CommitteeConfiguration>();
			if (!pluginConfig.EnableBlockProducerValidation)
				return ValidationResult::Success;

			auto committee = pCommitteeManager->committee();
			if (committee.Round != notification.Round)
				return Failure_Committee_Invalid_Committee_Round;

			if (!committee.validateBlockProposer(notification.BlockSigner))
				return Failure_Committee_Invalid_Block_Signer;

			return ValidationResult::Success;
		});
	}
}}
