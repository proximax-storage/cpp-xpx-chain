/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReputationEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading reputation entry data.
	struct ReputationEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const ReputationEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static ReputationEntry Load(io::InputStream& input);
	};
}}
