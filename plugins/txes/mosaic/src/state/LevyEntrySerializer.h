/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "LevyEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {
		
	/// Policy for saving and loading catapult upgrade entry data.
	struct LevyEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const LevyEntry& entry, io::OutputStream& output);
			
		/// Loads a single value from \a input.
		static LevyEntry Load(io::InputStream& input);
	};
		
	/// Policy for saving and loading levy entry data without historical information.
	struct LevyEntryNonHistoricalSerializer {
		/// Saves \a entry to \a output.
		static void Save(const LevyEntry& entry, io::OutputStream& output);
		
		/// Loads a single value from \a input.
		static LevyEntry Load(io::InputStream& input);
	};
}}

