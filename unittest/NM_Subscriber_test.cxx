/**
 * @file NM_Subscriber_test.cxx NetworkManager Subscriber class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "networkmanager/Subscriber.hpp"
#include "networkmanager/NetworkManager.hpp"
#include "networkmanager/networkmanager/Structs.hpp"

#define BOOST_TEST_MODULE Subscriber_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::networkmanager;

BOOST_AUTO_TEST_SUITE(Subscriber_test)

struct NetworkManagerTestFixture
{
  NetworkManagerTestFixture()
  {
    networkmanager::Conf testConfig;
    networkmanager::Connection testConn;
    testConn.name = "foo";
    testConn.address = "inproc://bar";
    testConn.type = networkmanager::Type::Subscriber;
    testConfig.connections.push_back(testConn);
    NetworkManager::get().configure(testConfig);
  }
  ~NetworkManagerTestFixture() { NetworkManager::get().reset(); }
};

BOOST_AUTO_TEST_CASE(CopyAndMoveSemantics)
{
  BOOST_REQUIRE(!std::is_copy_constructible_v<Subscriber>);
  BOOST_REQUIRE(!std::is_copy_assignable_v<Subscriber>);
  BOOST_REQUIRE(std::is_move_constructible_v<Subscriber>);
  BOOST_REQUIRE(std::is_move_assignable_v<Subscriber>);
}

BOOST_FIXTURE_TEST_CASE(InitialConditions, NetworkManagerTestFixture)
{
  Subscriber s("foo");

  BOOST_REQUIRE(!s.is_running());
  BOOST_REQUIRE_EQUAL(s.num_callbacks(), 0);

  auto ss = std::move(s);

  BOOST_REQUIRE(!ss.is_running());

  auto sss(std::move(ss));

  BOOST_REQUIRE(!sss.is_running());
}

BOOST_FIXTURE_TEST_CASE(AddRemove, NetworkManagerTestFixture)
{
  Subscriber s("foo");

  s.add_callback([&](dunedaq::ipm::Receiver::Response) { return; }, "testTopic");
  s.add_callback([&](dunedaq::ipm::Receiver::Response) { return; }, "anotherTestTopic");
  s.add_callback([&](dunedaq::ipm::Receiver::Response) { return; }, "");

  auto topics = s.topics();
  BOOST_REQUIRE_EQUAL(topics.size(), 3);
  BOOST_REQUIRE(topics.count(""));
  BOOST_REQUIRE(topics.count("testTopic"));

  BOOST_REQUIRE_EQUAL(s.num_callbacks(), 3);
  BOOST_REQUIRE(s.is_running());

  BOOST_REQUIRE_EXCEPTION(s.add_callback([&](dunedaq::ipm::Receiver::Response) { return; }, "testTopic"),
                          CallbackAlreadyRegistered,
                          [&](CallbackAlreadyRegistered const&) { return true; });

  s.remove_callback("testTopic");
  BOOST_REQUIRE(s.is_running());
  BOOST_REQUIRE_EQUAL(s.num_callbacks(), 2);
  BOOST_REQUIRE_EXCEPTION(s.remove_callback("testTopic"), CallbackNotRegistered, [&](CallbackNotRegistered const&){ return true; });
  BOOST_REQUIRE(s.is_running());
  BOOST_REQUIRE_EQUAL(s.num_callbacks(), 2);

  s.remove_callback("");
  BOOST_REQUIRE(s.is_running());
  BOOST_REQUIRE_EQUAL(s.num_callbacks(), 1);

  s.remove_callback("anotherTestTopic");
  BOOST_REQUIRE(!s.is_running());
  BOOST_REQUIRE_EQUAL(s.num_callbacks(), 0);
}

BOOST_FIXTURE_TEST_CASE(Shutdown, NetworkManagerTestFixture) {

  Subscriber s("foo");

  s.add_callback([&](dunedaq::ipm::Receiver::Response) { return; }, "testTopic");

  BOOST_REQUIRE_EQUAL(s.num_callbacks(), 1);
  BOOST_REQUIRE(s.is_running());

  s.shutdown();
  BOOST_REQUIRE(!s.is_running());
  BOOST_REQUIRE_EQUAL(s.num_callbacks(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
