/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/Notifications.h"
#include <utility>

namespace catapult::model {

	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, Opinion_Signature_v1, 0x0004);

	struct Opinion {
		Key PublicKey;
		Signature Sign;
		std::vector<uint8_t> Data;

		Opinion(const Key& publicKey, const Signature& sign, std::vector<uint8_t>&& data)
			: PublicKey(publicKey), Sign(sign), Data(std::move(data)) {}
	};

	template<VersionType version>
	struct OpinionSignatureNotification;

	template<>
	struct OpinionSignatureNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_Opinion_Signature_v1_Notification;

	public:
		explicit OpinionSignatureNotification(
				const std::vector<uint8_t>& commonData,
				const std::vector<Opinion>& opinions)
			: Notification(Notification_Type, sizeof(OpinionSignatureNotification<1>))
			, CommonData(commonData)
			, Opinions(opinions) {}

	public:
		std::vector<uint8_t> CommonData;
		std::vector<Opinion> Opinions;
	};
}