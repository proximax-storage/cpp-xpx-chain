/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ArraySet.h"
#include <map>

namespace catapult { namespace state {

    /// Drive state.
    enum class DriveState : uint8_t {
        /// Drive is not started.
        NotStarted,

        /// Drive is pending for storage units.
        Pending,

        /// Drive is active.
        InProgress,

        /// Drive is finished.
        Finished
    };

    struct PaymentInformation {
        Key Receiver;
        catapult::Amount Amount;
        catapult::Height Height;
    };

    struct BillingPeriodDescription {
        Height Start;
        Height End;

        std::vector<PaymentInformation> Payments;
    };

	struct FileInfo {
		uint64_t Size;
	};

	/// The map where key is hash of the file and value is file info.
	using FilesMap = std::map<Hash256, FileInfo>;

	struct ReplicatorInfo {
		Height Start;
		Height End;

        void AddInactiveUndepositedFile(const Hash256& file, const Height& height) {
            InactiveFilesWithoutDeposit[file].push_back(height);
        }

        void RemoveInactiveUndepositedFile(const Hash256& file, const Height& height) {
            auto iter = InactiveFilesWithoutDeposit.find(file);
            if (iter != InactiveFilesWithoutDeposit.end()) {
                if (!iter->second.empty() && iter->second.back() == height)
                    iter->second.pop_back();

                if (iter->second.empty())
                    InactiveFilesWithoutDeposit.erase(iter);
            }
        }

		/// Set of active files without deposit
		std::set<Hash256> ActiveFilesWithoutDeposit;

		/// Map of inactive files without deposit
        std::map<Hash256, std::vector<Height>> InactiveFilesWithoutDeposit;
	};

	/// The map where key is replicator and value is info.
	using ReplicatorsMap = std::map<Key, ReplicatorInfo>;

	/// The vector of removed replicators.
	using RemovedReplicators = std::vector<std::pair<Key, ReplicatorInfo>>;

	/// The vector of upload payments.
	using UploadPayments = std::vector<PaymentInformation>;

	// Mixin for storing drive details.
	class DriveMixin {
	public:
		/// Sets start \a height of drive.
		void setStart(const Height& height) {
			m_start = height;
		}

		/// Gets start height of drive.
		const Height& start() const {
			return m_start;
		}

		/// Sets end \a height of drive.
		void setEnd(const Height& height) {
			m_end = height;
		}

		/// Gets end height of drive.
		const Height& end() const {
			return m_end;
		}

		/// Gets the state of the drive.
		DriveState state() const {
			return m_state;
		}

		/// Sets the \a state of the drive.
		void setState(const DriveState& state) {
			m_state = state;
		}

        /// Sets \a owner of drive.
        void setOwner(const Key& owner) {
            m_owner = owner;
        }

        /// Gets owner of drive.
        const Key& owner() const {
            return m_owner;
        }

        /// Sets \a rootHash of drive.
        void setRootHash(const Hash256& rootHash) {
            m_rootHash = rootHash;
        }

        /// Gets root hash of drive.
        const Hash256& rootHash() const {
            return m_rootHash;
        }

		/// Gets the duration of the drive in blocks.
		const BlockDuration& duration() const {
			return m_duration;
		}

		/// Sets the \a duration of the drive in blocks.
		void setDuration(const BlockDuration& duration) {
			m_duration = duration;
		}

        /// Gets the billing period of the drive in blocks.
        const BlockDuration& billingPeriod() const {
            return m_billingPeriod;
        }

        /// Sets the \a billingPeriod of the drive in blocks.
        void setBillingPeriod(const BlockDuration& billingPeriod) {
            m_billingPeriod = billingPeriod;
        }

		/// Gets the billing price of the drive in storage units.
		const Amount& billingPrice() const {
			return m_billingPrice;
		}

		/// Sets the \a billingPrice of the drive in storage units.
		void setBillingPrice(const Amount& billingPrice) {
            m_billingPrice = billingPrice;
		}

		/// Gets the drive billing history.
		const std::vector<BillingPeriodDescription>& billingHistory() const {
			return m_billingHistory;
		}

		/// Gets the drive billing history.
		std::vector<BillingPeriodDescription>& billingHistory() {
			return m_billingHistory;
		}

		/// Gets the processed duration of drive.
		BlockDuration processedDuration() const {
			BlockDuration temp;

			for (const auto& desc : m_billingHistory)
				if (!desc.Payments.empty())
					temp = temp + BlockDuration(desc.End.unwrap() - desc.Start.unwrap());

			return temp;
		}

		/// Gets the drive upload payments.
		const UploadPayments& uploadPayments() const {
			return m_uploadPayments;
		}

		/// Gets the drive upload payments.
        UploadPayments& uploadPayments() {
			return m_uploadPayments;
		}

		/// Gets the drive size.
		const uint64_t& size() const {
			return m_size;
		}

		/// Sets the drive size.
		void setSize(const uint64_t& size) {
			m_size = size;
		}

		/// Gets the number of the drive replicas.
        const uint16_t& replicas() const {
			return m_replicas;
		}

		/// Sets the number of the drive \a replicas.
		void setReplicas(uint16_t replicas) {
			m_replicas = replicas;
		}

		/// Gets the number of the drive replicas.
        const uint16_t& minReplicators() const {
			return m_minReplicators;
		}

		/// Sets the number of the drive \a replicators which need for start of drive.
		void setMinReplicators(uint16_t replicators) {
			m_minReplicators = replicators;
		}

		/// Gets the number of the drive replicators for starting.
		const uint8_t& percentApprovers() const {
			return m_percentApprovers;
		}

		/// Sets the number of the drive \a approvers.
		void setPercentApprovers(uint8_t approvers) {
			m_percentApprovers = approvers;
		}

		/// Gets files.
		const FilesMap& files() const {
			return m_files;
		}

		/// Gets files.
		FilesMap& files() {
			return m_files;
		}

		/// Returns \c true if drive contains a file with \a hash.
		bool hasFile(const Hash256& hash) const {
			return m_files.end() != m_files.find(hash);
		}

		/// Gets replicator infos.
		const ReplicatorsMap& replicators() const {
			return m_replicators;
		}

		/// Gets replicator infos.
		ReplicatorsMap& replicators() {
			return m_replicators;
		}

		/// Gets removed replicator infos.
		const RemovedReplicators& removedReplicators() const {
			return m_removedReplicators;
		}

		/// Gets removed replicator infos.
		RemovedReplicators& removedReplicators() {
			return m_removedReplicators;
		}

        void restoreReplicator(const Key& key) {
            if (m_removedReplicators.back().first != key)
                CATAPULT_THROW_RUNTIME_ERROR_1("failed to restore replicator, wrong key", key);

            m_replicators.insert(m_removedReplicators.back());
            m_removedReplicators.pop_back();
        }

		void removeReplicator(const Key& key) {
		    if (!m_replicators.count(key))
                CATAPULT_THROW_RUNTIME_ERROR_1("replicator not found, key", key);

		    auto iter = m_replicators.find(key);
		    m_removedReplicators.push_back(*iter);
		    m_replicators.erase(iter);
		}

		/// Returns \c true if \a key is a replicator.
		bool hasReplicator(const Key& key) const {
			return m_replicators.end() != m_replicators.find(key);
		}

	private:
		Height m_start;
		Height m_end;
		DriveState m_state;
		Key m_owner;
		Hash256 m_rootHash;
		BlockDuration m_duration;
		BlockDuration m_billingPeriod;
		Amount m_billingPrice;
		std::vector<BillingPeriodDescription> m_billingHistory;
		uint64_t m_size;
		uint16_t m_replicas;
		uint16_t m_minReplicators;
		// Percent 0-100
		uint8_t m_percentApprovers;
		FilesMap m_files;
		ReplicatorsMap m_replicators;
		// We don't remove info about removed replicators, we will store it in separate vector
		RemovedReplicators m_removedReplicators;
		// Payments in streaming units. After end of drive and last streaming payment we will store the payment for xpx here.
		UploadPayments m_uploadPayments;
	};

	// Drive entry.
	class DriveEntry : public DriveMixin {
	public:
		// Creates a drive entry around \a key.
		explicit DriveEntry(const Key& key) : m_key(key)
		{}

	public:
		// Gets the drive public key.
		const Key& key() const {
			return m_key;
		}

	private:
		Key m_key;
	};
}}
