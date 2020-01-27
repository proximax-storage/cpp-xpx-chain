/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading super contract entry data.
	struct SuperContractEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const SuperContractEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static SuperContractEntry Load(io::InputStream& input);
	};
}}
