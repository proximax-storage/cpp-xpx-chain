/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include <memory>

namespace catapult { namespace utils {

	/// Class that allows to override the clone method of\aBase class by \a Derived class, which return \a ReturnCloneType
	template <class Derived, class Base, class ReturnCloneType>
	class Cloneable : public Base {
	public:
		using Base::Base;
		std::unique_ptr<ReturnCloneType> clone() const override {
			return std::make_unique<Derived>(reinterpret_cast<Derived const&>(*this));
		}
	};
}}
