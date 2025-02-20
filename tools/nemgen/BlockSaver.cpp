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
#include "catapult/io/BlockStatementSerializer.h"
#include <inttypes.h>

namespace catapult { namespace tools { namespace nemgen {

	namespace {
		void UpdateMemoryBlockStorageData(const model::Block& block, const std::string& cppFile, const std::string& cppFileHeader, const std::string& cppVariableName, NemesisTransactions* transactions, bool enableSpool) {
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

			if(enableSpool) {
				auto transactionsView = transactions->createView();
				for(auto transaction : transactionsView) {
					auto pCurrent = reinterpret_cast<const uint8_t*>(&*transaction.transaction);
					auto pEnd = pCurrent + transaction.transaction->Size;
					std::stringstream buffer;
					while (pCurrent != pEnd) {
						buffer << "\t\t";

						auto lineEnd = std::min(pCurrent + 16, pEnd);
						for (; pCurrent != lineEnd; ++pCurrent)
							buffer << "0x" << utils::HexFormat(*pCurrent) << ((pCurrent + 1 == lineEnd) ? "," : ", ");

						buffer << "\n";
					}
					cppRawFile.write(RawBuffer(reinterpret_cast<const uint8_t*>(buffer.str().c_str()), buffer.str().size()));
				}
			}
			else {
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
			}

			auto footer = "\t};\n}}\n";
			cppRawFile.write(RawBuffer(reinterpret_cast<const uint8_t*>(footer), strlen(footer)));
		}

		void UpdateFileBlockStorageData(const model::BlockElement& blockElement, const std::string& binDirectory, const Height& nemesisHeight) {
			io::FileBlockStorage storage(binDirectory, nemesisHeight);
			storage.saveBlock(blockElement);
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

		boost::filesystem::path GetDirectoryPath(const std::string& baseDirectory, Height adjustedHeight) {
			char subDirectory[16];
			SPRINTF(subDirectory, "%05" PRId64, adjustedHeight.unwrap() / Files_Per_Directory);
			boost::filesystem::path path = baseDirectory;
			path /= subDirectory;
			if (!boost::filesystem::exists(path))
				boost::filesystem::create_directory(path);

			return path;
		}

		boost::filesystem::path GetBlockPath(const std::string& baseDirectory, Height adjustedHeight, const char* extension) {
			auto path = GetDirectoryPath(baseDirectory, adjustedHeight);
			char filename[16];
			SPRINTF(filename, "%05" PRId64, adjustedHeight.unwrap() % Files_Per_Directory);
			path /= filename;
			path += extension;
			return path;
		}

		boost::filesystem::path GetHashFilePath(const std::string& baseDirectory, Height adjustedHeight) {
			auto path = GetDirectoryPath(baseDirectory, adjustedHeight);
			path /= "hashes.dat";
			return path;
		}

		boost::filesystem::path GetBlockStatementPath(const std::string& baseDirectory, Height adjustedHeight) {
			return GetBlockPath(baseDirectory, adjustedHeight, Block_Statement_File_Extension);
		}

		// endregion

		// region file utils

		bool IsRegularFile(const boost::filesystem::path& path) {
			return boost::filesystem::exists(path) && boost::filesystem::is_regular_file(path);
		}

		auto OpenBlockFile(const std::string& baseDirectory, Height adjustedHeight, io::OpenMode mode = io::OpenMode::Read_Only) {
			auto blockPath = GetBlockPath(baseDirectory, adjustedHeight, Block_File_Extension);
			return std::make_unique<io::RawFile>(blockPath.generic_string().c_str(), mode);
		}

		auto OpenBlockStatementFile(const std::string& baseDirectory, Height adjustedHeight, io::OpenMode mode = io::OpenMode::Read_Only) {
			auto blockStatementPath = GetBlockStatementPath(baseDirectory, adjustedHeight);
			return io::RawFile(blockStatementPath.generic_string().c_str(), mode);
		}

		// endregion
	}

	namespace {
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
		UpdateFileBlockStorageData(blockElement, config.BinDirectory, blockElement.Block.Height);

		// 3. update the memory based storage data
		if (!config.CppFile.empty()) {
			CATAPULT_LOG(info) << "creating cpp file " << config.CppFile;
			UpdateMemoryBlockStorageData(blockElement.Block, config.CppFile, config.CppFileHeader, config.CppVariableName, nullptr, false);
		}
	}

	void SaveNemesisBlockElementWithSpooling(const model::BlockElement& blockElement, const NemesisConfiguration& config, NemesisTransactions& transactions) {
		// 1. reset the index file
		io::IndexFile((boost::filesystem::path(config.BinDirectory) / "index.dat").generic_string()).set((blockElement.Block.Height).unwrap());

		// 2. update the file based storage data
		CATAPULT_LOG(info) << "creating binary storage seed in " << config.BinDirectory;
		NemesisFileBlockStorage storage(config.BinDirectory, blockElement.Block.Height);
		storage.saveBlock(blockElement, transactions, config.EnableSpool);

		// 3. update the memory based storage data
		if (!config.CppFile.empty()) {
			CATAPULT_LOG(info) << "creating cpp file " << config.CppFile;
			UpdateMemoryBlockStorageData(blockElement.Block, config.CppFile, config.CppFileHeader, config.CppVariableName, &transactions, config.EnableSpool);
		}
	}

	namespace {
		void WriteTransactionHashes(io::OutputStream& outputStream, NemesisTransactions& transactions, bool enableSpool) {
			auto numTransactions = static_cast<uint32_t>(transactions.Total());
			Write32(outputStream, numTransactions);
			if(enableSpool) {
				auto view = transactions.createView();
				for (const auto& transactionElement : view) {
					outputStream.write({ reinterpret_cast<const uint8_t*>(transactionElement.entityHash.data()), Hash256_Size });
					outputStream.write({ reinterpret_cast<const uint8_t*>(transactionElement.merkleHash.data()), Hash256_Size });
				}
			}
			else {
				for (const auto& transactionElement : transactions.transactions()) {
					outputStream.write({ reinterpret_cast<const uint8_t*>(transactionElement.entityHash.data()), Hash256_Size });
					outputStream.write({ reinterpret_cast<const uint8_t*>(transactionElement.merkleHash.data()), Hash256_Size });
				}
			}

		}

		void WriteSubCacheMerkleRoots(io::OutputStream& outputStream, const std::vector<Hash256>& subCacheMerkleRoots) {
			auto numHashes = static_cast<uint32_t>(subCacheMerkleRoots.size());
			Write32(outputStream, numHashes);
			outputStream.write({ reinterpret_cast<const uint8_t*>(subCacheMerkleRoots.data()), numHashes * Hash256_Size });
		}
	}

	void WriteBlockElement(io::OutputStream& outputStream, const model::BlockElement& blockElement, NemesisTransactions& transactions, bool enableSpool) {
		// 1. write constant size data
		outputStream.write({ reinterpret_cast<const uint8_t*>(&blockElement.Block), enableSpool ? blockElement.Block.Size-transactions.Size() : blockElement.Block.Size});
		if(enableSpool)
		{
			auto view = transactions.createView();
			for(const auto& transaction : view) {
				outputStream.write({ reinterpret_cast<const uint8_t*>(transaction.transaction.get()), transaction.transaction->Size });
			}
		}
		outputStream.write(blockElement.EntityHash);
		outputStream.write(blockElement.GenerationHash);

		// 2. write transaction hashes
		WriteTransactionHashes(outputStream, transactions, enableSpool);

		// 3. write sub cache merkle roots
		WriteSubCacheMerkleRoots(outputStream, blockElement.SubCacheMerkleRoots);
	}

	void NemesisFileBlockStorage::saveBlock(
			const model::BlockElement& blockElement,
			NemesisTransactions& transactions,
			bool enableSpool) {

		auto currentHeight = chainHeight();
		auto height = blockElement.Block.Height;

		// Following check must be disabled as storage height does not matter since new block is not at height 1
		//if (height != currentHeight + Height(1)) {
		//	std::ostringstream out;
		//	out << "cannot save block with height " << height << " when storage height is " << currentHeight;
		//	CATAPULT_THROW_INVALID_ARGUMENT(out.str().c_str());
		//}

		{
			// write element
			auto pBlockFile = OpenBlockFile(m_dataDirectory, Height(1), io::OpenMode::Read_Write);
			RawFileOutputStreamAdapter streamAdapter(*pBlockFile);
			WriteBlockElement(streamAdapter, blockElement, transactions, enableSpool);

			// write statements
			if (blockElement.OptionalStatement) {
				io::BufferedOutputFileStream blockStatementOutputStream(OpenBlockStatementFile(m_dataDirectory,  Height(1), io::OpenMode::Read_Write));
				io::WriteBlockStatement(blockStatementOutputStream, *blockElement.OptionalStatement);
				blockStatementOutputStream.flush();
			}
		}

		if (io::FileBlockStorageMode::Hash_Index == m_mode)
			m_hashFile.save(height, blockElement.EntityHash);

		if (height > currentHeight)
			m_indexFile.set(height.unwrap());

	}
}}}
