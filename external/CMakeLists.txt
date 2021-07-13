cmake_minimum_required(VERSION 3.2)
project(external LANGUAGES C ASM)

file(GLOB external_ref10_SRC
	"ref10/*.cpp"
	"ref10/*.c"
	"ref10/*.h")
file(GLOB external_sha256_SRC
	"sha256/*.c"
	"sha256/*.h")
file(GLOB external_sha3_SRC
	"sha3/*.c"
	"sha3/*.h")
file(GLOB external_ripemd160_SRC
	"ripemd160/*.c"
	"ripemd160/*.h"
)
enable_language(ASM)
file(GLOB external_blst_SRC
	"blst/blst.h"
	"blst/blst.hpp"
	"blst/blst.aux"
	"blst/server.c"
	"blst/assembly.S"
)

SOURCE_GROUP("ref10" FILES ${external_ref10_SRC})
SOURCE_GROUP("sha256" FILES ${external_sha256_SRC})
SOURCE_GROUP("sha3" FILES ${external_sha3_SRC})
SOURCE_GROUP("ripemd160" FILES ${external_ripemd160_SRC})
SOURCE_GROUP("blst" FILES ${external_blst_SRC})

add_library(external ${external_ref10_SRC} ${external_sha256_SRC} ${external_sha3_SRC} ${external_ripemd160_SRC} ${external_blst_SRC})
set_property(TARGET external PROPERTY POSITION_INDEPENDENT_CODE ON)
