/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include <type_traits>
#include <tuple>

namespace catapult { namespace utils {

	/// Applies an accumulator function (\a fun) against an initial value(\a initialValue) and
	/// each element of \a container to reduce it to a single value.
	template<typename TContainer, typename TInitial, typename TFunction>
	auto Reduce(const TContainer& container, TInitial initialValue, TFunction fun) {
		for (const auto& element : container)
			initialValue = fun(initialValue, element);

		return initialValue;
	}

	/// Applies \a accessor on each element of \a container and sums resulting values.
	/// \note this does not use Reduce in order to avoid creating another lambda.
	template<typename TContainer, typename TFunction>
	auto Sum(const TContainer& container, TFunction accessor) {
		std::invoke_result_t<TFunction, typename TContainer::value_type> sum = 0;

		for (const auto& element : container)
			sum += accessor(element);

		return sum;
	}
	/// Expands a tuple and repacks it's types as a tuple of containers, each containing elements of one of the tuple types.
	template<template<typename...> class TContainer, typename...>
	struct ExpandPackTo { };

	/// Expands a tuple and repacks it's types as a tuple of containers, each containing elements of one of the tuple types.
	template<template<typename...> class TContainer, typename... Ts>
	struct ExpandPackTo<TContainer, std::tuple<Ts...>>
	{
		using type = std::tuple<TContainer<Ts>...>;
	};

	template<typename TMapper, typename TTupleArg>
	auto make_tuple_unpack(TMapper map, TTupleArg args)
	{
		return std::apply([&map](auto& ...args){
		  return std::make_tuple(map(args)...);
		}, args);
	}

	template <typename T, T... S, typename F>
	constexpr void for_sequence(std::integer_sequence<T, S...>, F&& f) {
		using unpack_t = int[];
		(void)unpack_t{(static_cast<void>(f(std::integral_constant<T, S>{})), 0)..., 0};
	}

	template <typename> struct is_tuple: std::false_type {};

	template <typename ...T> struct is_tuple<std::tuple<T...>>: std::true_type {};

	template<typename T>
	static constexpr bool is_tuple_v = is_tuple<T>::value;
}}
