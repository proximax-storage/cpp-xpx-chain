/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"
#include "catapult/dbrb/DbrbUtils.h"

namespace catapult { namespace model {

	DEFINE_NOTIFICATION_TYPE(All, Dbrb, AddDbrbProcess_v1, 0x0001);

	DEFINE_NOTIFICATION_TYPE(All, Dbrb, RemoveDbrbProcess_v1, 0x0002);

	template<VersionType version>
	struct AddDbrbProcessNotification;

	template<>
	struct AddDbrbProcessNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = Dbrb_AddDbrbProcess_v1_Notification;

	public:
		explicit AddDbrbProcessNotification(const dbrb::ProcessId& processId)
			: Notification(Notification_Type, sizeof(AddDbrbProcessNotification<1>))
			, ProcessId(processId)
		{}

	public:
		/// The ID of the process to add.
		dbrb::ProcessId ProcessId;
	};

	template<VersionType version>
	struct RemoveDbrbProcessNotification;

	template<>
	struct RemoveDbrbProcessNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = Dbrb_RemoveDbrbProcess_v1_Notification;

	public:
		explicit RemoveDbrbProcessNotification(const dbrb::ProcessId& processId)
			: Notification(Notification_Type, sizeof(RemoveDbrbProcessNotification<1>))
			, ProcessId(processId)
		{}

	public:
		/// The ID of the process to remove.
		dbrb::ProcessId ProcessId;
	};
}}