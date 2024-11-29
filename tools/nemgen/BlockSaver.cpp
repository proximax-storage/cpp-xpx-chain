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
#include <inttypes.h>

namespace catapult { namespace tools { namespace nemgen {

	namespace {
		void UpdateMemoryBlockStorageData(const model::Block& block, const std::string& cppFile, const std::string& cppFileHeader) {
			io::RawFile cppRawFile(cppFile, io::OpenMode::Read_Write);

			if (!cppFileHeader.empty()) {
				io::RawFile cppHeaderRawFile(cppFileHeader, io::OpenMode::Read_Only);
				std::vector<uint8_t> headerBuffer(cppHeaderRawFile.size());
				cppHeaderRawFile.read(headerBuffer);
				cppRawFile.write(headerBuffer);
			}

			auto header =
				"#pragma once\n"
				"#include <stdint.h>\n\n"
				"namespace catapult { namespace test {\n\n"
				"\tconstexpr inline Timestamp Nemesis_Timestamp = Timestamp(";
			cppRawFile.write(RawBuffer(reinterpret_cast<const uint8_t*>(header), strlen(header)));

			std::stringstream timestamp;
			timestamp << block.Timestamp;
			cppRawFile.write(RawBuffer(reinterpret_cast<const uint8_t*>(timestamp.str().c_str()), timestamp.str().size()));

			header =
				");\n\n"
				"\tconstexpr inline uint8_t MemoryBlockStorage_NemesisBlockData[] = {\n";
			cppRawFile.write(RawBuffer(reinterpret_cast<const uint8_t*>(header), strlen(header)));

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

		void UpdateFileBlockStorageDataSpool() {

		}
	}
	namespace {
		static constexpr uint64_t Unset_Directory_Id = std::numeric_limits<uint64_t>::max();
		static constexpr uint32_t Files_Per_Directory = 65536u;
		static constexpr auto Block_File_Extension = ".dat";
		static constexpr auto Block_Statement_File_Extension = ".stmt";

		// region path utils

#ifdef _MSC_VER
#define SPRINTF sprintf_s
#else
#define SPRINTF sprintf
#endif

		boost::filesystem::path GetDirectoryPath(const std::string& baseDirectory, Height height) {
			char subDirectory[16];
			SPRINTF(subDirectory, "%05" PRId64, height.unwrap() / Files_Per_Directory);
			boost::filesystem::path path = baseDirectory;
			path /= subDirectory;
			if (!boost::filesystem::exists(path))
				boost::filesystem::create_directory(path);

			return path;
		}

		boost::filesystem::path GetBlockPath(const std::string& baseDirectory, Height height, const char* extension) {
			auto path = GetDirectoryPath(baseDirectory, height);
			char filename[16];
			SPRINTF(filename, "%05" PRId64, height.unwrap() % Files_Per_Directory);
			path /= filename;
			path += extension;
			return path;
		}

		boost::filesystem::path GetHashFilePath(const std::string& baseDirectory, Height height) {
			auto path = GetDirectoryPath(baseDirectory, height);
			path /= "hashes.dat";
			return path;
		}

		boost::filesystem::path GetBlockStatementPath(const std::string& baseDirectory, Height height) {
			return GetBlockPath(baseDirectory, height, Block_Statement_File_Extension);
		}

		// endregion

		// region file utils

		bool IsRegularFile(const boost::filesystem::path& path) {
			return boost::filesystem::exists(path) && boost::filesystem::is_regular_file(path);
		}

		auto OpenBlockFile(const std::string& baseDirectory, Height height, io::OpenMode mode = io::OpenMode::Read_Only) {
			auto blockPath = GetBlockPath(baseDirectory, height, Block_File_Extension);
			return std::make_unique<io::RawFile>(blockPath.generic_string().c_str(), mode);
		}

		auto OpenBlockStatementFile(const std::string& baseDirectory, Height height, io::OpenMode mode = io::OpenMode::Read_Only) {
			auto blockStatementPath = GetBlockStatementPath(baseDirectory, height);
			return io::RawFile(blockStatementPath.generic_string().c_str(), mode);
		}

		class RawFileOutputStreamAdapter : public io::OutputStream {
		public:
			explicit RawFileOutputStreamAdapter(io::RawFile& rawFile) : m_rawFile(rawFile)
			{}

		public:
			void write(const RawBuffer& buffer) override {
				m_rawFile.write(buffer);
			}

			void flush() override {
				CATAPULT_THROW_INVALID_ARGUMENT("flush not supported");
			}

		private:
			io::RawFile& m_rawFile;
		};
		// endregion
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
			UpdateMemoryBlockStorageData(blockElement.Block, config.CppFile, config.CppFileHeader);
		}
	}

	void SaveNemesisBlockElementWithSpooling(const model::BlockElement& blockElement, const NemesisConfiguration& config, NemesisTransactions& transactions) {
		// 1. reset the index file
		io::IndexFile((boost::filesystem::path(config.BinDirectory) / "index.dat").generic_string()).set(0);

		// 2. update the file based storage data
		CATAPULT_LOG(info) << "creating binary storage seed in " << config.BinDirectory;
		UpdateFileBlockStorageData(blockElement, config.BinDirectory);

		// 3. update the memory based storage data
		if (!config.CppFile.empty()) {
			CATAPULT_LOG(info) << "creating cpp file " << config.CppFile;
			UpdateMemoryBlockStorageData(blockElement.Block, config.CppFile, config.CppFileHeader);
		}
	}

	namespace {
		void WriteTransactionHashes(io::OutputStream& outputStream, NemesisTransactions& transactions) {
			auto numTransactions = static_cast<uint32_t>(transactions.Total());
			Write32(outputStream, numTransactions);
			std::vector<Hash256> hashes(2 * numTransactions);
			auto iter = hashes.begin();
			auto view = transactions.createView();
			for (const auto& transactionElement : view) {
				*iter++ = transactionElement.entityHash;
				*iter++ = transactionElement.merkleHash;
			}

			outputStream.write({ reinterpret_cast<const uint8_t*>(hashes.data()), hashes.size() * Hash256_Size });
		}
	}

	void WriteBlockElement(io::OutputStream& outputStream, const model::BlockElement& blockElement, NemesisTransactions& transactions) {
		// 1. write constant size data
		outputStream.write({ reinterpret_cast<const uint8_t*>(&blockElement.Block), blockElement.Block.Size-transactions.Size() });
		{
			auto view = transactions.createView();
			for(const auto& transaction : view) {
				outputStream.write({ reinterpret_cast<const uint8_t*>(transaction.transaction.get()), transaction.transaction->Size });
			}
		}
		outputStream.write(blockElement.EntityHash);
		outputStream.write(blockElement.GenerationHash);

		// 2. write transaction hashes
		WriteTransactionHashes(outputStream, transactions);
	}
	void NemesisFileBlockStorage::saveBlock(
			const model::BlockElement& blockElement,
			NemesisTransactions& transactions) {

		auto currentHeight = chainHeight();
		auto height = blockElement.Block.Height;

		if (height != currentHeight + Height(1)) {
			std::ostringstream out;
			out << "cannot save block with height " << height << " when storage height is " << currentHeight;
			CATAPULT_THROW_INVALID_ARGUMENT(out.str().c_str());
		}

		{
			// write element
			auto pBlockFile = OpenBlockFile(m_dataDirectory, height, io::OpenMode::Read_Write);
			RawFileOutputStreamAdapter streamAdapter(*pBlockFile);
			WriteBlockElement(streamAdapter, blockElement, transactions);

			// write statements
			/*if (blockElement.OptionalStatement) {
				io::BufferedOutputFileStream blockStatementOutputStream(OpenBlockStatementFile(m_dataDirectory, height, io::OpenMode::Read_Write));
				io::WriteBlockStatement(blockStatementOutputStream, *blockElement.OptionalStatement);
				blockStatementOutputStream.flush();
			}*/
		}

		if (io::FileBlockStorageMode::Hash_Index == m_mode)
			m_hashFile.save(height, blockElement.EntityHash);

		if (height > currentHeight)
			m_indexFile.set(height.unwrap());

	}
}}}
