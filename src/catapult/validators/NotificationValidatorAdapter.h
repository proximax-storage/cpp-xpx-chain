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
#include "ValidatorTypes.h"
#include "catapult/model/NotificationPublisher.h"
#include "NotificationValidatorAdapter.h"
#include "ValidatingNotificationSubscriber.h"
#include "catapult/model/TransactionPlugin.h"
#include <utility>

namespace catapult { namespace model { class TransactionRegistry; } }

namespace catapult { namespace validators {

	/// A notification validator to entity validator adapter.
	template<typename... TArgs>
	class NotificationValidatorAdapter : public EntityValidatorT<TArgs...> {
	protected:
		using NotificationValidatorPointer = std::unique_ptr<const catapult::validators::NotificationValidatorT<model::Notification, TArgs...>>;
		using NotificationPublisherPointer = std::unique_ptr<const model::NotificationPublisher>;

	public:
		/// Creates a new adapter around \a pValidator and \a pPublisher.
		NotificationValidatorAdapter(NotificationValidatorPointer&& pValidator, NotificationPublisherPointer&& pPublisher)
			: m_pValidator(std::move(pValidator))
			, m_pPublisher(std::move(pPublisher))
		{}

	public:
		const std::string& name() const override {
			return m_pValidator->name();
		}

		ValidationResult validate(const model::WeakEntityInfo& entityInfo, TArgs&&... args) const override {
			ValidatingNotificationSubscriber<TArgs...> sub(*m_pValidator, args...);
			m_pPublisher->publish(entityInfo, sub);
			return sub.result();
		};

	private:
		NotificationValidatorPointer m_pValidator;
		NotificationPublisherPointer m_pPublisher;
	};

	/// A stateless notification validator to entity validator adapter.
	/// \note This adapter intentionally only supports stateless validators.
	class NotificationStatelessValidatorAdapter : public NotificationValidatorAdapter<const StatelessValidatorContext&> {
	public:
		/// Creates a new adapter around \a pValidator and \a pPublisher.
		NotificationStatelessValidatorAdapter(NotificationValidatorPointer&& pValidator, NotificationPublisherPointer&& pPublisher)
			: NotificationValidatorAdapter<const StatelessValidatorContext&>(
					  static_cast<NotificationValidatorPointer&&>(pValidator),
					  static_cast<NotificationPublisherPointer&&>(pPublisher)) {}
	};
}}
