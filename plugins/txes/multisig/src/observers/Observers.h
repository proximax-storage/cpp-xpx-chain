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
#include "src/model/MultisigNotifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by modify multisig cosigners notifications and:
	/// - adds / deletes multisig account to / from cache
	/// - adds / deletes cosignatories
	DECLARE_OBSERVER(ModifyMultisigCosigners, model::ModifyMultisigCosignersNotification<1>)();

	/// Observes changes triggered by modify multisig settings notifications and:
	/// - sets new values of min removal and min approval
	DECLARE_OBSERVER(ModifyMultisigSettings, model::ModifyMultisigSettingsNotification<1>)();
}}
