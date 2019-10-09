/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "catapult/model/Mosaic.h"
#include "catapult/model/Notifications.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

	// region transfer notification types

/// Defines a transfer notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
// Note "Hello" should be a member of catapult::model::FacilityCode
#define DEFINE_HELLO_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Hello, DESCRIPTION, CODE)

	/// Transfer was received with a message.
	DEFINE_HELLO_NOTIFICATION(MessageCount_v1, 0x001, Validator);

#undef DEFINE_HELLO_NOTIFICATION

	// endregion

	/// Notification of a transfer transaction with a message.
	template<VersionType version>
	struct HelloMessageCountNotification;

	template<>
	struct HelloMessageCountNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Hello_MessageCount_v1_Notification;

	public:
		/// Creates a notification around \a messageSize.
		explicit HelloMessageCountNotification(uint16_t messageCount, Key key)
				: Notification(Notification_Type, sizeof(HelloMessageCountNotification<1>))
				, MessageCount(messageCount)
				, SignerKey(key)
		{}

	public:
		/// Message size in bytes.
		uint16_t MessageCount;

		// transaction signer key
		Key SignerKey;
	};

}}
