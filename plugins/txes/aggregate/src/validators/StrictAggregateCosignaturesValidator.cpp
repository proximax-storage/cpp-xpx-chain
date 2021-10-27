/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateCosignaturesNotification<1>;

	auto returnValidResult(const Notification& notification, const ValidatorContext& context) {
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::AggregateConfiguration>();
		if (!pluginConfig.EnableStrictCosignatureCheck)
			return ValidationResult::Success;

		// collect all cosigners (initially set used flag to false)
		utils::ArrayPointerFlagMap<Key> cosigners;
		cosigners.emplace(&notification.Signer, false);
		const auto* pCosignature = notification.CosignaturesPtr;
		for (auto i = 0u; i < notification.CosignaturesCount; ++i) {
			cosigners.emplace(&pCosignature->Signer, false);
			++pCosignature;
		}

		// check all transaction signers and mark cosigners as used
		// notice that ineligible cosigners must dominate missing cosigners in order for cosigner aggregation to work
		auto hasMissingCosigners = false;
		const auto* pTransaction = notification.TransactionsPtr;
		for (auto i = 0u; i < notification.TransactionsCount; ++i) {
			auto iter = cosigners.find(&pTransaction->Signer);
			if (cosigners.cend() == iter)
				hasMissingCosigners = true;
			else
				iter->second = true;

			pTransaction = model::AdvanceNext(pTransaction);
		}

		// only return success if all cosigners are used
		return std::all_of(cosigners.cbegin(), cosigners.cend(), [&signerKey = notification.Signer, strictlyRequireSignerAsCosigner = pluginConfig.StrictSigner](const auto& pair) { return pair.second || *pair.first == signerKey && !strictlyRequireSignerAsCosigner; })
			   ? hasMissingCosigners ? Failure_Aggregate_Missing_Cosigners : ValidationResult::Success
			   : Failure_Aggregate_Ineligible_Cosigners;
	}
	DECLARE_STATEFUL_VALIDATOR(StrictAggregateCosignatures, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(StrictAggregateCosignatures, returnValidResult);
	}
}}
