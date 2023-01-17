/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "DbrbProcess.h"
#include "Messages.h"
#include "catapult/crypto/Signer.h"


namespace catapult { namespace dbrb {

	/// Struct that stores information about view history. Validity is not guaranteed.
	struct ViewHistory {
		/// Sequence of views, starting with the view that follows the initial view.
		dbrb::Sequence Sequence;

		/// List of Install messages data that link adjacent views from Sequence.
		std::vector<InstallMessage> InstallMessages;

		// TODO: Can be optimized by replacing InstallMessages with these fields:
//		/// Lengths of subsequences of Sequence that make up respective Converged messages.
//		std::vector<uint8_t> SequenceLengths;
//
//		/// Map of processes and their signatures for appropriate Converged messages for Sequence.
//		std::map<ProcessId, catapult::Signature> ConvergedSignatures;

		/// Check whether given \a viewHistory is a valid view history, given an \a initialView.
		static bool isValidViewHistory(const ViewHistory& viewHistory, const View& initialView) {
			auto& views = viewHistory.Sequence.data();
			const auto& installMessages = viewHistory.InstallMessages;

			const auto& viewsCount = views.size();
			const auto& installMessagesCount = installMessages.size();

			if (viewsCount != installMessagesCount)
				return false;

			// Forming a complete sequence of views, including the initial view.
			std::vector<View> completeViews{ initialView };
			completeViews.reserve(1u + viewsCount);
			std::move(views.begin(), views.end(), std::back_inserter(completeViews));

			// Iterating over all pairs of corresponding views and install messages.
			for (auto i = 0u; i < viewsCount; ++i) {
				const auto& view = completeViews.at(i);
				const auto& installMessage = installMessages.at(i);

				// Install message must be properly formed.
				const auto pInstallMessageData = installMessage.tryGetMessageData();
				if (!pInstallMessageData.has_value())
					return false;

				// ReplacedView must be equal to the current view.
				if (pInstallMessageData->ReplacedView != view)
					return false;

				// ConvergedSignatures must contain a quorum of signatures with respect to the current view.
				if (installMessage.ConvergedSignatures.size() < view.quorumSize())
					return false;

				// Validating signatures.
				const auto& convergedSequence = pInstallMessageData->ConvergedSequence;
				const auto& replacedView = view;
				ConvergedMessage convergedMessage(ProcessId(), convergedSequence, replacedView);
				for (const auto& [processId, signature] : installMessage.ConvergedSignatures) {
					convergedMessage.Sender = processId;

					// Signature doesn't matter here, all we need is buffers() to generate a hash.
					const auto pConvergedPacket = convergedMessage.toNetworkPacket(nullptr);
					const auto hash = CalculateHash(pConvergedPacket->buffers());

					const bool isValid = crypto::Verify(processId, hash, signature);
					if (!isValid)
						return false;
				}
			}

			return true;
		}
	};
}}