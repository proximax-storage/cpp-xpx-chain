cmake_minimum_required(VERSION 3.2)

catapult_define_extension_test(sync)

# DoubleSpend* tests from sync depend on this lib in run time
add_dependencies(tests.catapult.sync catapult.plugins.transfer)
