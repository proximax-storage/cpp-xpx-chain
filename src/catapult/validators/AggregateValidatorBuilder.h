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
#include "catapult/utils/NamedObject.h"
#include <utility>
#include <vector>

namespace catapult { namespace validators {

	/// A strongly typed aggregate notification validator builder.
	template<typename TNotification, typename... TArgs>
	class AggregateValidatorBuilder {
	private:
		using NotificationValidatorPointer = std::unique_ptr<const NotificationValidatorT<TNotification, TArgs...>>;
		using NotificationValidatorPointerMap = std::map<model::NotificationType, std::vector<NotificationValidatorPointer>>;
		using AggregateValidatorPointer = std::unique_ptr<const AggregateNotificationValidatorT<TNotification, TArgs...>>;

	public:
		/// Adds \a pValidator to the builder and allows chaining.
		AggregateValidatorBuilder& add(model::NotificationType type, NotificationValidatorPointer&& pValidator) {
			m_validators[type].push_back(std::move(pValidator));
			return *this;
		}

		/// Builds a strongly typed notification validator that ignores suppressed failures according to \a isSuppressedFailure.
		AggregateValidatorPointer build(const ValidationResultPredicate& isSuppressedFailure) {
			return std::make_unique<DefaultAggregateNotificationValidator>(std::move(m_validators), isSuppressedFailure);
		}

	private:
		class DefaultAggregateNotificationValidator : public AggregateNotificationValidatorT<TNotification, TArgs...> {
		public:
			explicit DefaultAggregateNotificationValidator(
					NotificationValidatorPointerMap&& validators,
					ValidationResultPredicate isSuppressedFailure)
					: m_validators(std::move(validators))
					, m_isSuppressedFailure(std::move(isSuppressedFailure))
					, m_name(utils::ReduceNames(utils::ExtractNames(m_validators)))
			{}

		public:
			const std::string& name() const override {
				return m_name;
			}

			std::vector<std::string> names() const override {
				return utils::ExtractNames(m_validators);
			}

			ValidationResult validate(const TNotification& notification, TArgs&&... args) const override {
				auto aggregateResult = ValidationResult::Success;

				auto validatorIter = m_validators.find(notification.Type);
				if (m_validators.end() == validatorIter)
					return aggregateResult;

				for (const auto& pValidator : validatorIter->second) {
					auto result = pValidator->validate(notification, std::forward<TArgs>(args)...);

					// ignore suppressed failures
					if (m_isSuppressedFailure(result))
						continue;

					// exit on other failures
					if (IsValidationResultFailure(result))
						return result;

					AggregateValidationResult(aggregateResult, result);
				}

				return aggregateResult;
			}

		private:
			NotificationValidatorPointerMap m_validators;
			ValidationResultPredicate m_isSuppressedFailure;
			std::string m_name;
		};

	private:
		NotificationValidatorPointerMap m_validators;
	};
}}
