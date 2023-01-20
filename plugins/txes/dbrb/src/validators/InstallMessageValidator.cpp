/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/crypto/Signer.h"
#include "extensions/fastfinality/src/dbrb/Messages.h"
#include "src/cache/ViewSequenceCache.h"


namespace catapult { namespace validators {

	using Notification = model::InstallMessageNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(InstallMessage, ([](const Notification& notification, const ValidatorContext& context) {
		const auto& viewSequenceCache = context.Cache.sub<cache::ViewSequenceCache>();

		// Check if respective view sequence exists
		if (viewSequenceCache.contains(notification.MessageHash))
			return Failure_Dbrb_View_Sequence_Already_Exists;

	  	// Check if there are at least two views in a sequence
	  	if (notification.ViewsCount < 2)
		  	return Failure_Dbrb_View_Sequence_Size_Insufficient;


		// Forming the replaced view and the converged sequence
	  	dbrb::View replacedView;
	  	dbrb::Sequence convergedSequence;
	  	{
			auto pProcessId = notification.ViewProcessIdsPtr;
			auto pMembershipChange = notification.MembershipChangesPtr;
			auto pViewSize = notification.ViewSizesPtr;

			dbrb::View view;
			auto& viewData = view.Data;
			for (auto i = 0u; i < notification.MostRecentViewSize; ++i, ++pProcessId, ++pMembershipChange) {
				dbrb::ProcessId processId = *pProcessId;
				const auto membershipChange = static_cast<const dbrb::MembershipChange>(*pMembershipChange);
				viewData.emplace(processId, membershipChange);

				if (viewData.size() >= *pViewSize) {
					if (replacedView.Data.empty()) {
						// Forming the replaced view
						replacedView = view;
					} else {
						// Forming the converged sequence
						convergedSequence.tryAppend(view);
					}
					++pViewSize;
				}
			}
		}

		// Check if the replaced view is the most recent view stored in the cache
		if (viewSequenceCache.getLatestView() != replacedView)
			return Failure_Dbrb_Invalid_Replaced_View;

		// Check if there are enough signatures
		if (notification.SignaturesCount < convergedSequence.maybeMostRecent()->quorumSize())
			return Failure_Dbrb_Signatures_Count_Insufficient;

	  	// Check if all signatures are valid
	  	dbrb::ConvergedMessage convergedMessage(dbrb::ProcessId(), convergedSequence, replacedView);
	  	{
			auto pProcessId = notification.SignaturesProcessIdsPtr;
			auto pSignature = notification.SignaturesPtr;

			for (auto i = 0u; i < notification.SignaturesCount; ++i, ++pProcessId, ++pSignature) {
				convergedMessage.Sender = *pProcessId;

				// Packet signature doesn't matter here, all we need is buffers() to generate a hash
				const auto pConvergedPacket = convergedMessage.toNetworkPacket(nullptr);
				const auto hash = dbrb::CalculateHash(pConvergedPacket->buffers());

				if (!crypto::Verify(*pProcessId, hash, *pSignature))
					return Failure_Dbrb_Invalid_Signature;
			}
	  	}

		return ValidationResult::Success;
	}));
}}
