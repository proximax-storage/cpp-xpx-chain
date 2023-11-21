/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaOfferGroupEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading SDA-SDA group entry data.
	struct SdaOfferGroupEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const SdaOfferGroupEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static SdaOfferGroupEntry Load(io::InputStream& input);
	};
}}
