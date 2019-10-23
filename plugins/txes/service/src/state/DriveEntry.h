/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
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

    /// Drive action.
    enum class DriveActionType : uint8_t {
        Add,
        Remove
    };

    struct DriveAction {
        DriveActionType Type;
        Height ActionHeight;
    };

	struct FileInfo {
		uint64_t Size = 0;
        Amount Deposit;

		bool isActive() const {
			return Actions.back().Type == DriveActionType::Add;
		}

		std::vector<DriveAction> Actions;
        std::vector<PaymentInformation> Payments;
	};

	/// The map where key is hash of the file and value is file info.
	using FilesMap = std::map<Hash256, FileInfo>;

	struct ReplicatorInfo {
		Height Start;
		Height End;
		Amount Deposit;

        bool isActive() const {
            return End.unwrap() == 0;
        }

        void IncrementUndepositedFileCounter(const Hash256& file) {
            ++FilesWithoutDeposit[file];
        }

        void DecrementUndepositedFileCounter(const Hash256& file) {
            auto result = --FilesWithoutDeposit[file];

            if (!result)
                FilesWithoutDeposit.erase(file);
        }

		/// Set of files without deposit
        std::map<Hash256, uint16_t> FilesWithoutDeposit;
	};

	/// The map where key is replicator and value is info.
	using ReplicatorsMap = std::map<Key, ReplicatorInfo>;

	// Mixin for storing drive details.
	class DriveMixin {
	public:
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
		BlockDuration duration() const {
			return m_duration;
		}

		/// Sets the \a duration of the drive in blocks.
		void setDuration(const BlockDuration& duration) {
			m_duration = duration;
		}

        /// Gets the billing period of the drive in blocks.
        BlockDuration billingPeriod() const {
            return m_billingPeriod;
        }

        /// Sets the \a billingPeriod of the drive in blocks.
        void setBillingPeriod(const BlockDuration& billingPeriod) {
            m_billingPeriod = billingPeriod;
        }

		/// Gets the billing price of the drive in storage units.
		Amount billingPrice() const {
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

		/// Gets the drive size.
		uint64_t size() const {
			return m_size;
		}

		/// Sets the drive size.
		void setSize(const uint64_t& size) {
			m_size = size;
		}

		/// Gets the number of the drive replicas.
        uint16_t replicas() const {
			return m_replicas;
		}

		/// Sets the number of the drive \a replicas.
		void setReplicas(uint16_t replicas) {
			m_replicas = replicas;
		}

		/// Gets the number of the drive replicas.
        uint16_t minReplicators() const {
			return m_minReplicators;
		}

		/// Sets the number of the drive \a replicators which need for start of drive.
		void setMinReplicators(uint16_t replicators) {
			m_minReplicators = replicators;
		}

		/// Gets the number of the drive replicators for starting.
		uint8_t minApprovers() const {
			return m_minApprovers;
		}

		/// Sets the number of the drive \a approvers.
		void setMinApprovers(uint8_t approvers) {
			m_minApprovers = approvers;
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

		/// Gets replicator account keys.
		const ReplicatorsMap& replicators() const {
			return m_replicators;
		}

		/// Gets replicator account keys.
		ReplicatorsMap& replicators() {
			return m_replicators;
		}

		/// Returns \c true if \a key is a replicator.
		bool hasReplicator(const Key& key) const {
			return m_replicators.end() != m_replicators.find(key);
		}

	private:
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
		uint8_t m_minApprovers;
		FilesMap m_files;
		ReplicatorsMap m_replicators;
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
