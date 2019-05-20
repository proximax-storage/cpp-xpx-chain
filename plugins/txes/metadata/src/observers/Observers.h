/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/MetadataNotifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by modify address metadata value notifications and:
	/// - adds / deletes metadata to address to / from cache
	DECLARE_OBSERVER(AddressMetadataValueModification, model::ModifyAddressMetadataValueNotification_v1)();

	/// Observes changes triggered by modify mosaic metadata value notifications and:
	/// - adds / deletes metadata to mosaic to / from cache
	DECLARE_OBSERVER(MosaicMetadataValueModification, model::ModifyMosaicMetadataValueNotification_v1)();

	/// Observes changes triggered by modify namespace metadata value notifications and:
	/// - adds / deletes metadata to namespace to / from cache
	DECLARE_OBSERVER(NamespaceMetadataValueModification, model::ModifyNamespaceMetadataValueNotification_v1)();
}}
