/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(EndFileDownload, model::EndFileDownloadNotification<1>, ([](const auto& notification, ObserverContext& context) {
		ObserveDownloadNotification<model::EndFileDownloadNotification<1>, NotifyMode::Rollback>(notification, context);
	}));
}}
