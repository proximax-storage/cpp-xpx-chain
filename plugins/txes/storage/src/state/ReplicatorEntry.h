/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"
#include "catapult/state/StorageState.h"

namespace catapult { namespace state {

	struct DriveInfo {
		/// Identifier of the most recent data modification of the drive approved by the replicator.
		Hash256 LastApprovedDataModificationId;

		/// Indicates if \p LastApprovedDataModificationId is an identifier of an existing data modification.
		/// Can be \b false only if the drive had no approved data modifications when the replicator joined it.
		/// Set to \b true after replicator’s first data modification approval.
		bool DataModificationIdIsValid;

		/// Used drive size at the time of the replicator’s onboarding excluding metafiles size.
		/// Set to \p 0 after replicator’s first data modification approval.
		uint64_t InitialDownloadWork;
	};

	/// The map where key is drive and value is info.
	using DrivesMap = std::map<Key, DriveInfo>;

	// Mixin for storing replicator details.
	class ReplicatorMixin {
	public:
		ReplicatorMixin() = default;

	public:
		/// Sets the capacity of the replicator.
		void setCapacity(const Amount& capacity) {
			m_capacity = capacity;
		}

		/// Gets the capacity of the replicator.
		const Amount& capacity() const {
			return m_capacity;
		}

		/// Sets BLS public key of the replicator.
		void setBlsKey(const BLSPublicKey& blsKey) {
			m_blsKey = blsKey;
		}

		/// Gets BLS public key of the replicator.
		const BLSPublicKey& blsKey() const {
			return m_blsKey;
		}

		/// Gets infos of drives assigned to the replicator.
		const DrivesMap& drives() const {
			return m_drives;
		}

		/// Gets infos of drives assigned to the replicator.
		DrivesMap& drives() {
			return m_drives;
		}

	private:
		Amount m_capacity;
		BLSPublicKey m_blsKey;
		DrivesMap m_drives;
	};

	// Replicator entry.
	class ReplicatorEntry : public ReplicatorMixin {
	public:
		// Creates a replicator entry around \a key.
		explicit ReplicatorEntry(const Key& key) : m_key(key), m_version(1)
		{}

	public:
		// Gets the public key of the replicator.
		const Key& key() const {
			return m_key;
		}

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

	private:
		Key m_key;
		VersionType m_version;
	};
}}
