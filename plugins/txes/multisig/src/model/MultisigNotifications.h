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
#include "ModifyMultisigAccountTransaction.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region multisig notification types

/// Defines a multisig notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_MULTISIG_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Multisig, DESCRIPTION, CODE)

	/// Multisig account cosigners were modified.
	DEFINE_MULTISIG_NOTIFICATION(Modify_Cosigners_v1, 0x0001, All);

	/// A cosigner was added to a multisig account.
	DEFINE_MULTISIG_NOTIFICATION(Modify_New_Cosigner_v1, 0x0002, Validator);

	/// Multisig account settings were modified.
	DEFINE_MULTISIG_NOTIFICATION(Modify_Settings_v1, 0x1001, All);

#undef DEFINE_MULTISIG_NOTIFICATION

	// endregion

	/// Notification of a multisig cosigners modification.
	template<VersionType version>
	struct ModifyMultisigCosignersNotification;

	template<>
	struct ModifyMultisigCosignersNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Multisig_Modify_Cosigners_v1_Notification;

	public:
		/// Creates a notification around \a signer, \a modificationsCount and \a pModifications.
		explicit ModifyMultisigCosignersNotification(
				const Key& signer,
				uint8_t modificationsCount,
				const CosignatoryModification* pModifications,
				const bool& allowMultipleRemove = false)
				: Notification(Notification_Type, sizeof(ModifyMultisigCosignersNotification<1>))
				, Signer(signer)
				, ModificationsCount(modificationsCount)
				, ModificationsPtr(pModifications)
				, AllowMultipleRemove(allowMultipleRemove)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Number of modifications.
		uint8_t ModificationsCount;

		/// Const pointer to the first modification.
		const CosignatoryModification* ModificationsPtr;

		/// Allow multiple remove in modifications.
		bool AllowMultipleRemove;
	};

	/// Notification of a new cosigner.
	template<VersionType version>
	struct ModifyMultisigNewCosignerNotification;

	template<>
	struct ModifyMultisigNewCosignerNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Multisig_Modify_New_Cosigner_v1_Notification;

	public:
		/// Creates a notification around \a multisigAccountKey and \a cosignatoryKey.
		explicit ModifyMultisigNewCosignerNotification(const Key& multisigAccountKey, const Key& cosignatoryKey)
				: Notification(Notification_Type, sizeof(ModifyMultisigNewCosignerNotification<1>))
				, MultisigAccountKey(multisigAccountKey)
				, CosignatoryKey(cosignatoryKey)
		{}

	public:
		/// Multisig account key.
		const Key& MultisigAccountKey;

		/// New cosignatory key.
		const Key& CosignatoryKey;
	};

	/// Notification of a multisig settings modification.
	template<VersionType version>
	struct ModifyMultisigSettingsNotification;

	template<>
	struct ModifyMultisigSettingsNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Multisig_Modify_Settings_v1_Notification;

	public:
		/// Creates a notification around \a signer, \a minRemovalDelta and \a minApprovalDelta.
		explicit ModifyMultisigSettingsNotification(
				const Key& signer, int8_t minRemovalDelta, int8_t minApprovalDelta, uint8_t modificationsCount)
				: Notification(Notification_Type, sizeof(ModifyMultisigSettingsNotification<1>))
				, Signer(signer)
				, MinRemovalDelta(minRemovalDelta)
				, MinApprovalDelta(minApprovalDelta)
				, ModificationsCount(modificationsCount)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Relative change of cosigs needed to remove another cosig.
		int8_t MinRemovalDelta;

		/// Relative change of cosigs needed to approve a transaction.
		int8_t MinApprovalDelta;

		/// Count of modification in a transaction.
		uint8_t ModificationsCount;
	};
}}
