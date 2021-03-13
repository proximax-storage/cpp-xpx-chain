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
#include <utility>

#include "catapult/utils/SpinLock.h"
#include "catapult/validators/ValidatorTypes.h"
#include "tests/test/nodeps/AtomicVector.h"

namespace catapult { namespace mocks {

	/// Base of mock notification validators.
	class BasicMockNotificationValidator {
	protected:
		/// Creates a mock validator.
		BasicMockNotificationValidator()
				: m_result(validators::ValidationResult::Success)
				, m_triggerOnSpecificType(false)
		{}

	public:
		/// Returns collected notification types.
		const auto& notificationTypes() const {
			return m_notificationTypes;
		}

	public:
		/// Gets the result for a notification with the specified type (\a notificationType).
		validators::ValidationResult getResultForType(model::NotificationType notificationType) const {
			m_notificationTypes.push_back(notificationType);
			return m_triggerOnSpecificType && m_triggerType != notificationType
					? validators::ValidationResult::Success
					: (validators::ValidationResult)m_result;
		}

	public:
		/// Sets the result of validate to \a result.
		void setResult(validators::ValidationResult result) {
			m_result = result;
		}

		/// Sets the result of validate for a specific notification type (\a triggerType) to \a result.
		void setResult(validators::ValidationResult result, model::NotificationType triggerType) {
			setResult(result);
			m_triggerOnSpecificType = true;
			m_triggerType = triggerType;
		}

	private:
		std::atomic<validators::ValidationResult> m_result;
		bool m_triggerOnSpecificType;
		model::NotificationType m_triggerType;
		mutable test::AtomicVector<model::NotificationType> m_notificationTypes;
	};

	/// Mock stateless notification validator that captures information about observed notifications.
	template<typename TNotification>
	class MockStatelessNotificationValidatorT
			: public BasicMockNotificationValidator
			, public validators::stateless::NotificationValidatorT<TNotification> {
	public:
		/// Creates a mock validator with a default name.
		MockStatelessNotificationValidatorT() : MockStatelessNotificationValidatorT("MockStatelessNotificationValidatorT")
		{}

		/// Creates a mock validator with \a name.
		explicit MockStatelessNotificationValidatorT(std::string name) : m_name(std::move(name))
		{}

	public:
		const std::string& name() const override {
			return m_name;
		}

		validators::ValidationResult validate(const TNotification& notification, const validators::StatelessValidatorContext&) const override {
			// stateless validators need to be threadsafe, so guard getResultForType, which is not
			std::lock_guard<std::mutex> guard(m_mutex);
			return getResultForType(notification.Type);
		}

	private:
		std::string m_name;
		mutable std::mutex m_mutex;
	};

	/// Mock stateful notification validator that captures information about observed notifications and contexts.
	template<typename TNotification>
	class MockNotificationValidatorT
			: public BasicMockNotificationValidator
			, public validators::stateful::NotificationValidatorT<TNotification> {
	public:
		/// Creates a mock validator with a default name.
		MockNotificationValidatorT() : MockNotificationValidatorT("MockNotificationValidatorT")
		{}

		/// Creates a mock validator with \a name.
		explicit MockNotificationValidatorT(std::string name) : m_name(std::move(name))
		{}

	public:
		const std::string& name() const override {
			return m_name;
		}

		validators::ValidationResult validate(
				const TNotification& notification,
				const validators::StatefulValidatorContext& context) const override {
			m_contextPointers.push_back(&context);
			return getResultForType(notification.Type);
		}

	public:
		/// Returns collected context pointers.
		const auto& contextPointers() const {
			return m_contextPointers;
		}

	private:
		std::string m_name;
		mutable std::vector<const validators::StatefulValidatorContext*> m_contextPointers;
	};

	using MockNotificationValidator = MockNotificationValidatorT<model::Notification>;
}}
