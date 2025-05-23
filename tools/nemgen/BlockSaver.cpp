/**
*** Copyright (c) 2016-present,
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

#include "BlockSaver.h"
#include "NemesisConfiguration.h"
#include "catapult/io/FileBlockStorage.h"

namespace catapult { namespace tools { namespace nemgen {

	namespace {
		void UpdateMemoryBlockStorageData(const model::Block& block, const std::string& cppFile, const std::string& cppFileHeader, const std::string& cppVariableName) {
			io::RawFile cppRawFile(cppFile, io::OpenMode::Read_Write);

			if (!cppFileHeader.empty()) {
				io::RawFile cppHeaderRawFile(cppFileHeader, io::OpenMode::Read_Only);
				std::vector<uint8_t> headerBuffer(cppHeaderRawFile.size());
				cppHeaderRawFile.read(headerBuffer);
				cppRawFile.write(headerBuffer);
			}

			std::string nemesisTimestampVariablePrefix;
			if (cppVariableName.find("Extended_Basic_") != std::string::npos) {
				nemesisTimestampVariablePrefix = "Extended_Basic_";
			} else if (cppVariableName.find("Basic_") != std::string::npos) {
				nemesisTimestampVariablePrefix = "Basic_";
			}

			std::string header =
				"#pragma once\n"
				"#include <stdint.h>\n\n"
				"namespace catapult { namespace test {\n\n"
				"\tconstexpr inline Timestamp " + nemesisTimestampVariablePrefix + "Nemesis_Timestamp = Timestamp(";
			cppRawFile.write(RawBuffer(reinterpret_cast<const uint8_t*>(header.data()), header.size()));

			std::stringstream timestamp;
			timestamp << block.Timestamp;
			cppRawFile.write(RawBuffer(reinterpret_cast<const uint8_t*>(timestamp.str().c_str()), timestamp.str().size()));

			header =
				");\n\n"
				"\tconstexpr inline uint8_t "+cppVariableName+"[] = {\n";
			cppRawFile.write(RawBuffer(reinterpret_cast<const uint8_t*>(header.data()), header.size()));

			auto pCurrent = reinterpret_cast<const uint8_t*>(&block);
			auto pEnd = pCurrent + block.Size;
			std::stringstream buffer;
			while (pCurrent != pEnd) {
				buffer << "\t\t";

				auto lineEnd = std::min(pCurrent + 16, pEnd);
				for (; pCurrent != lineEnd; ++pCurrent)
					buffer << "0x" << utils::HexFormat(*pCurrent) << ((pCurrent + 1 == lineEnd) ? "," : ", ");

				buffer << "\n";
			}

			cppRawFile.write(RawBuffer(reinterpret_cast<const uint8_t*>(buffer.str().c_str()), buffer.str().size()));

			auto footer = "\t};\n}}\n";
			cppRawFile.write(RawBuffer(reinterpret_cast<const uint8_t*>(footer), strlen(footer)));
		}

		void UpdateFileBlockStorageData(const model::BlockElement& blockElement, const std::string& binDirectory) {
			io::FileBlockStorage storage(binDirectory);
			storage.saveBlock(blockElement);
		}
	}

	void SaveNemesisBlockElement(const model::BlockElement& blockElement, const NemesisConfiguration& config) {
		// 1. reset the index file
		io::IndexFile((boost::filesystem::path(config.BinDirectory) / "index.dat").generic_string()).set(0);

		// 2. update the file based storage data
		CATAPULT_LOG(info) << "creating binary storage seed in " << config.BinDirectory;
		UpdateFileBlockStorageData(blockElement, config.BinDirectory);

		// 3. update the memory based storage data
		if (!config.CppFile.empty()) {
			CATAPULT_LOG(info) << "creating cpp file " << config.CppFile;
			UpdateMemoryBlockStorageData(blockElement.Block, config.CppFile, config.CppFileHeader, config.CppVariableName);
		}
	}
}}}
