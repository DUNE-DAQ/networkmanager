/**
 * @file NetworkManager_test.cxx NetworkManager class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "networkmanager/NetworkManager.hpp"
#include "networkmanager/networkmanager/Structs.hpp"

#define BOOST_TEST_MODULE NetworkManager_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::networkmanager;

BOOST_AUTO_TEST_SUITE(NetworkManager_test)

struct NetworkManagerSubscriberTestFixture
{
  NetworkManagerSubscriberTestFixture()
  {
    networkmanager::Conf testConfig;
    networkmanager::Connection testConn;
    testConn.name = "foo";
    testConn.address = "inproc://bar";
    testConn.type = networkmanager::Type::Subscriber;
    testConfig.connections.push_back(testConn);
    NetworkManager::get().configure(testConfig);
  }
  ~NetworkManagerSubscriberTestFixture() { NetworkManager::get().reset(); }
};

struct NetworkManagerReceiverTestFixture
{
  NetworkManagerReceiverTestFixture()
  {
    networkmanager::Conf testConfig;
    networkmanager::Connection testConn;
    testConn.name = "foo";
    testConn.address = "inproc://bar";
    testConn.type = networkmanager::Type::Receiver;
    testConfig.connections.push_back(testConn);
    NetworkManager::get().configure(testConfig);
  }
  ~NetworkManagerReceiverTestFixture() { NetworkManager::get().reset(); }
};

BOOST_AUTO_TEST_CASE(CopyAndMoveSemantics)
{
  BOOST_REQUIRE(!std::is_copy_constructible_v<NetworkManager>);
  BOOST_REQUIRE(!std::is_copy_assignable_v<NetworkManager>);
  BOOST_REQUIRE(!std::is_move_constructible_v<NetworkManager>);
  BOOST_REQUIRE(!std::is_move_assignable_v<NetworkManager>);
}

BOOST_AUTO_TEST_CASE(Singleton)
{
  auto& nm = NetworkManager::get();
  auto& another_nm = NetworkManager::get();

  BOOST_REQUIRE_EQUAL(&nm, &another_nm);
}

BOOST_FIXTURE_TEST_CASE(FakeConfigure, NetworkManagerReceiverTestFixture)
{
  BOOST_REQUIRE_EQUAL(NetworkManager::get().get_connection_string("foo"), "inproc://bar");
  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().get_connection_string("blahblah"),
                          ConnectionNotFound,
                          [&](ConnectionNotFound const&) { return true; });

  networkmanager::Conf testConfig;
  networkmanager::Connection testConn;
  testConn.name = "oof";
  testConn.address = "inproc://rab";
  testConn.type = networkmanager::Type::Receiver;
  testConfig.connections.push_back(testConn);
  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().configure(testConfig),
                          NetworkManagerAlreadyConfigured,
                          [&](NetworkManagerAlreadyConfigured const&) { return true; });

  NetworkManager::get().reset();

  NetworkManager::get().configure(testConfig);
  BOOST_REQUIRE_EQUAL(NetworkManager::get().get_connection_string("oof"), "inproc://rab");
  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().get_connection_string("foo"),
                          ConnectionNotFound,
                          [&](ConnectionNotFound const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(Listener, NetworkManagerReceiverTestFixture)
{
  BOOST_REQUIRE(!NetworkManager::get().has_listener("foo"));
  NetworkManager::get().add_listener("foo", [&](dunedaq::ipm::Receiver::Response) { return; });
  BOOST_REQUIRE(NetworkManager::get().has_listener("foo"));

  BOOST_REQUIRE(!NetworkManager::get().has_subscriber("foo", ""));

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().add_listener("foo", [&](dunedaq::ipm::Receiver::Response) { return; }),
                          ListenerAlreadyRegistered,
                          [&](ListenerAlreadyRegistered const&) { return true; });

  NetworkManager::get().remove_listener("foo");

  BOOST_REQUIRE(!NetworkManager::get().has_listener("bar"));
  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().add_listener("bar", [&](dunedaq::ipm::Receiver::Response) { return; }),
                          ConnectionNotFound,
                          [&](ConnectionNotFound const&) { return true; });
  BOOST_REQUIRE(!NetworkManager::get().has_listener("bar"));

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().remove_listener("foo"),
                          ListenerNotRegistered,
                          [&](ListenerNotRegistered const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().add_subscriber("foo", "bar", [&](dunedaq::ipm::Receiver::Response) { return; }),
                          ConnectionTypeMismatch,
                          [&](ConnectionTypeMismatch const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(Subscriber, NetworkManagerSubscriberTestFixture)
{
  BOOST_REQUIRE(!NetworkManager::get().has_subscriber("foo", "baz"));
  NetworkManager::get().add_subscriber("foo", "baz", [&](dunedaq::ipm::Receiver::Response) { return; });
  BOOST_REQUIRE(NetworkManager::get().has_subscriber("foo", "baz"));

  BOOST_REQUIRE(!NetworkManager::get().has_listener("foo"));

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().add_subscriber("foo", "baz", [&](dunedaq::ipm::Receiver::Response) { return; }),
                          SubscriberAlreadyRegistered,
                          [&](SubscriberAlreadyRegistered const&) { return true; });

  NetworkManager::get().add_subscriber("foo", "bar", [&](dunedaq::ipm::Receiver::Response) { return; });
  BOOST_REQUIRE(NetworkManager::get().has_subscriber("foo", "bar"));

  NetworkManager::get().remove_subscriber("foo", "bar");
  BOOST_REQUIRE(!NetworkManager::get().has_subscriber("foo", "bar"));
  BOOST_REQUIRE(NetworkManager::get().has_subscriber("foo", "baz"));

  NetworkManager::get().add_subscriber("foo", "bar", [&](dunedaq::ipm::Receiver::Response) { return; });
  BOOST_REQUIRE(NetworkManager::get().has_subscriber("foo", "bar"));

  BOOST_REQUIRE(!NetworkManager::get().has_subscriber("bar", "baz"));
  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().add_subscriber("bar", "baz", [&](dunedaq::ipm::Receiver::Response) { return; }),
                          ConnectionNotFound,
                          [&](ConnectionNotFound const&) { return true; });
  BOOST_REQUIRE(!NetworkManager::get().has_subscriber("bar", "baz"));

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().remove_subscriber("foo", "blah"),
                          SubscriberNotRegistered,
                          [&](SubscriberNotRegistered const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().add_listener("foo", [&](dunedaq::ipm::Receiver::Response) { return; }),
                          ConnectionTypeMismatch,
                          [&](ConnectionTypeMismatch const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(ReceiveFrom, NetworkManagerReceiverTestFixture)
{
  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().receive_from("foo", dunedaq::ipm::Receiver::s_no_block),
                          dunedaq::ipm::ReceiveTimeoutExpired,
                          [&](dunedaq::ipm::ReceiveTimeoutExpired const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().receive_from("bar", dunedaq::ipm::Receiver::s_no_block),
                          ConnectionNotFound,
                          [&](ConnectionNotFound const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().receive_from("foo", dunedaq::ipm::Receiver::s_no_block),
                          dunedaq::ipm::ReceiveTimeoutExpired,
                          [&](dunedaq::ipm::ReceiveTimeoutExpired const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(SendTo, NetworkManagerReceiverTestFixture)
{
  std::string sent_string;
  std::string received_string;

  std::function<void(dunedaq::ipm::Receiver::Response)> callback = [&](dunedaq::ipm::Receiver::Response response) {
    received_string = std::string(response.data.begin(), response.data.end());
  };
  NetworkManager::get().add_listener("foo", callback);

  received_string = "";
  sent_string = "this is the first test string";
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block);

  while (received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(received_string, sent_string);

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().send_to("bar", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block),
    ConnectionNotFound,
    [&](ConnectionNotFound const&) { return true; });

  sent_string = "this is another test string";
  received_string = "";
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block);

  while (received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(received_string, sent_string);
}

BOOST_FIXTURE_TEST_CASE(Publish, NetworkManagerSubscriberTestFixture)
{
  std::string sent_string;
  std::string received_string;

  std::function<void(dunedaq::ipm::Receiver::Response)> callback = [&](dunedaq::ipm::Receiver::Response response) {
    received_string = std::string(response.data.begin(), response.data.end());
  };
  NetworkManager::get().add_subscriber("foo", "bar", callback);

  received_string = "";
  sent_string = "this is the first test string";
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "bar");

  while (received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(received_string, sent_string);

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().send_to("bar", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block),
    ConnectionNotFound,
    [&](ConnectionNotFound const&) { return true; });

  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "baz");

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().receive_from("foo", dunedaq::ipm::Receiver::s_no_block, "bar"),
                          dunedaq::ipm::ReceiveTimeoutExpired,
                          [&](dunedaq::ipm::ReceiveTimeoutExpired const&) { return true; });

  sent_string = "this is another test string";
  received_string = "";
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "bar");

  while (received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(received_string, sent_string);
}

BOOST_AUTO_TEST_SUITE_END()
