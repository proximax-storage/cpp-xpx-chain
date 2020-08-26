/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/chain/WeightedVotingCommitteeManager.h"

namespace catapult { namespace validators {

	using Notification = model::BlockCosignaturesNotification<1>;

	DECLARE_STATELESS_VALIDATOR(Committee, Notification)(const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager) {
		return MAKE_STATELESS_VALIDATOR(Committee, [pCommitteeManager](const auto& notification) {
			auto committee = pCommitteeManager->committee();
			if (committee.BlockProposer != notification.Signer)
				return Failure_Committee_Invalid_Block_Signer;

			if (committee.Cosigners.size() < notification.NumCosignatures)
				return Failure_Committee_Invalid_Committee_Number;

			auto pCosignature = notification.CosignaturesPtr;
			for (auto i = 0u; i < notification.NumCosignatures; ++i, ++pCosignature) {
				if (committee.Cosigners.find(pCosignature->Signer) == committee.Cosigners.end())
					return Failure_Committee_Invalid_Committee;
			}

			return ValidationResult::Success;
		})
	}
}}
