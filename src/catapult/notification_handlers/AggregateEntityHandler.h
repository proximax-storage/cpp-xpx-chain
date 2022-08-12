/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "HandlerTypes.h"
#include "catapult/model/VerifiableEntity.h"
#include "catapult/utils/NamedObject.h"
#include <memory>

namespace catapult { namespace notification_handlers {

	/// An aggregate handler.
	template<typename... TArgs>
	class AggregateEntityHandlerT final {
	public:
		using HandlerVector = HandlerVectorT<TArgs...>;

	public:
		/// Creates an aggregate handler around \a handlers.
		explicit AggregateEntityHandlerT(HandlerVector&& handlers)
				: m_handlers(std::move(handlers))
		{}

	public:
		/// Helper for invoking curried handlers.
		class DispatchForwarder {
		public:
			/// Creates a forwarder around \a handlers.
			explicit DispatchForwarder(HandlerFunctions&& handlerFunctions)
					: m_handlerFunctions(std::move(handlerFunctions))
			{}

		public:
			/// Dispatches handle of \a entityInfos to \a dispatcher.
			template<typename TDispatcher>
			auto dispatch(const TDispatcher& dispatcher, const model::WeakEntityInfos& entityInfos) const {
				return dispatcher(entityInfos, m_handlerFunctions);
			}

		private:
			HandlerFunctions m_handlerFunctions;
		};

		/// Gets the names of all sub handlers.
		std::vector<std::string> names() const {
			return utils::ExtractNames(m_handlers);
		}

	private:
		HandlerVector m_handlers;
	};
}}
