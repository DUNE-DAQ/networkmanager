/**
 * @file Listener_test.cxx Listener class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "networkmanager/Listener.hpp"
#include "networkmanager/NetworkManager.hpp"
#include "networkmanager/networkmanager/Structs.hpp"

#define BOOST_TEST_MODULE Listener_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::networkmanager;

BOOST_AUTO_TEST_SUITE(Listener_test)

struct NetworkManagerTestFixture
{
  NetworkManagerTestFixture()
  {
    networkmanager::Conf testConfig;
    networkmanager::Connection testConn;
    testConn.name = "foo";
    testConn.address = "inproc://bar";
    testConn.type = networkmanager::Type::Receiver;
    testConfig.connections.push_back(testConn);
    NetworkManager::get().configure(testConfig);
  }
  ~NetworkManagerTestFixture() { NetworkManager::get().reset(); }
};

BOOST_AUTO_TEST_CASE(CopyAndMoveSemantics)
{
  BOOST_REQUIRE(!std::is_copy_constructible_v<Listener>);
  BOOST_REQUIRE(!std::is_copy_assignable_v<Listener>);
  BOOST_REQUIRE(std::is_move_constructible_v<Listener>);
  BOOST_REQUIRE(std::is_move_assignable_v<Listener>);
}

BOOST_FIXTURE_TEST_CASE(InitialConditions, NetworkManagerTestFixture)
{
  Listener l("foo");

  BOOST_REQUIRE_EQUAL(l.is_listening(), false);

  auto ll = std::move(l);

  BOOST_REQUIRE_EQUAL(ll.is_listening(), false);

  auto lll(std::move(ll));

  BOOST_REQUIRE_EQUAL(lll.is_listening(), false);
}

BOOST_FIXTURE_TEST_CASE(StartStop, NetworkManagerTestFixture)
{
  Listener l("foo");

  l.start_listening([&](dunedaq::ipm::Receiver::Response) {});

  BOOST_REQUIRE_EXCEPTION(l.start_listening([&](dunedaq::ipm::Receiver::Response) {}),
                          ListenerAlreadyRegistered,
                          [&](ListenerAlreadyRegistered const&) { return true; });

  BOOST_REQUIRE(l.is_listening());

  l.stop_listening();
  BOOST_REQUIRE(!l.is_listening());
  BOOST_REQUIRE_EXCEPTION(
    l.stop_listening(), ListenerNotRegistered, [&](ListenerNotRegistered const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(Shutdown, NetworkManagerTestFixture) {

  Listener l("foo");

  l.start_listening([&](dunedaq::ipm::Receiver::Response) {});

  BOOST_REQUIRE(l.is_listening());

  l.shutdown();
  BOOST_REQUIRE(!l.is_listening());
  BOOST_REQUIRE_EXCEPTION(
    l.stop_listening(), ListenerNotRegistered, [&](ListenerNotRegistered const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(Callback, NetworkManagerTestFixture) {
  std::string sent_string;
  std::string received_string;

  std::function<void(dunedaq::ipm::Receiver::Response)> callback = [&](dunedaq::ipm::Receiver::Response response) {
    received_string = std::string(response.data.begin(), response.data.end());
  };

  Listener l("foo");
  l.start_listening(callback);

  received_string = "";
  sent_string = "this is the first test string";
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block);

  while (received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(received_string, sent_string);
}

BOOST_AUTO_TEST_SUITE_END()
