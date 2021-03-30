/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/WeakEntityInfo.h"
#include <string>

namespace catapult { namespace notification_handlers {

	/// A weakly typed entity handler.
	/// \note This is intended to be used only for stateless validation.
	template<typename... TArgs>
	class EntityHandlerT {
	public:
		virtual ~EntityHandlerT() = default;

	public:
		/// Gets the handler name.
		virtual const std::string& name() const = 0;

		/// Handles a single \a entityInfo with contextual information \a args.
		virtual void handle(const model::WeakEntityInfo& entityInfo, TArgs&&... args) const = 0;
	};
}}
