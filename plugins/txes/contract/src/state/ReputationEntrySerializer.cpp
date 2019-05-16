/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReputationEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/HexFormatter.h"

namespace catapult { namespace state {

	void ReputationEntrySerializer::Save(const ReputationEntry& entry, io::OutputStream& output) {
		io::Write64(output, entry.positiveInteractions().unwrap());
		io::Write64(output, entry.negativeInteractions().unwrap());
		io::Write(output, entry.key());
	}

	ReputationEntry ReputationEntrySerializer::Load(io::InputStream& input) {
		auto positiveInteractions = Reputation{io::Read64(input)};
		auto negativeInteractions = Reputation{io::Read64(input)};
		Key key;
		input.read(key);

		auto entry = state::ReputationEntry(key);
		entry.setPositiveInteractions(positiveInteractions);
		entry.setNegativeInteractions(negativeInteractions);

		return entry;
	}
}}
