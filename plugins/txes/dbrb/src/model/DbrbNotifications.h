/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"
#include "extensions/fastfinality/src/dbrb/DbrbUtils.h"

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
			explicit InstallMessageNotification(const Hash256& messageHash, const uint8_t* pPayload)
				: Notification(Notification_Type, sizeof(InstallMessageNotification<1>))
				, MessageHash(messageHash)
				, Sequence(dbrb::Read<dbrb::Sequence>(pPayload))
				, Certificate(dbrb::Read<dbrb::CertificateType>(pPayload)) {}

		public:
			/// Install message hash.
			Hash256 MessageHash;

			/// Converged sequence.
			dbrb::Sequence Sequence;

						/// Install message certificate.
			dbrb::CertificateType Certificate;
		};
}}