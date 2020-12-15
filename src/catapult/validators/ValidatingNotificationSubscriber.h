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
#include "AggregateValidationResult.h"
#include "ValidatorTypes.h"
#include "catapult/model/NotificationSubscriber.h"

namespace catapult { namespace validators {

	/// A notification subscriber that validates notifications.
	template<typename TValidator, typename... TArgs>
	class ValidatingNotificationSubscriber : public model::NotificationSubscriber {
	public:
		/// Creates a validating notification subscriber around \a validator.
		explicit ValidatingNotificationSubscriber(const TValidator& validator, const TArgs&... args)
				: m_validator(validator)
				, m_result(ValidationResult::Success)
				, m_args(args...)
		{}

	public:
		/// Gets the aggregate validation result.
		ValidationResult result() const {
			return m_result;
		}

	public:
		void notify(const model::Notification& notification) override {
			if (!IsSet(notification.Type, model::NotificationChannel::Validator))
				return;

			if (IsValidationResultFailure(m_result))
				return;

			auto result = validate(notification);
			AggregateValidationResult(m_result, result);
		}

	private:
		template<typename T = std::tuple<TArgs...>, std::enable_if_t<!std::tuple_size_v<T>, bool> = true>
		ValidationResult validate(const model::Notification& notification) {
			return m_validator.validate(notification);
		}

		template<typename T = std::tuple<TArgs...>, std::enable_if_t<std::tuple_size_v<T>, bool> = true>
		ValidationResult validate(const model::Notification& notification) {
			return m_validator.validate(notification, std::get<0>(m_args));
		}

	private:
		const TValidator& m_validator;
		ValidationResult m_result;
		std::tuple<TArgs...> m_args;
	};
}}
