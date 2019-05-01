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

#include "SecretLockTransactionPlugin.h"
#include "src/model/SecretLockNotifications.h"
#include "src/model/SecretLockTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, NotificationSubscriber& sub) {
			sub.notify(AccountAddressNotification(transaction.Recipient, transaction.EntityVersion()));
			sub.notify(SecretLockDurationNotification(transaction.Duration, transaction.EntityVersion()));
			sub.notify(SecretLockHashAlgorithmNotification(transaction.HashAlgorithm, transaction.EntityVersion()));
			sub.notify(AddressInteractionNotification(transaction.Signer, transaction.Type, { transaction.Recipient }, transaction.EntityVersion()));
			sub.notify(BalanceDebitNotification(transaction.Signer, transaction.Mosaic.MosaicId, transaction.Mosaic.Amount, transaction.EntityVersion()));
			sub.notify(SecretLockNotification(
					transaction.Signer,
					transaction.Mosaic,
					transaction.Duration,
					transaction.HashAlgorithm,
					transaction.Secret,
					transaction.Recipient,
					transaction.EntityVersion()));
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(SecretLock, Publish)
}}
