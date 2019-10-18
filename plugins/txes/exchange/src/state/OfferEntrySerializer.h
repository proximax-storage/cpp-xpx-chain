/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading exchange entry data.
	struct OfferEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const OfferEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static OfferEntry Load(io::InputStream& input);
	};
}}
