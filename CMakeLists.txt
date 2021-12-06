cmake_minimum_required(VERSION 3.12)
project(networkmanager VERSION 1.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(Boost 1.70.0 COMPONENTS unit_test_framework program_options REQUIRED)

find_package(ipm REQUIRED)
find_package(logging REQUIRED)
find_package(utilities REQUIRED)

find_package(opmonlib REQUIRED)
find_package(ers REQUIRED)
find_package(nlohmann_json REQUIRED)


##############################################################################
# Schema

daq_codegen( nwmgr.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 )
daq_codegen( nwmgr*info.jsonnet DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2 )

##############################################################################
# Main library

daq_add_library(NetworkManager.cpp Listener.cpp LINK_LIBRARIES ipm::ipm utilities::utilities logging::logging opmonlib::opmonlib)

##############################################################################
# Unit tests
daq_add_unit_test(Listener_test LINK_LIBRARIES networkmanager)
daq_add_unit_test(NetworkManager_test LINK_LIBRARIES networkmanager)

daq_install()
