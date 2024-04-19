/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/dbrb/DbrbDefinitions.h"
#include <map>

namespace catapult { namespace dbrb {

	struct DbrbDoubleShard {
		static constexpr size_t MinShardSize = 4;

		bool Initialized = false;

		ProcessId Parent;
		ViewData Siblings;
		ViewData Children;
		ViewData Neighbours;

		ViewData ParentView;
		std::map<ProcessId, ViewData> SiblingViews;
		std::map<ProcessId, ViewData> ChildViews;
	};

	DbrbTreeView CreateDbrbTreeView(const ViewData& reachableNodes, const ViewData& unreachableNodes, const ProcessId& broadcaster, size_t shardSize);
	DbrbDoubleShard CreateDbrbShard(const DbrbTreeView& tree, const ProcessId& thisProcessId, size_t shardSize);
}}