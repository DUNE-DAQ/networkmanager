/**
 * @file Listener_test.cxx Listener class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "networkmanager/Listener.hpp"
#include "networkmanager/NetworkManager.hpp"
#include "networkmanager/nwmgr/Structs.hpp"

#include "logging/Logging.hpp"

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
    nwmgr::Connections testConfig;
    nwmgr::Connection testConn;
    testConn.name = "foo";
    testConn.address = "inproc://bar";
    testConfig.push_back(testConn);
    NetworkManager::get().configure(testConfig);
  }
  ~NetworkManagerTestFixture() { NetworkManager::get().reset(); }
};

struct NetworkManagerSubscriptionTextFixture
{
  NetworkManagerSubscriptionTextFixture()
  {

    nwmgr::Connections testConfig;
    nwmgr::Connection testConn;
    testConn.name = "foo";
    testConn.address = "inproc://foo";
    testConn.topics = { "bar", "baz" };
    testConfig.push_back(testConn);
    testConn.name = "oof";
    testConn.address = "inproc://oof";
    testConn.topics = { "bar", "qui" };
    testConfig.push_back(testConn);
    NetworkManager::get().configure(testConfig);

    NetworkManager::get().start_publisher("foo");
    NetworkManager::get().start_publisher("oof");
  }
  ~NetworkManagerSubscriptionTextFixture() { NetworkManager::get().reset(); }
};

BOOST_AUTO_TEST_CASE(CopyAndMoveSemantics)
{
  TLOG() << "CopyAndMoveSemantics test case BEGIN";
  BOOST_REQUIRE(!std::is_copy_constructible_v<Listener>);
  BOOST_REQUIRE(!std::is_copy_assignable_v<Listener>);
  BOOST_REQUIRE(std::is_move_constructible_v<Listener>);
  BOOST_REQUIRE(std::is_move_assignable_v<Listener>);
  TLOG() << "CopyAndMoveSemantics test case END";
}

BOOST_FIXTURE_TEST_CASE(InitialConditions, NetworkManagerTestFixture)
{
  TLOG() << "InitialConditions test case BEGIN";
  Listener l("foo");

  BOOST_REQUIRE_EQUAL(l.is_listening(), false);

  auto ll = std::move(l);

  BOOST_REQUIRE_EQUAL(ll.is_listening(), false);

  auto lll(std::move(ll));

  BOOST_REQUIRE_EQUAL(lll.is_listening(), false);
  TLOG() << "InitialConditions test case END";
}

BOOST_FIXTURE_TEST_CASE(StartStop, NetworkManagerTestFixture)
{
  TLOG() << "StartStop test case BEGIN";
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
  TLOG() << "StartStop test case END";
}

BOOST_FIXTURE_TEST_CASE(Shutdown, NetworkManagerTestFixture)
{
  TLOG() << "Shutdown test case BEGIN";

  Listener l("foo");

  l.start_listening([&](dunedaq::ipm::Receiver::Response) {});

  BOOST_REQUIRE(l.is_listening());

  l.shutdown();
  BOOST_REQUIRE(!l.is_listening());
  BOOST_REQUIRE_EXCEPTION(
    l.stop_listening(), ListenerNotRegistered, [&](ListenerNotRegistered const&) { return true; });
  TLOG() << "Shutdown test case END";
}

BOOST_FIXTURE_TEST_CASE(Callback, NetworkManagerTestFixture)
{
  TLOG() << "Callback test case BEGIN";
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
  TLOG() << "Callback test case END";
}

BOOST_FIXTURE_TEST_CASE(Subscriptions, NetworkManagerSubscriptionTextFixture)
{
  TLOG() << "Subscriptions test case BEGIN";
  std::string sent_string;
  std::string received_string;

  std::function<void(dunedaq::ipm::Receiver::Response)> callback = [&](dunedaq::ipm::Receiver::Response response) {
    received_string = std::string(response.data.begin(), response.data.end());
  };

  TLOG() << "Starting bar listener";
  Listener l("bar");
  l.start_listening(callback);

  TLOG() << "Sending first message";
  received_string = "";
  sent_string = "this is the first test string";
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "bar");

  TLOG() << "Waiting for first response";
  while (received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(received_string, sent_string);

  TLOG() << "Sending second message";
  received_string = "";
  sent_string = "this is the second test string";
  NetworkManager::get().send_to("oof", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "bar");

  TLOG() << "Waiting for second response";
  while (received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(received_string, sent_string);

  TLOG() << "Starting baz listener";
  Listener ll("baz");
  std::string another_received_string;

  std::function<void(dunedaq::ipm::Receiver::Response)> another_callback =
    [&](dunedaq::ipm::Receiver::Response response) {
      another_received_string = std::string(response.data.begin(), response.data.end());
    };
  ll.start_listening(another_callback);

  TLOG() << "Sending third message";
  received_string = "";
  another_received_string = "";
  sent_string = "this is the third test string";
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "baz");

  TLOG() << "Waiting for third response";
  while (another_received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(another_received_string, sent_string);
  BOOST_REQUIRE_EQUAL(received_string, "");

  TLOG() << "Sending fourth message";
  received_string = "";
  another_received_string = "";
  sent_string = "this is the fourth test string";
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "qui");

  TLOG() << "Waiting 1 second";
  std::this_thread::sleep_for(std::chrono::seconds(1));

  BOOST_REQUIRE_EQUAL(another_received_string, "");
  BOOST_REQUIRE_EQUAL(received_string, "");
  TLOG() << "Subscriptions test case END";
}

BOOST_AUTO_TEST_SUITE_END()
