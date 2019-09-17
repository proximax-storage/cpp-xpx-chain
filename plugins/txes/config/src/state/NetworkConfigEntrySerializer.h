/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NetworkConfigEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading network config entry data.
	struct NetworkConfigEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const NetworkConfigEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static NetworkConfigEntry Load(io::InputStream& input);
	};
}}
