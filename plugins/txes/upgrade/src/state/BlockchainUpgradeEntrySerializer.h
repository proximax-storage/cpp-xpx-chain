/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlockchainUpgradeEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading catapult upgrade entry data.
	struct BlockchainUpgradeEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const BlockchainUpgradeEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static BlockchainUpgradeEntry Load(io::InputStream& input);
	};
}}
