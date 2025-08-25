# Tell vcpkg to not download anything, but instead to use the source code from a local path.
vcpkg_from_local(
        SOURCE_PATH "${CURRENT_PORT_DIR}/../../../"
)

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()