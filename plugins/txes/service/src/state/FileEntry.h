/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ArraySet.h"
#include <vector>

namespace catapult { namespace state {

	struct DriveFileKey_tag { static constexpr auto Byte_Size = Key_Size + Hash256_Size; };
	using DriveFileKey = utils::ByteArray<Key_Size + Hash256_Size, DriveFileKey_tag>;

	DriveFileKey MakeDriveFileKey(const Key& drive, const Hash256& fileHash);

	// Mixin for storing file details.
	class FileMixin {
	public:
		/// Gets the parent key of the file.
		DriveFileKey parentKey() const {
			return m_parentKey;
		}

		/// Sets the \a parentKey of the file.
		void setParentKey(const DriveFileKey& parentKey) {
			m_parentKey = parentKey;
		}

		/// Gets the file name.
		std::string name() const {
			return m_name;
		}

		/// Sets the file \a name.
		void setName(const std::string& name) {
			m_name = name;
		}

	private:
		DriveFileKey m_parentKey;
		std::string m_name;
	};

	// File entry.
	class FileEntry : public FileMixin {
	public:
		// Creates a drive entry around \a key.
		explicit FileEntry(const DriveFileKey& key) : m_key(key)
		{}
		
		// Creates a drive entry around \a key.
		explicit FileEntry(const Key& drive, const Hash256& fileHash) : m_key(MakeDriveFileKey(drive, fileHash))
		{}

	public:
		// Gets the drive public key.
		const DriveFileKey& key() const {
			return m_key;
		}

	private:
		DriveFileKey m_key;
	};
}}
