/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationEntry.h"
#include "plugins/txes/lock_shared/src/state/LockInfoSerializer.h"

namespace catapult { namespace state {

	/// Policy for saving and loading operation entry extended data.
	struct OperationEntryExtendedDataSerializer {
		/// Saves \a entry extended data to \a output.
		static void Save(const OperationEntry& entry, io::OutputStream& output);

		/// Loads operation entry extended data from \a input into \a entry.
		static void Load(io::InputStream& input, OperationEntry& entry);
	};

	/// Policy for saving and loading operation entry data.
	struct OperationEntrySerializer : public LockInfoSerializer<OperationEntry, OperationEntryExtendedDataSerializer, 2> {};
}}
