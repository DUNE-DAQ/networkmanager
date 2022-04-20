cmake_minimum_required(VERSION 3.12)
project(networkmanager VERSION 1.0.3)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(Boost 1.70.0 COMPONENTS unit_test_framework program_options REQUIRED)

find_package(ipm REQUIRED)
find_package(logging REQUIRED)
find_package(utilities REQUIRED)

find_package(opmonlib REQUIRED)
find_package(ers REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost COMPONENTS unit_test_framework REQUIRED)
find_package(serialization REQUIRED)

##############################################################################
# Schema

daq_codegen( nwmgr.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 )
daq_codegen( connectioninfo.jsonnet DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2 )

##############################################################################
# Main library

daq_add_library(NetworkManager.cpp Listener.cpp LINK_LIBRARIES ipm::ipm utilities::utilities logging::logging opmonlib::opmonlib serialization::serialization)

##############################################################################
# Unit tests
daq_add_unit_test(Listener_test LINK_LIBRARIES networkmanager)
daq_add_unit_test(NetworkManager_test LINK_LIBRARIES networkmanager)

daq_install()
