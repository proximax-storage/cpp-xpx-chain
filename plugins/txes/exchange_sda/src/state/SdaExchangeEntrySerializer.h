/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaExchangeEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading exchange entry data.
	struct SdaExchangeEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const SdaExchangeEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static SdaExchangeEntry Load(io::InputStream& input);
	};
}}
