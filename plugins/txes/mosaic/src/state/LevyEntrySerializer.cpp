/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/catapult/utils/Casting.h>
#include "LevyEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {
		
	void LevyEntrySerializer::Save(const LevyEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);
		
		io::Write(output, entry.mosaicId());
		
		io::Write16(output, utils::to_underlying_type(entry.levy().Type));
		output.write(entry.levy().Recipient);
		io::Write(output, entry.levy().MosaicId);
		io::Write(output, entry.levy().Fee);
	}
		
	LevyEntry LevyEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of LevyEntry", version);
		
		auto mosaicId = io::Read<MosaicId>(input);
		
		model::MosaicLevy levy;
		levy.Type = (model::LevyType)io::Read16(input);
		input.read(levy.Recipient);
		levy.MosaicId = io::Read<MosaicId>(input);
		levy.Fee = io::Read<Amount>(input);
		
		return state::LevyEntry(mosaicId, levy);
	}
}}
