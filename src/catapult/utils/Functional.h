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
#include "catapult/exceptions.h"
#include "boost/preprocessor.hpp"

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
	auto make_tuple_unpack(TMapper map, TTupleArg& args)
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

#define DEFINE_EXISTS_DECLARATION(NAME) \
	template<typename T, typename Enable = void> \
	struct ExistsDeclaration##NAME {      \
	static constexpr bool Value = false;         \
	};   \
                                 \
	template<typename T>\
	struct ExistsDeclaration##NAME<T, std::void_t<decltype(T::NAME)>> {\
		using Type = decltype(T::NAME);           \
		static constexpr bool Value = true;         \
	};
#define EXISTS_DECLARATION(NAME) ExistsDeclaration##NAME


/// Default tatic method is needed because contexpr still evaluates even if condition is never true
#define SUPPORTS_STATIC_DEF(METHOD_NAME) \
	template<typename TClass, typename T = void, typename Enable = void> \
	struct SupportsStaticMethod_##METHOD_NAME { \
	static constexpr bool Value = false;           \
	template<typename... TParams>              \
	static void Call(TParams&&... params) {\
		CATAPULT_THROW_RUNTIME_ERROR("Invalid execution!"); \
	}                                       \
	};                                          \
	template <typename TClass, typename T> \
	struct SupportsStaticMethod_##METHOD_NAME<TClass, T, typename std::enable_if<!std::is_member_pointer<decltype(&TClass::template METHOD_NAME<T>)>::value>::type> \
	{ \
	static constexpr bool Value = true;         \
	template<typename... TParams>              \
	static void Call(TParams&&... params)\
	{\
		TClass::METHOD_NAME(std::forward(params)...);\
	} \
	}; \
	template <typename TClass> \
	struct SupportsStaticMethod_##METHOD_NAME<TClass, void, typename std::enable_if<!std::is_member_pointer<decltype(&TClass::template METHOD_NAME)>::value>::type> \
	{ \
	static constexpr bool Value = true;       \
	template<typename... TParams>     \
	static void Call(TParams&&... params)\
	{\
		TClass::METHOD_NAME(std::forward<TParams>(params)...);\
	} \
	};

#define SUPPORTS_STATIC(METHOD_NAME) SupportsStaticMethod_##METHOD_NAME
#define IS_STATIC_SUPPORTED(METHOD_NAME, ...)SUPPORTS_STATIC(METHOD_NAME)<__VA_ARGS__>::Value
#define CALL_IF_STATIC_SUPPORTED(METHOD_NAME, ...) \
if constexpr(IS_STATIC_SUPPORTED(METHOD_NAME, __VA_ARGS__))                     \
	SUPPORTS_STATIC(METHOD_NAME)<__VA_ARGS__>::Call

// Generates a single element
#define CATAPULT_ENUM_ELEMENT(r, data, elem) elem,

// Generates a single element with X_CATAPULT function
#define CATAPULT_ENUM_XELEMENT(r, data, i, elem) CATAPULT_X_(data, elem, i)

// Map enum element to map element.
#define CATAPULT_MAP_ELEMENT(r, EnumType, elem) {BOOST_PP_STRINGIZE(elem), EnumType::elem},

// Map enum element to map element.
#define CATAPULT_RMAP_ELEMENT(r, EnumType, elem) {EnumType::elem, BOOST_PP_STRINGIZE(elem)},

// Declares an enum, generating the elements and the corresponding map
#define CATAPULT_DECLARE_ENUM(EnumName, EnumType, ...) \
    enum class EnumName : EnumType { \
        BOOST_PP_SEQ_FOR_EACH(CATAPULT_ENUM_ELEMENT, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
    }; \
    static const std::unordered_map<std::string, EnumName>& EnumName##_Map() { \
        static const std::unordered_map<std::string, EnumName> map = { \
            BOOST_PP_SEQ_FOR_EACH(CATAPULT_MAP_ELEMENT, EnumName, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
        }; \
        return map; \
    }\
	static const std::unordered_map<EnumName, std::string>& EnumName##_RMap() { \
		static const std::unordered_map<EnumName, std::string> map = { \
			BOOST_PP_SEQ_FOR_EACH(CATAPULT_RMAP_ELEMENT, EnumName, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
		}; \
		return map; \
	}

#define CATAPULT_ENUM_XLIST(...) \
	BOOST_PP_SEQ_FOR_EACH_I(CATAPULT_ENUM_XELEMENT, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
}}
#define CATAPULT_TO_SEQ(...) BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)

