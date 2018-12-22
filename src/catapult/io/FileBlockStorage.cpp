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

#include "FileBlockStorage.h"
#include "PodIoUtils.h"
#include "RawFile.h"
#include "catapult/model/Elements.h"
#include "catapult/utils/MemoryUtils.h"
#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <inttypes.h>

using catapult::model::Block;
using catapult::model::BlockElement;

namespace catapult { namespace io {

	namespace {
		static constexpr uint64_t Unset_Directory_Id = std::numeric_limits<uint64_t>::max();
		static constexpr uint32_t Files_Per_Directory = 65536u;
		static constexpr auto Block_File_Extension = ".dat";
		static constexpr auto Index_File = "index.dat";

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

		boost::filesystem::path GetBlockPath(const std::string& baseDirectory, Height height) {
			auto path = GetDirectoryPath(baseDirectory, height);
			char filename[16];
			SPRINTF(filename, "%05" PRId64, height.unwrap() % Files_Per_Directory);
			path /= filename;
			path += Block_File_Extension;
			return path;
		}

		boost::filesystem::path GetHashFilePath(const std::string& baseDirectory, Height height) {
			auto path = GetDirectoryPath(baseDirectory, height);
			path /= "hashes.dat";
			return path;
		}

		auto OpenBlockFile(const std::string& baseDirectory, Height height, OpenMode mode = OpenMode::Read_Only) {
			auto blockPath = GetBlockPath(baseDirectory, height);
			return std::make_unique<RawFile>(blockPath.generic_string().c_str(), mode);
		}

		bool HasJournal(const std::string& baseDirectory) {
			boost::filesystem::path journalPath = baseDirectory;
			journalPath /= Index_File;

			return boost::filesystem::exists(journalPath) && boost::filesystem::is_regular_file(journalPath);
		}

		auto OpenJournalFile(const std::string& baseDirectory, OpenMode mode = OpenMode::Read_Only) {
			boost::filesystem::path journalPath = baseDirectory;
			journalPath /= Index_File;
			return std::make_unique<RawFile>(journalPath.generic_string().c_str(), mode);
		}

		// note: DeleteBlockFile returns false when attempting to delete nonexistent file.
		bool DeleteBlockFile(const std::string& baseDirectory, Height height) {
			auto blockPath = GetBlockPath(baseDirectory, height);
			return boost::filesystem::remove(blockPath);
		}
	}

	FileBlockStorage::HashFile::HashFile(const std::string& dataDirectory)
			: m_dataDirectory(dataDirectory)
			, m_cachedDirectoryId(Unset_Directory_Id)
	{}

	namespace {
		std::unique_ptr<RawFile> OpenHashFile(const std::string& baseDirectory, Height height, io::OpenMode openMode) {
			auto hashFilePath = GetHashFilePath(baseDirectory, height);
			auto pHashFile = std::make_unique<RawFile>(hashFilePath.generic_string().c_str(), openMode, LockMode::None);
			// check that first hash file has at least two hashes inside.
			if (height.unwrap() < Files_Per_Directory && Hash256_Size * 2 > pHashFile->size())
				CATAPULT_THROW_RUNTIME_ERROR_1("hashes.dat has invalid size", pHashFile->size());

			return pHashFile;
		}

		void SeekHashFile(RawFile& hashFile, Height height) {
			auto index = height.unwrap() % Files_Per_Directory;
			hashFile.seek(index * Hash256_Size);
		}
	}

	model::HashRange FileBlockStorage::HashFile::loadHashesFrom(Height height, size_t numHashes) const {
		uint8_t* pData = nullptr;
		auto range = model::HashRange::PrepareFixed(numHashes, &pData);

		while (numHashes) {
			auto pHashFile = OpenHashFile(m_dataDirectory, height, OpenMode::Read_Only);
			SeekHashFile(*pHashFile, height);

			auto count = Files_Per_Directory - (height.unwrap() % Files_Per_Directory);
			count = std::min<size_t>(numHashes, count);

			pHashFile->read(MutableRawBuffer(pData, count * Hash256_Size));

			pData += count * Hash256_Size;
			numHashes -= count;
			height = height + Height(count);
		}

		return range;
	}

	void FileBlockStorage::HashFile::save(Height height, const Hash256& hash) {
		auto currentId = height.unwrap() / Files_Per_Directory;
		if (m_cachedDirectoryId != currentId) {
			m_pCachedHashFile = OpenHashFile(m_dataDirectory, height, OpenMode::Read_Append);
			m_cachedDirectoryId = currentId;
		}

		SeekHashFile(*m_pCachedHashFile, height);
		m_pCachedHashFile->write(hash);
	}

	FileBlockStorage::FileBlockStorage(const std::string& dataDirectory)
			: m_dataDirectory(dataDirectory)
			, m_hashFile(m_dataDirectory)
	{}

	namespace {
		void SetHeight(const std::string& baseDirectory, Height height) {
			auto pJournalFile = OpenJournalFile(baseDirectory, OpenMode::Read_Write);
			Write(*pJournalFile, height);
		}
	}

	Height FileBlockStorage::chainHeight() const {
		if (!HasJournal(m_dataDirectory))
			return Height(1);

		auto pJournalFile = OpenJournalFile(m_dataDirectory);
		return Read<Height>(*pJournalFile);
	}

	namespace {
		std::shared_ptr<Block> ReadBlock(RawFile& blockFile) {
			auto size = Read32(blockFile);
			blockFile.seek(0);

			auto pBlock = utils::MakeSharedWithSize<Block>(size);
			blockFile.read({ reinterpret_cast<uint8_t*>(pBlock.get()), size });
			return pBlock;
		}

		std::shared_ptr<model::BlockElement> ReadBlockElement(RawFile& blockFile) {
			auto size = Read32(blockFile);
			blockFile.seek(0);

			// allocate memory for both the element and the block in one shot (Block data is appended)
			auto pData = utils::MakeUniqueWithSize<uint8_t>(sizeof(BlockElement) + size);

			// read the block data
			auto pBlockData = pData.get() + sizeof(BlockElement);
			blockFile.read({ pBlockData, size });

			// create the block element and transfer ownership from pData to pBlockElement
			auto pBlockElementRaw = new (pData.get()) BlockElement(*reinterpret_cast<Block*>(pBlockData));
			auto pBlockElement = std::shared_ptr<BlockElement>(pBlockElementRaw);
			pData.release();

			// read metadata
			blockFile.read(pBlockElement->EntityHash);
			blockFile.read(pBlockElement->GenerationHash);
			return pBlockElement;
		}

		void ReadTransactionHashes(RawFile& blockFile, BlockElement& blockElement) {
			auto numTransactions = Read32(blockFile);
			std::vector<Hash256> hashes(2 * numTransactions);
			blockFile.read({ reinterpret_cast<uint8_t*>(hashes.data()), hashes.size() * Hash256_Size });

			size_t i = 0;
			for (const auto& transaction : blockElement.Block.Transactions()) {
				blockElement.Transactions.push_back(model::TransactionElement(transaction));
				blockElement.Transactions.back().EntityHash = hashes[i++];
				blockElement.Transactions.back().MerkleComponentHash = hashes[i++];
			}
		}

		void ReadSubCacheMerkleRoots(RawFile& blockFile, std::vector<Hash256>& subCacheMerkleRoots) {
			auto numHashes = Read32(blockFile);
			subCacheMerkleRoots.resize(numHashes);
			blockFile.read({ reinterpret_cast<uint8_t*>(subCacheMerkleRoots.data()), numHashes * Hash256_Size });
		}
	}

	std::shared_ptr<const model::Block> FileBlockStorage::loadBlock(Height height) const {
		if (height > chainHeight())
			CATAPULT_THROW_INVALID_ARGUMENT_1("cannot load block at height greater than chain height", height);

		auto pBlockFile = OpenBlockFile(m_dataDirectory, height);
		return ReadBlock(*pBlockFile);
	}

	std::shared_ptr<const model::BlockElement> FileBlockStorage::loadBlockElement(Height height) const {
		if (height > chainHeight())
			CATAPULT_THROW_INVALID_ARGUMENT_1("cannot load block at height greater than chain height", height);

		auto pBlockFile = OpenBlockFile(m_dataDirectory, height);
		auto pBlockElement = ReadBlockElement(*pBlockFile);

		ReadTransactionHashes(*pBlockFile, *pBlockElement);
		ReadSubCacheMerkleRoots(*pBlockFile, pBlockElement->SubCacheMerkleRoots);
		return pBlockElement;
	}

	model::HashRange FileBlockStorage::loadHashesFrom(Height height, size_t maxHashes) const {
		auto currentHeight = chainHeight();
		if (Height(0) == height || currentHeight < height)
			return model::HashRange();

		auto numAvailableHashes = static_cast<size_t>((currentHeight - height).unwrap() + 1);
		auto numHashes = std::min(maxHashes, numAvailableHashes);
		return m_hashFile.loadHashesFrom(height, numHashes);
	}

	namespace {
		void WriteTransactionHashes(RawFile& blockFile, const std::vector<model::TransactionElement>& transactionElements) {
			auto numTransactions = static_cast<uint32_t>(transactionElements.size());
			Write32(blockFile, numTransactions);
			std::vector<Hash256> hashes(2 * numTransactions);
			auto iter = hashes.begin();
			for (const auto& transactionElement : transactionElements) {
				*iter++ = transactionElement.EntityHash;
				*iter++ = transactionElement.MerkleComponentHash;
			}

			blockFile.write({ reinterpret_cast<const uint8_t*>(hashes.data()), hashes.size() * Hash256_Size });
		}

		void WriteSubCacheMerkleRoots(RawFile& blockFile, const std::vector<Hash256>& subCacheMerkleRoots) {
			auto numHashes = static_cast<uint32_t>(subCacheMerkleRoots.size());
			Write32(blockFile, numHashes);
			blockFile.write({ reinterpret_cast<const uint8_t*>(subCacheMerkleRoots.data()), numHashes * Hash256_Size });
		}
	}

	void FileBlockStorage::saveBlock(const model::BlockElement& blockElement) {
		auto currentHeight = chainHeight();
		auto height = blockElement.Block.Height;

		if (height != currentHeight + Height(1))
			CATAPULT_THROW_INVALID_ARGUMENT_1("cannot save out of order block at height", height);

		{
			// 1. write constant size data
			auto pBlockFile = OpenBlockFile(m_dataDirectory, height, OpenMode::Read_Write);
			pBlockFile->write({ reinterpret_cast<const uint8_t*>(&blockElement.Block), blockElement.Block.Size });
			pBlockFile->write(blockElement.EntityHash);
			pBlockFile->write(blockElement.GenerationHash);

			// 2. write transaction hashes
			WriteTransactionHashes(*pBlockFile, blockElement.Transactions);

			// 3. write sub cache merkle roots
			WriteSubCacheMerkleRoots(*pBlockFile, blockElement.SubCacheMerkleRoots);
		}

		m_hashFile.save(height, blockElement.EntityHash);

		if (height > currentHeight)
			SetHeight(m_dataDirectory, height);
	}

	void FileBlockStorage::dropBlocksAfter(Height height) {
		SetHeight(m_dataDirectory, height);
	}

	void FileBlockStorage::pruneBlocksBefore(Height pruneHeight) {
		auto currentHeight = chainHeight();

		if (pruneHeight > currentHeight)
			CATAPULT_THROW_INVALID_ARGUMENT_1("prune requested with height", pruneHeight);

		for (auto height = pruneHeight - Height(1); height > Height(1); height = height - Height(1))
			if (!DeleteBlockFile(m_dataDirectory, height))
				break;
	}
}}
