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

#include <src/catapult/validators/ValidatorContext.h>
#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::EntityNotification<1>;

	namespace {
		ValidationResult Validate(const Notification& notification, const ValidatorContext& context) {
			const auto& networkConfig = context.Config.Network;
			if(networkConfig.AccountVersion > 1 && notification.TransactionDerivationScheme == DerivationScheme::Unset)
				return Failure_Core_Derivation_Scheme_Unset;
			return ValidationResult::Success;
		}
	}
	DECLARE_STATEFUL_VALIDATOR(DerivationSchemeSpecification, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(DerivationSchemeSpecification, Validate);
	}
}}
