cmake_minimum_required(VERSION 3.2)

catapult_define_extension_test(partialtransaction api chain handlers test)

target_link_libraries(tests.catapult.partialtransaction catapult.plugins.multisig)
