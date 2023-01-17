/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

		/// Defines an Install message notification type.
		DEFINE_NOTIFICATION_TYPE(All, Dbrb, Install_Message_v1, 0x0001);

		/// Notification of an Install message.
		template<VersionType version>
		struct InstallMessageNotification;

		template<>
		struct InstallMessageNotification<1> : public Notification {
		public:
			/// Matching notification type.
			static constexpr auto Notification_Type = Dbrb_Install_Message_v1_Notification;

		public:
			explicit InstallMessageNotification(
					const Hash256& messageHash,
					const uint32_t viewsCount,
					const uint32_t mostRecentViewSize,
					const uint32_t signaturesCount,
					const uint16_t* viewSizesPtr,
					const Key* viewProcessIdsPtr,
					const bool* membershipChangesPtr,
					const Key* signaturesProcessIdsPtr,
					const Signature* signaturesPtr)
				: Notification(Notification_Type, sizeof(InstallMessageNotification<1>))
				, MessageHash(messageHash)
				, ViewsCount(viewsCount)
				, MostRecentViewSize(mostRecentViewSize)
				, SignaturesCount(signaturesCount)
				, ViewSizesPtr(viewSizesPtr)
				, ViewProcessIdsPtr(viewProcessIdsPtr)
				, MembershipChangesPtr(membershipChangesPtr)
				, SignaturesProcessIdsPtr(signaturesProcessIdsPtr)
				, SignaturesPtr(signaturesPtr) {}

		public:
			/// Hash of the Install message.
			Hash256 MessageHash;

			/// Number of views that compose a sequence.
			uint32_t ViewsCount;

			/// Number of membership change pairs in the most recent (longest) view.
			uint32_t MostRecentViewSize;

			/// Number of pairs of process IDs and their signatures.
			uint32_t SignaturesCount;

			/// Numbers of membership change pairs in corresponding views.
			/// Has the length of ViewsCount.
			const uint16_t* ViewSizesPtr;

			/// Keys of the processes mentioned in the most recent (longest) view.
			/// Has the length of MostRecentViewSize.
			const Key* ViewProcessIdsPtr;

			/// Changes of process membership in the system. True = joined, false = left.
			/// Has the length of MostRecentViewSize.
			const bool* MembershipChangesPtr;

			/// Keys of the processes mentioned in the signatures map.
			/// Has the length of SignaturesCount.
			const Key* SignaturesProcessIdsPtr;

			/// Signatures of processes.
			/// Has the length of SignaturesCount.
			const Signature* SignaturesPtr;
		};
}}