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

#include "HashLockTransactionPlugin.h"
#include "src/model/HashLockNotifications.h"
#include "src/model/HashLockTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, NotificationSubscriber& sub) {
			switch (transaction.Version) {
			case 1:
				sub.notify(HashLockDurationNotification<1>(transaction.Duration));
				sub.notify(HashLockMosaicNotification<1>(transaction.Mosaic));
				sub.notify(BalanceDebitNotification<1>(transaction.Signer, transaction.Mosaic.MosaicId, transaction.Mosaic.Amount));
				sub.notify(HashLockNotification<1>(transaction.Signer, transaction.Mosaic, transaction.Duration, transaction.Hash));
				break;

			default:
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of HashLockTransaction", transaction.Version);
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(HashLock, Publish)
}}
