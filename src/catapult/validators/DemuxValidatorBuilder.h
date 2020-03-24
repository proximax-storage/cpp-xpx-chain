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
#include "AggregateValidatorBuilder.h"
#include "ValidatorTypes.h"
#include <functional>
#include <utility>
#include <vector>

namespace catapult { namespace validators {

	/// A demultiplexing validator builder.
	template<typename... TArgs>
	class DemuxValidatorBuilderT {
	private:
		template<typename TNotification>
		using NotificationValidatorPointerT = std::unique_ptr<const NotificationValidatorT<TNotification, TArgs...>>;
		using AggregateValidatorPointer = std::unique_ptr<const AggregateNotificationValidatorT<model::Notification, TArgs...>>;

	public:
		/// Adds a validator (\a pValidator) to the builder that is invoked only when matching notifications are processed.
		template<typename TNotification>
		DemuxValidatorBuilderT& add(NotificationValidatorPointerT<TNotification>&& pValidator) {
			m_builder.add(TNotification::Notification_Type, std::make_unique<NotificationValidatorAdapter<TNotification>>(std::move(pValidator)));
			return *this;
		}

		/// Builds a demultiplexing validator that ignores suppressed failures according to \a isSuppressedFailure.
		AggregateValidatorPointer build(const ValidationResultPredicate& isSuppressedFailure) {
			return m_builder.build(isSuppressedFailure);
		}

	private:
		template<typename TNotification>
		class NotificationValidatorAdapter : public NotificationValidatorT<model::Notification, TArgs...> {
		public:
			NotificationValidatorAdapter(NotificationValidatorPointerT<TNotification>&& pValidator)
				: m_pValidator(std::move(pValidator))
			{}

		public:
			const std::string& name() const override {
				return m_pValidator->name();
			}

			ValidationResult validate(const model::Notification& notification, TArgs&&... args) const override {
				return m_pValidator->validate(static_cast<const TNotification&>(notification), std::forward<TArgs>(args)...);
			}

		private:
			NotificationValidatorPointerT<TNotification> m_pValidator;
		};

	private:
		AggregateValidatorBuilder<model::Notification, TArgs...> m_builder;
	};
}}
