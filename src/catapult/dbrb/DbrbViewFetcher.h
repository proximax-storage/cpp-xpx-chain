/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "DbrbDefinitions.h"
#include "catapult/utils/NonCopyable.h"

namespace catapult { namespace dbrb {

	class DbrbViewFetcher : public utils::NonCopyable {
	public:
		virtual ~DbrbViewFetcher() = default;

	public:
		/// Returns the latest registered view.
		virtual ViewData getView(Timestamp timestamp) const = 0;

		/// Returns the latest registered view.
		virtual Timestamp getExpirationTime(const ProcessId& processId) const = 0;

		/// Logs all known DBRB processes.
		virtual void logAllProcesses() const = 0;

		/// Logs \a view.
		virtual void logView(const dbrb::ViewData& view) const = 0;
	};
}}