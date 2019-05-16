/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "ReputationEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/HexFormatter.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	void ReputationEntrySerializer::Save(const ReputationEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write64(output, entry.positiveInteractions().unwrap());
		io::Write64(output, entry.negativeInteractions().unwrap());
		io::Write(output, entry.key());
	}

	ReputationEntry ReputationEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of ReputationEntry", version);

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
