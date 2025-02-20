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

#pragma once
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include "catapult/types.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/model/Address.h"
#include "catapult/crypto/Hashes.h"
#include "catapult/crypto/KeyPair.h"
#include <boost/random/random_device.hpp>

namespace catapult { namespace utils {

	/// Represents a block duration.
	class AccountMigrationManager {

		class EntropyGenerator {
		public:
			EntropyGenerator() : m_pRd(std::make_shared<boost::random_device>())
			{}

		public:
			uint8_t operator()() {
				return static_cast<uint8_t>((*m_pRd)());
			}

		private:
			std::shared_ptr<boost::random_device> m_pRd; // shared_ptr because entropy source needs to be copyable
		};

		template<typename TValueType>
		using MemAllocator = boost::interprocess::allocator<std::pair<const Address, TValueType>, boost::interprocess::managed_mapped_file::segment_manager>;

		using CharAllocator = boost::interprocess::allocator<char, boost::interprocess::managed_mapped_file::segment_manager>;
		using BipString = boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator>;

		using VectorAllocator = boost::interprocess::allocator<BipString, boost::interprocess::managed_mapped_file::segment_manager>;
		using BipVector = boost::interprocess::vector<BipString, VectorAllocator>;

		using VectorMapAllocator = boost::interprocess::allocator<std::pair<const Address, BipVector>, boost::interprocess::managed_mapped_file::segment_manager>;

		typedef boost::interprocess::map<Address, Address, std::less<Address>, MemAllocator<Address>> PagedKeyMap;
		typedef boost::interprocess::map<Address, crypto::KeyPair, std::less<Address>, MemAllocator<crypto::KeyPair>> PagedKeyPairMap;
		typedef boost::interprocess::map<Address, Hash256, std::less<Address>, MemAllocator<Hash256>> PagedHashMap;
		typedef boost::interprocess::map<Address, BipVector, std::less<Address>, VectorMapAllocator> MetadataMap;
	public:
		explicit AccountMigrationManager(const std::string& nemesisKeyPair, std::string path, std::shared_ptr<config::BlockchainConfigurationHolder> pHolder);




	public:
		Address GetActiveAddress(const Address& sourceAddress);
		Address GetActiveAddress(const Key& sourceKey);
		void AssociateNemesis(const Address& oldAddress);
		void AssociateNemesis(const Key& publicKey);

		/// Pushes a new metadata entry \a metadata to a new address \a address
		void PushMetadata(const Address& address, std::string metadata);

		/// Removes the last metadata entry from a new address \a address
		void PopMetadata(const Address& address);

		boost::interprocess::offset_ptr<crypto::KeyPair> GetKeyPair(const Address& sourceAddress);
		boost::interprocess::offset_ptr<crypto::KeyPair> GetKeyPair(const Key& sourceKey);
		void writeToFile(std::string path);
		~AccountMigrationManager();

	private:
		Address Get(const Address& account);

	private:
		boost::interprocess::managed_mapped_file m_file;
		MemAllocator<Key> m_keyAllocator;
		MemAllocator<crypto::KeyPair> m_KeyPairAllocator;
		MemAllocator<Hash256> m_HashAllocator;
		VectorMapAllocator m_vectorAllocator;
		PagedHashMap* m_HashMap;
		bool m_NemesisSet;
		PagedKeyMap* m_keyMap;
		MetadataMap* m_metadataMap;
		boost::filesystem::path m_path;
		PagedKeyPairMap* m_KeyPairMap;
		std::string m_NemesisPrivateKey;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pHolder;
		EntropyGenerator m_Generator;
	};

	/// Insertion operator for outputting \a blockSpan to \a out.
	std::ostream& operator<<(std::ostream& out, const BlockSpan& blockSpan);
}}
