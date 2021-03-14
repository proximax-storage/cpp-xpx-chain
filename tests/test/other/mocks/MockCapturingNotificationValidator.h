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
#include "MockNotificationValidator.h"
#include "catapult/validators/StatelessValidatorContext.h"
#include "catapult/validators/StatefulValidatorContext.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/nodeps/ParamsCapture.h"

namespace catapult { namespace mocks {

	/// Information about a captured transaction notification.
	struct CapturedTransactionNotificationInfo {
	public:
		/// Creates a captured transaction notification info around \a notification.
		explicit CapturedTransactionNotificationInfo(const model::Notification& notification) {
			IsSet = model::Core_Transaction_v1_Notification == notification.Type;
			if (IsSet) {
				const auto& transactionNotification = static_cast<const model::TransactionNotification<1>&>(notification);
				TransactionHash = transactionNotification.TransactionHash;
				Deadline = transactionNotification.Deadline;
			}
		}

	public:
		/// \c true if a transaction notification was captured.
		bool IsSet;

		/// Transaction hash.
		Hash256 TransactionHash;

		/// Transaction deadline.
		Timestamp Deadline;
	};

	/// Stateless notification validator params.
	struct StatelessNotificationValidatorParams {
	public:
		/// Creates params around \a notification.
		explicit StatelessNotificationValidatorParams(const model::Notification& notification)
				: NotificationPtr(&notification)
				, TransactionNotificationInfo(notification)
		{}

	public:
		/// Pointer to the notification.
		const model::Notification* NotificationPtr;

		/// Transaction notification information (if applicable).
		CapturedTransactionNotificationInfo TransactionNotificationInfo;
	};

	/// A mock stateless notification validator that captures parameters passed to validate.
	template<typename TNotification>
	class MockCapturingStatelessNotificationValidator
			: public mocks::MockStatelessNotificationValidatorT<TNotification>
			, public test::ParamsCapture<StatelessNotificationValidatorParams> {
	private:
		using BaseType = mocks::MockStatelessNotificationValidatorT<TNotification>;

	public:
		using BaseType::MockStatelessNotificationValidatorT;

	public:
		validators::ValidationResult validate(const TNotification& notification, const validators::StatelessValidatorContext& context) const override {
			const_cast<MockCapturingStatelessNotificationValidator*>(this)->push(notification);
			return BaseType::validate(notification, context);
		}
	};

	/// Stateful notification validator params.
	struct StatefulNotificationValidatorParams {
	public:
		/// Creates params around \a notification and \a context.
		explicit StatefulNotificationValidatorParams(const model::Notification& notification, const validators::StatefulValidatorContext& context)
				: NotificationPtr(&notification)
				, TransactionNotificationInfo(notification)
				, Height(context.Height)
				, BlockTime(context.BlockTime)
				, NetworkIdentifier(context.NetworkIdentifier)
				, IsMarkedCache(test::IsMarkedCache(context.Cache))
				, ResolvedMosaicIdOne(context.Resolvers.resolve(UnresolvedMosaicId(1)))
		{}

	public:
		/// Pointer to the notification.
		const model::Notification* NotificationPtr;

		/// Transaction notification information (if applicable).
		CapturedTransactionNotificationInfo TransactionNotificationInfo;

		/// Validation height.
		catapult::Height Height;

		/// Validation block time.
		Timestamp BlockTime;

		/// Validation network.
		model::NetworkIdentifier NetworkIdentifier;

		/// \c true if the validation cache is marked.
		bool IsMarkedCache;

		/// Resolved mosaic id for unresolved mosaic id `one`.
		MosaicId ResolvedMosaicIdOne;
	};

	/// A mock stateful notification validator that captures parameters passed to validate.
	template<typename TNotification>
	class MockCapturingStatefulNotificationValidator
			: public mocks::MockNotificationValidatorT<TNotification>
			, public test::ParamsCapture<StatefulNotificationValidatorParams> {
	private:
		using BaseType = mocks::MockNotificationValidatorT<TNotification>;

	public:
		using BaseType::MockNotificationValidatorT;

	public:
		validators::ValidationResult validate(
				const TNotification& notification,
				const validators::StatefulValidatorContext& context) const override {
			const_cast<MockCapturingStatefulNotificationValidator*>(this)->push(notification, context);
			return BaseType::validate(notification, context);
		}
	};
}}
