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

#include "AccountLinkTransactionPlugin.h"
#include "src/model/AccountLinkNotifications.h"
#include "src/model/AccountLinkTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const PublishContext& context, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 2:
				if (AccountLinkAction::Link == transaction.LinkAction) {
					// NewRemoteAccountNotification must be raised before AccountPublicKeyNotification because the latter adds account to cache
					sub.notify(NewRemoteAccountNotification<1>(transaction.RemoteAccountKey));
					sub.notify(AccountPublicKeyNotification<1>(transaction.RemoteAccountKey));
				}
				sub.notify(KeyLinkActionNotification<1>(transaction.LinkAction));
				sub.notify(AddressInteractionNotification<1>(transaction.Signer, transaction.Type, {}, { transaction.RemoteAccountKey }));
				sub.notify(RemoteAccountLinkNotification<1>(transaction.Signer, transaction.RemoteAccountKey, transaction.LinkAction));
				break;

			default:
				CATAPULT_LOG(debug) << "invalid version of AccountLinkTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(AccountLink, Default, Publish)
}}
