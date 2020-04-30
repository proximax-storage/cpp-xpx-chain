/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading exchange entry data without historical information.
	struct ExchangeEntryNonHistoricalSerializer {
		/// Saves \a entry to \a output.
		static void Save(const ExchangeEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static ExchangeEntry Load(io::InputStream& input);
	};

	/// Policy for saving and loading exchange entry data.
	struct ExchangeEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const ExchangeEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static ExchangeEntry Load(io::InputStream& input);
	};
}}
