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
#include <boost/interprocess/containers/map.hpp>
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
		typedef boost::interprocess::map<Address, Address, std::less<Address>, MemAllocator<Address>> PagedKeyMap;
		typedef boost::interprocess::map<Address, crypto::KeyPair, std::less<Address>, MemAllocator<crypto::KeyPair>> PagedKeyPairMap;
		typedef boost::interprocess::map<Address, Hash256, std::less<Address>, MemAllocator<Hash256>> PagedHashMap;
	public:
		explicit AccountMigrationManager(std::string path, std::shared_ptr<config::BlockchainConfigurationHolder> pHolder) : m_file(boost::interprocess::open_or_create, path.c_str(), 65536)
			, m_keyAllocator(m_file.get_segment_manager())
			, m_HashAllocator(m_file.get_segment_manager())
			, m_KeyPairAllocator(m_file.get_segment_manager())
			, m_pHolder(pHolder)
			, m_Generator()
		{
			m_HashMap = m_file.find_or_construct<PagedHashMap>("PagedHashMap")(std::less<Address>(), m_HashAllocator);
			m_keyMap = m_file.find_or_construct<PagedKeyMap>("PagedKeyMap")(std::less<Address>(), m_keyAllocator);
			m_KeyPairMap = m_file.find_or_construct<PagedKeyPairMap>("PagedKeyPairMap")(std::less<Address>(), m_KeyPairAllocator);
		}

	public:
		Address GetActiveAddress(const Address& sourceAddress);
		Address GetActiveAddress(const Key& sourceKey);
		boost::interprocess::offset_ptr<crypto::KeyPair> GetKeyPair(const Address& sourceAddress);
		boost::interprocess::offset_ptr<crypto::KeyPair> GetKeyPair(const Key& sourceKey);
		void writeToFile(std::string path);

	private:
		Address Get(const Address& account);

		void WriteAccounts();
	private:
		boost::interprocess::managed_mapped_file m_file;
		MemAllocator<Key> m_keyAllocator;
		MemAllocator<crypto::KeyPair> m_KeyPairAllocator;
		MemAllocator<Hash256> m_HashAllocator;
		PagedHashMap* m_HashMap;
		PagedKeyMap* m_keyMap;
		PagedKeyPairMap* m_KeyPairMap;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pHolder;
		EntropyGenerator m_Generator;
	};

	/// Insertion operator for outputting \a blockSpan to \a out.
	std::ostream& operator<<(std::ostream& out, const BlockSpan& blockSpan);
}}
