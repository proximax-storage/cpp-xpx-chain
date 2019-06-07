/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"

namespace catapult { namespace model {

	/// Plugin configuration base class.
	struct PluginConfiguration {
	public:
		virtual ~PluginConfiguration(){};

	public:
		/// Returns number of common plugin properties.
		static int CommonPropertyNumber();
	};
}}