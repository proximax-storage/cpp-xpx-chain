set(
    HUNTER_CACHE_SERVERS
    "https://github.com/elucideye/hunter-cache;https://github.com/ingenue/hunter-cache"
    CACHE
    STRING
    "Binary cache server"
)


include(${CMAKE_CURRENT_LIST_DIR}/HunterGate.cmake)
