cmake_minimum_required(VERSION 3.12)
project(networkmanager VERSION 1.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(ipm REQUIRED)
find_package(logging REQUIRED)
find_package(toolbox REQUIRED)

find_package(ers REQUIRED)
find_package(nlohmann_json REQUIRED)


##############################################################################
# Schema

daq_codegen(
  nwmgr.jsonnet
  TEMPLATES Structs.hpp.j2 Nljs.hpp.j2
)

##############################################################################
# Main library

daq_add_library(NetworkManager.cpp Listener.cpp Subscriber.cpp LINK_LIBRARIES ipm::ipm toolbox::toolbox logging::logging)

##############################################################################
# Unit tests
daq_add_unit_test(Listener_test LINK_LIBRARIES networkmanager)
daq_add_unit_test(NM_Subscriber_test LINK_LIBRARIES networkmanager)
daq_add_unit_test(NetworkManager_test LINK_LIBRARIES networkmanager)

daq_install()
