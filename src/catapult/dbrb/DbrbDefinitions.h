/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "MessageValidationResult.h"
#include "catapult/ionet/Packet.h"
#include "catapult/types.h"
#include "catapult/functions.h"
#include <set>

namespace catapult { namespace dbrb {

	using ProcessId = Key;
	using ViewData = std::set<ProcessId>;
	using DbrbTreeView = std::vector<ProcessId>;
	using Payload = std::shared_ptr<ionet::Packet>;
	using CertificateType = std::map<ProcessId, Signature>;

	constexpr size_t ProcessId_Size = Key_Size;

	using ValidationCallback = std::function<MessageValidationResult (const Payload&)>;
	using DeliverCallback = consumer<const Payload&>;
}}