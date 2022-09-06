/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include "GlobalEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading global entries.
	struct GlobalEntrySerializer {
		/// Serialized state version.
		static constexpr uint16_t State_Version = 1;

		/// Saves \a restrictions to \a output.
		static void Save(const GlobalEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static GlobalEntry Load(io::InputStream& input);
	};
}}
