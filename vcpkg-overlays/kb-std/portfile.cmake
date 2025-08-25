
set(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../vendor/kb/kb-std-cpp")

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
            -DBUILD_TESTS=ON
            -DKB_STD_INSTALL=ON
)

# vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/kb-std)

vcpkg_cmake_install()