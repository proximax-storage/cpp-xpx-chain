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
#include "catapult/model/Notifications.h"

namespace helper
{
	template <int... Is>
	struct index {};

	template <int N, int... Is>
	struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

	template <int... Is>
	struct gen_seq<0, Is...> : index<Is...> {};
}

namespace catapult { namespace validators {

	/// A notification subscriber that validates notifications.
	template<typename... TArgs>
	class ValidatingNotificationSubscriber : public model::NotificationSubscriber {
	public:
		/// Creates a validating notification subscriber around \a validator.
		explicit ValidatingNotificationSubscriber(
				const catapult::validators::NotificationValidatorT<model::Notification, TArgs...>& validator,
				TArgs&&... args)
				: m_validator(validator)
				, m_args(std::forward<TArgs>(args)...)
				, m_result(ValidationResult::Success)
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
		const catapult::validators::NotificationValidatorT<model::Notification, TArgs...>& m_validator;
		std::tuple<TArgs...> m_args;
		ValidationResult m_result;

		template <typename... Args, int... Is>
		ValidationResult func(const model::Notification& notification, std::tuple<Args...>& tup, helper::index<Is...>)
		{
			return m_validator.validate(notification, std::get<Is>(tup)...);
		}

		ValidationResult func(const model::Notification& notification, std::tuple<TArgs...>& tup) {
			return func(notification, tup, helper::gen_seq<sizeof...(TArgs)>{});
		}

		ValidationResult validate(const model::Notification& notification) {
			return func(notification, m_args);
		}
	};
}}
