/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/TransactionPlugin.h"
#include "catapult/state/AccountState.h"
#include "catapult/types.h"
#include "AccountMigrationManager.h"
#include "catapult/state/ExchangeEntry.h"
#include "catapult/state/MetadataV1Entry.h"
#include "catapult/model/MetadataV1Types.h"
#include "catapult/state/MetadataEntry.h"
#include "catapult/state/MosaicEntry.h"
#include "catapult/state/LevyEntry.h"
#include "catapult/state/MultisigEntry.h"
#include "catapult/state/NamespaceEntry.h"
#include "catapult/state/RootNamespaceHistory.h"
#include "catapult/state/AccountProperties.h"
#include "catapult/model/PropertyTypes.h"
#include "catapult/state/AccountProperty.h"
#include "catapult/utils/ByteArray.h"
#include "catapult/state/BcDriveEntry.h"
#include "catapult/model/MetadataTypes.h"
#include "catapult/state/CommitteeEntry.h"
#include "catapult/state/NetworkConfigEntry.h"

namespace catapult { namespace tools { namespace nemgen {

	template<typename TValType>
	TValType Convert(utils::AccountMigrationManager& accountMigrationManager, const TValType& value) {
		return value;
	}
	state::NetworkConfigEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::NetworkConfigEntry& value, bool is_future);

	state::AccountState Convert(utils::AccountMigrationManager& accountMigrationManager, const state::AccountState& value);

	state::ExchangeEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::ExchangeEntry& value);

	state::MetadataV1Entry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::MetadataV1Entry& value);

	state::MetadataEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::MetadataEntry& value);

	state::MosaicEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::MosaicEntry& value);

	state::LevyEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::LevyEntry& value);

	state::MultisigEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::MultisigEntry& value);

	state::RootNamespaceHistory Convert(utils::AccountMigrationManager& accountMigrationManager, const state::RootNamespaceHistory& value);

	state::AccountProperties Convert(utils::AccountMigrationManager& accountMigrationManager, const state::AccountProperties& value);

	state::BcDriveEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::BcDriveEntry& value);

	state::CommitteeEntry Convert(utils::AccountMigrationManager& accountMigrationManager, const state::CommitteeEntry& value);

}}}
