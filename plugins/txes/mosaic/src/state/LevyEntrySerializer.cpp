/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/Casting.h"
#include "LevyEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {
	
	namespace {
		void WriteLevyData(io::OutputStream &output, const state::LevyEntryData &levy) {
			
			io::Write8(output, utils::to_underlying_type(levy.Type));
			output.write(levy.Recipient);
			io::Write(output, levy.MosaicId);
			io::Write(output, levy.Fee);
		}
		
		LevyEntryData ReadLevyData(io::InputStream &input) {
			Address recipient;
			model::LevyType type = (model::LevyType)io::Read8(input);
			input.read(recipient);
			MosaicId mosaicId = io::Read<MosaicId>(input);
			Amount fee = io::Read<Amount>(input);
			return LevyEntryData(type, recipient, mosaicId, fee);
		}
		
		void SaveLevyEntry(const LevyEntry &entry, io::OutputStream &output, bool includeHistory = true) {
			// write version
			io::Write32(output, 1);
			io::Write(output, entry.mosaicId());
			
			if (entry.levy() != nullptr) {
				io::Write8(output, 1);
				WriteLevyData(output, *entry.levy());
			} else {
				io::Write8(output, 0);
			}

			if (includeHistory) {
				io::Write16(output, utils::checked_cast<size_t, uint16_t>(entry.updateHistory().size()));
				auto history = entry.updateHistory();
				for (const auto &pair : history) {
					io::Write(output, pair.first);
					WriteLevyData(output, pair.second);
				}
			}
		}
		
		LevyEntry LoadLevyEntry(io::InputStream& input, bool loadHistory = true) {
			// read version
			VersionType version = io::Read32(input);
			if (version > 1)
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of LevyEntry", version);
			
			auto mosaicId = io::Read<MosaicId>(input);
			auto levyExist = io::Read8(input);
			std::shared_ptr<LevyEntryData> pLevy(nullptr);
			
			if(levyExist)
				pLevy = std::make_shared<LevyEntryData>(ReadLevyData(input));
			
			state::LevyEntry entry(mosaicId, pLevy);
			
			if(loadHistory) {
				uint16_t count = io::Read16(input);
				for(auto i=0; i < count; i++) {
					auto height = io::Read<Height>(input);
					auto levyData = ReadLevyData(input);
					entry.updateHistory().push_back(std::make_pair(height, std::move(levyData)));
				}
			}
			
			return entry;
		}
	}
	
	void LevyEntrySerializer::Save(const LevyEntry& entry, io::OutputStream& output) {
		SaveLevyEntry(entry, output);
	}
	
	void LevyEntryNonHistoricalSerializer::Save(const LevyEntry& entry, io::OutputStream& output) {
		SaveLevyEntry(entry, output, false);
	}
	
	LevyEntry LevyEntrySerializer::Load(io::InputStream& input) {
		return LoadLevyEntry(input);
	}
	
	LevyEntry LevyEntryNonHistoricalSerializer::Load(io::InputStream& input) {
		return LoadLevyEntry(input, false);
	}
}}
