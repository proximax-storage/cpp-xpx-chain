/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"
#include "src/model/SuperContractTypes.h"
#include "catapult/utils/MemoryUtils.h"
#include <vector>

namespace catapult { namespace model {

	/// Defines a deploy notification type.
	DEFINE_NOTIFICATION_TYPE(All, SuperContract, Deploy_v1, 0x0001);

	/// Defines a super contract notification type.
	DEFINE_NOTIFICATION_TYPE(Validator, SuperContract, SuperContract_v1, 0x0002);

	/// Defines a deactivate notification type.
	DEFINE_NOTIFICATION_TYPE(All, SuperContract, StartExecute_v1, 0x0003);

	/// Defines a deactivate notification type.
	DEFINE_NOTIFICATION_TYPE(All, SuperContract, EndExecute_v1, 0x0004);

	/// Defines a deactivate notification type.
	DEFINE_NOTIFICATION_TYPE(All, SuperContract, Deactivate_v1, 0x0005);

	/// Defines a suspend notification type.
	DEFINE_NOTIFICATION_TYPE(All, SuperContract, Suspend_v1, 0x0006);

	/// Defines a resume notification type.
	DEFINE_NOTIFICATION_TYPE(All, SuperContract, Resume_v1, 0x0007);

	struct BaseSuperContractNotification : public Notification {
	public:
		explicit BaseSuperContractNotification(
			NotificationType type,
			size_t size,
			const Key& contract)
			: Notification(type, size)
			, SuperContract(contract)
		{}

	public:
		/// Public key of the super contract multisig account.
		Key SuperContract;
	};

    /// Deploy super contract notification.
    template<VersionType version>
    struct DeployNotification;

    template<>
    struct DeployNotification<1> : public BaseSuperContractNotification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = SuperContract_Deploy_v1_Notification;

    public:
        /// Creates a notification around \a contract, \a owner, \a drive, \a file and \a version.
        explicit DeployNotification(
			const Key& contract,
			const Key& owner,
			const Key& drive,
			const Hash256& file,
			const VmVersion& version)
			: BaseSuperContractNotification(Notification_Type, sizeof(DeployNotification<1>), contract)
			, Owner(owner)
			, Drive(drive)
			, FileHash(file)
			, VmVersion(version)
        {}

    public:
        /// Owner of super contract.
        const Key& Owner;

        /// Drive according to super contract.
        const Key& Drive;

        /// Hash of according file.
        const Hash256& FileHash;

        /// Version of super contract.
        catapult::VmVersion VmVersion;
    };

    /// Notification of a super contract.
    template<VersionType version>
    struct SuperContractNotification;

    template<>
    struct SuperContractNotification<1> : public BaseSuperContractNotification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = SuperContract_SuperContract_v1_Notification;

    public:
        explicit SuperContractNotification(const Key& contract, const model::EntityType& type)
			: BaseSuperContractNotification(Notification_Type, sizeof(SuperContractNotification<1>), contract)
			, TransactionType(type)
        {}

    public:
		/// Transactions type.
        model::EntityType TransactionType;
    };

    /// Deactivate super contract notification.
    template<VersionType version>
    struct DeactivateNotification;

    template<>
    struct DeactivateNotification<1> : public BaseSuperContractNotification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = SuperContract_Deactivate_v1_Notification;

    public:
        /// Creates a notification around \a signer, \a superContract and \a driveKey.
        explicit DeactivateNotification(
			const Key& signer,
			const Key& superContract,
			const Key& driveKey)
			: BaseSuperContractNotification(Notification_Type, sizeof(DeactivateNotification<1>), superContract)
			, Signer(signer)
			, DriveKey(driveKey)
        {}

    public:
		/// Signer.
		const Key& Signer;

        /// Drive key.
        const Key& DriveKey;
    };

    /// Notification of a start execute super contract.
    template<VersionType version>
    struct StartExecuteNotification;

    template<>
    struct StartExecuteNotification<1> : public BaseSuperContractNotification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = SuperContract_StartExecute_v1_Notification;

    public:
        explicit StartExecuteNotification(const Key& contract)
			: BaseSuperContractNotification(Notification_Type, sizeof(StartExecuteNotification<1>), contract)
        {}
    };

    /// Notification of an end execute super contract.
    template<VersionType version>
    struct EndExecuteNotification;

    template<>
    struct EndExecuteNotification<1> : public BaseSuperContractNotification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = SuperContract_EndExecute_v1_Notification;

    public:
        explicit EndExecuteNotification(const Key& contract)
			: BaseSuperContractNotification(Notification_Type, sizeof(EndExecuteNotification<1>), contract)
        {}
    };

    struct BaseChangeSuperContractStateNotification : public BaseSuperContractNotification {
    public:
        explicit BaseChangeSuperContractStateNotification(
			NotificationType type,
			size_t size,
			const Key& signer,
			const Key& superContract)
			: BaseSuperContractNotification(type, size, superContract)
			, Signer(signer)
        {}

    public:
		/// Signer.
		const Key& Signer;
    };

    /// Suspend super contract notification.
    template<VersionType version>
    struct SuspendNotification;

    template<>
    struct SuspendNotification<1> : public BaseChangeSuperContractStateNotification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = SuperContract_Suspend_v1_Notification;

    public:
        /// Creates a notification around \a signer and \a superContract.
        explicit SuspendNotification(const Key& signer, const Key& superContract)
			: BaseChangeSuperContractStateNotification(Notification_Type, sizeof(SuspendNotification<1>), signer, superContract)
        {}
    };

    /// Resume super contract notification.
    template<VersionType version>
    struct ResumeNotification;

    template<>
    struct ResumeNotification<1> : public BaseChangeSuperContractStateNotification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = SuperContract_Resume_v1_Notification;

    public:
        /// Creates a notification around \a signer and \a superContract.
        explicit ResumeNotification(const Key& signer, const Key& superContract)
			: BaseChangeSuperContractStateNotification(Notification_Type, sizeof(SuspendNotification<1>), signer, superContract)
        {}
    };
}}
