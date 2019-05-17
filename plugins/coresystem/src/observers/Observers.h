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
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/model/Notifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

	// region VerifiableEntity

	/// Observes account address changes.
	DECLARE_OBSERVER(AccountAddress, model::AccountAddressNotification<1>)();

	/// Observes account public key changes.
	DECLARE_OBSERVER(AccountPublicKey, model::AccountPublicKeyNotification<1>)();

	// endregion

	// region Block

	/// Observes block notifications and credits the harvester with transaction fees given the currency mosaic id (\a currencyMosaicId).
	DECLARE_OBSERVER(HarvestFee, model::BlockNotification)(MosaicId currencyMosaicId);

	/// Observes block difficulties.
	DECLARE_OBSERVER(BlockDifficulty, model::BlockNotification)();

	/// Observes block notifications and counts transactions.
	DECLARE_OBSERVER(TotalTransactions, model::BlockNotification)();

	/// Observes block notifications and clean up snapshot from modified accounts.
	DECLARE_OBSERVER(SnapshotCleanUp, model::BlockNotification)(const model::BlockChainConfiguration& config);

	// endregion

	// region Transaction

	/// Observes balance changes triggered by balance transfer notifications.
	DECLARE_OBSERVER(BalanceTransfer, model::BalanceTransferNotification<1>)();

	/// Observes balance changes triggered by balance debit notifications.
	DECLARE_OBSERVER(BalanceDebit, model::BalanceDebitNotification<1>)();

	// endregion

	// region SourceChange

	/// Observes source changes and changes observer source.
	DECLARE_OBSERVER(SourceChange, model::SourceChangeNotification)();

	// endregion
}}
