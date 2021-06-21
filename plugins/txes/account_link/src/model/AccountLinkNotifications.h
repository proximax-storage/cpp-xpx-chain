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

#pragma once
#include "AccountLinkAction.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region account link notification types

/// Defines an account link notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_ACCOUNT_LINK_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, AccountLink, DESCRIPTION, CODE)

	/// Remote account was un/linked.
	DEFINE_ACCOUNT_LINK_NOTIFICATION(Remote_v1, 0x001, All);

	/// New remote account was created.
	DEFINE_ACCOUNT_LINK_NOTIFICATION(New_Remote_Account_v1, 0x002, Validator);

	/// Account was un/linked to a node.
	DEFINE_ACCOUNT_LINK_NOTIFICATION(Node, 0x003, All);

	/// Key link action was received.
	DEFINE_ACCOUNT_LINK_NOTIFICATION(Key_Link_Action, 0x004, Validator);

#undef DEFINE_ACCOUNTLINK_NOTIFICATION

	// endregion


	/// Notification of a remote account link.
	template<VersionType version>
	struct RemoteAccountLinkNotification;

	template<>
	struct RemoteAccountLinkNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = AccountLink_Remote_v1_Notification;

	public:
		/// Creates a notification around \a mainAccountKey, \a remoteAccountKey and \a linkAction.
		RemoteAccountLinkNotification(const Key& mainAccountKey, const Key& remoteAccountKey, AccountLinkAction linkAction)
				: Notification(Notification_Type, sizeof(RemoteAccountLinkNotification<1>))
				, MainAccountKey(mainAccountKey)
				, RemoteAccountKey(remoteAccountKey)
				, LinkAction(linkAction)
		{}

	public:
		/// Main account key.
		const Key& MainAccountKey;

		/// Remote account key.
		const Key& RemoteAccountKey;

		/// Account link action.
		AccountLinkAction LinkAction;
	};

	/// Notification of a new remote account.
	template<VersionType version>
	struct NewRemoteAccountNotification;

	template<>
	struct NewRemoteAccountNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = AccountLink_New_Remote_Account_v1_Notification;

	public:
		/// Creates a notification around \a remoteAccountKey.
		explicit NewRemoteAccountNotification(const Key& remoteAccountKey)
				: Notification(Notification_Type, sizeof(NewRemoteAccountNotification<1>))
				, RemoteAccountKey(remoteAccountKey)
		{}

	public:
		/// Remote account key.
		const Key& RemoteAccountKey;
	};

	// region NodeKeyLinkNotification
	/// Notification of a node link.
	template<VersionType version>
	struct NodeAccountLinkNotification;

	template<>
	struct NodeAccountLinkNotification<1> : public Notification {
		public:
			/// Matching notification type.
			static constexpr auto Notification_Type = AccountLink_Node_Notification;

		public:
			/// Creates a notification around \a mainAccountKey, \a remoteAccountKey and \a linkAction.
			NodeAccountLinkNotification(const Key& mainAccountKey, const Key& remoteAccountKey, AccountLinkAction linkAction)
					: Notification(Notification_Type, sizeof(RemoteAccountLinkNotification<1>))
					, MainAccountKey(mainAccountKey)
					, RemoteAccountKey(remoteAccountKey)
					, LinkAction(linkAction)
			{}

		public:
			/// Main account key.
			const Key& MainAccountKey;

			/// Remote account key.
			const Key& RemoteAccountKey;

			/// Account link action.
			AccountLinkAction LinkAction;
		};
	// endregion

	// region KeyLinkAction
	/// Notification of an account link action.
	template<VersionType version>
	struct KeyLinkActionNotification;
	/// Notification of a key link action.
	template<>
	struct KeyLinkActionNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = AccountLink_Key_Link_Action_Notification;

	public:
		/// Creates a notification around \a linkAction.
		explicit KeyLinkActionNotification(AccountLinkAction linkAction)
				: Notification(Notification_Type, sizeof(KeyLinkActionNotification))
				, LinkAction(linkAction)
		{}

	public:
		/// Link action.
		AccountLinkAction LinkAction;
	};
	// endregion

}}
