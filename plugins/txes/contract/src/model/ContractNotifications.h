/**
*** Copyright (c) 2018-present,
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
#include "ModifyContractTransaction.h"
#include "catapult/model/Notifications.h"
#include "catapult/utils/MemoryUtils.h"

namespace catapult { namespace model {

	// region reputation notification types

	/// Defines a reputation notification type.
	DEFINE_NOTIFICATION_TYPE(Observer, Reputation, Update, 0x0001);

	// endregion

	/// Notification of a reputation update.
	struct ReputationUpdateNotification : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Reputation_Update_Notification;

	private:
		/// Creates a notification around \a signer, \a modificationCount and \a pModifications.
		explicit ReputationUpdateNotification(
			const std::vector<const CosignatoryModification*>& modifications)
			: Notification(Notification_Type, RealSize(modifications))
		{}

		uint8_t* ModificationsStart() {
			return reinterpret_cast<uint8_t*>(this) + sizeof(ReputationUpdateNotification);
		}

		const void* ModificationsStart() const {
			return reinterpret_cast<const uint8_t*>(this) + sizeof(ReputationUpdateNotification);
		}

		static size_t HeaderSize() {
			return sizeof(ReputationUpdateNotification);
		}

		static size_t BodySize(const std::vector<const CosignatoryModification*>& modifications) {
			return sizeof(modifications.data()) * modifications.size();
		}

		static size_t RealSize(const std::vector<const CosignatoryModification*>& modifications) {
			return HeaderSize() + BodySize(modifications);
		}

	public:
		/// Number of customer modifications.
		size_t ModificationCount() const {
			return (Size - HeaderSize()) / sizeof(const CosignatoryModification*);
		}

		/// Const pointer to the first customer modification.
		const CosignatoryModification* const * ModificationsPtr() const {
			return reinterpret_cast<const CosignatoryModification* const *>(ModificationsStart());
		}

		static std::unique_ptr<ReputationUpdateNotification> CreateReputationUpdateNotification(const std::vector<const CosignatoryModification*>& modifications) {
			auto smart_ptr = utils::MakeUniqueWithSize<ReputationUpdateNotification>(RealSize(modifications));
			auto& notification = *smart_ptr.get();
			notification.Type = Notification_Type;
			notification.Size = RealSize(modifications);

			std::memcpy(notification.ModificationsStart(), modifications.data(), BodySize(modifications));

			return smart_ptr;
		}
	};

	// endregion

	// region contract notification types

	/// Defines a contract notification type.
	DEFINE_NOTIFICATION_TYPE(Observer, Contract, Modify, 0x0002);

	// endregion

	/// Notification of a contract update.
	struct ModifyContractNotification : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Contract_Modify_Notification;

	public:
		/// Creates a notification around \a signer, \a modificationCount and \a pModifications.
		explicit ModifyContractNotification(
			const int64_t& durationDelta,
			const Key& multisig,
			const Hash256& hash,
			const uint8_t& customerModificationCount,
			const CosignatoryModification* pCustomerModifications,
			const uint8_t& executorModificationCount,
			const CosignatoryModification* pExecutorModifications,
			const uint8_t& verifierModificationCount,
			const CosignatoryModification* pVerifierModifications)
			: Notification(Notification_Type, sizeof(ModifyContractNotification))
			, DurationDelta(durationDelta)
			, Multisig(multisig)
			, Hash(hash)
			, CustomerModificationCount(customerModificationCount)
			, CustomerModificationsPtr(pCustomerModifications)
			, ExecutorModificationCount(executorModificationCount)
			, ExecutorModificationsPtr(pExecutorModifications)
			, VerifierModificationCount(verifierModificationCount)
			, VerifierModificationsPtr(pVerifierModifications)
		{}

	public:
		/// Relative change of the duration of the contract in blocks.
		int64_t DurationDelta;

		/// Public key of the contract multisig account.
		Key Multisig;

		/// Hash of an entity passed from customers to executors (e.g. file hash).
		Hash256 Hash;

		/// Number of customer modifications.
		uint8_t CustomerModificationCount;

		/// Const pointer to the first customer modification.
		const CosignatoryModification* CustomerModificationsPtr;

		/// Number of executor modifications.
		uint8_t ExecutorModificationCount;

		/// Const pointer to the first executor modification.
		const CosignatoryModification* ExecutorModificationsPtr;

		/// Number of verifier modifications.
		uint8_t VerifierModificationCount;

		/// Const pointer to the first verifier modification.
		const CosignatoryModification* VerifierModificationsPtr;
	};

	// endregion
}}
