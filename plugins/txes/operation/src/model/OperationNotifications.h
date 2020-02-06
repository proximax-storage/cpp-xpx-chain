/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/lock_shared/src/model/LockNotifications.h"

namespace catapult { namespace model {

	// region operation notification types

/// Defines a operation notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_OPERATION_NOTIFICATION(CHANNEL, DESCRIPTION, CODE) DEFINE_NOTIFICATION_TYPE(CHANNEL, Operation, DESCRIPTION, CODE)

	/// Operation duration.
	DEFINE_OPERATION_NOTIFICATION(Validator, Duration_v1, 0x0001);

	/// Start operation.
	DEFINE_OPERATION_NOTIFICATION(All, Start_v1, 0x0002);

	/// End operation.
	DEFINE_OPERATION_NOTIFICATION(All, End_v1, 0x0003);

	/// Mosaic.
	DEFINE_OPERATION_NOTIFICATION(Validator, Mosaic_v1, 0x0004);

#undef DEFINE_OPERATION_NOTIFICATION

	// endregion

	/// Notification of an operation duration
	template<VersionType version>
	struct OperationDurationNotification;

	template<>
	struct OperationDurationNotification<1> : public BaseLockDurationNotification<OperationDurationNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Operation_Duration_v1_Notification;

	public:
		using BaseLockDurationNotification<OperationDurationNotification<1>>::BaseLockDurationNotification;
	};

	/// Notification of an operation start.
	template<VersionType version>
	struct StartOperationNotification;

	template<>
	struct StartOperationNotification<1> : public BaseLockNotification<StartOperationNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Operation_Start_v1_Notification;

	public:
		/// Creates start operation notification around \a operationToken, \a initiator, \a pExecutors, \a executorCount, \a pMosaics and \a mosaicCount.
		StartOperationNotification(
			const Hash256& operationToken,
			const Key& initiator,
			const Key* pExecutors,
			uint16_t executorCount,
			const UnresolvedMosaic* pMosaics,
			uint8_t mosaicCount,
			BlockDuration duration)
			: BaseLockNotification(initiator, pMosaics, mosaicCount, duration)
			, OperationToken(operationToken)
			, ExecutorsPtr(pExecutors)
			, ExecutorCount (executorCount)
		{}

	public:
		/// Operation token.
		Hash256 OperationToken;

		/// Operation executors.
		const Key* ExecutorsPtr;

		/// Operation executors count.
		uint16_t ExecutorCount;
	};

	/// Notification of an end operation notification.
	template<VersionType version>
	struct EndOperationNotification;

	template<>
	struct EndOperationNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Operation_End_v1_Notification;

	public:
		/// Creates end operation notification around \a executor, \a operationToken, \a pMosaics and \a mosaicCount.
		EndOperationNotification(
			const Key& executor,
			const Hash256& operationToken,
			const UnresolvedMosaic* pMosaics,
			uint8_t mosaicCount)
				: Notification(Notification_Type, sizeof(EndOperationNotification<1>))
				, Executor(executor)
				, OperationToken(operationToken)
				, MosaicsPtr(pMosaics)
				, MosaicCount(mosaicCount)
		{}

	public:
		/// Signer.
		const Key& Executor;

		/// OperationToken.
		Hash256 OperationToken;

		/// Spent mosaics.
		const UnresolvedMosaic* MosaicsPtr;

		/// Spent mosaics count.
		uint8_t MosaicCount;
	};

	/// Notification of an end operation notification.
	template<VersionType version>
	struct OperationMosaicNotification;

	template<>
	struct OperationMosaicNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Operation_Mosaic_v1_Notification;

	public:
		/// Creates end operation notification around \a pMosaics and \a mosaicCount.
		OperationMosaicNotification(
			const UnresolvedMosaic* pMosaics,
			uint8_t mosaicCount)
				: Notification(Notification_Type, sizeof(OperationMosaicNotification<1>))
				, MosaicsPtr(pMosaics)
				, MosaicCount(mosaicCount)
		{}

	public:
		/// Spent mosaics.
		const UnresolvedMosaic* MosaicsPtr;

		/// Spent mosaics count.
		uint8_t MosaicCount;
	};
}}
