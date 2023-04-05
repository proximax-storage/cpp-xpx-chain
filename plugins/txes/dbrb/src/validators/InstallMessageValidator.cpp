/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/crypto/Signer.h"
#include "catapult/dbrb/Messages.h"
#include "src/cache/ViewSequenceCache.h"


namespace catapult { namespace validators {

	using Notification = model::InstallMessageNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(InstallMessage, ([](const Notification& notification, const ValidatorContext& context) {
		const auto& viewSequenceCache = context.Cache.sub<cache::ViewSequenceCache>();

		// Check if respective view sequence exists
		if (viewSequenceCache.contains(notification.MessageHash))
			return Failure_Dbrb_View_Sequence_Already_Exists;

	  	// Check if there are at least two views in a sequence
	  	if (notification.Sequence.data().size() < 2u)
		  	return Failure_Dbrb_View_Sequence_Size_Insufficient;

		auto replacedView = *notification.Sequence.maybeLeastRecent();
		std::vector<dbrb::View> convergedSequenceData(notification.Sequence.data().begin() + 1u, notification.Sequence.data().end());
		auto convergedSequence = *dbrb::Sequence::fromViews(convergedSequenceData);

		// Check if the replaced view is the most recent view stored in the cache
		if (viewSequenceCache.size() > 0 && viewSequenceCache.getLatestView() != replacedView)
			return Failure_Dbrb_Invalid_Replaced_View;

		// Check if there are enough signatures
		if (notification.Certificate.size() < replacedView.quorumSize())
			return Failure_Dbrb_Signatures_Count_Insufficient;

		return ValidationResult::Success;
	}));
}}
