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

void
configure_network_manager(networkmanager::Type manager_type)
{
  networkmanager::Conf testConfig;
  networkmanager::Connection testConn;
  testConn.name = "foo";
  testConn.address = "inproc://bar";
  testConn.type = manager_type;
  testConfig.connections.push_back(testConn);
  NetworkManager::get().configure(testConfig);
}

BOOST_AUTO_TEST_CASE(CopyAndMoveSemantics)
{
  BOOST_REQUIRE(!std::is_copy_constructible_v<Listener>);
  BOOST_REQUIRE(!std::is_copy_assignable_v<Listener>);
  BOOST_REQUIRE(std::is_move_constructible_v<Listener>);
  BOOST_REQUIRE(std::is_move_assignable_v<Listener>);
}

BOOST_AUTO_TEST_CASE(InitialConditions)
{
  configure_network_manager(networkmanager::Type::Receiver);

  Listener l("foo", false);

  BOOST_REQUIRE_EQUAL(l.is_subscriber(), false);
  BOOST_REQUIRE_EQUAL(l.is_listening(), false);

  auto ll = std::move(l);

  BOOST_REQUIRE_EQUAL(ll.is_subscriber(), false);
  BOOST_REQUIRE_EQUAL(ll.is_listening(), false);

  auto lll(std::move(ll));

  BOOST_REQUIRE_EQUAL(lll.is_subscriber(), false);
  BOOST_REQUIRE_EQUAL(lll.is_listening(), false);

  NetworkManager::get().reset();

  configure_network_manager(networkmanager::Type::Subscriber);

  Listener llll("foo", true);

  BOOST_REQUIRE_EQUAL(llll.is_subscriber(), true);
  BOOST_REQUIRE_EQUAL(llll.is_listening(), false);

  auto l5 = new Listener("foo", true);

  BOOST_REQUIRE_EQUAL(l5->is_subscriber(), true);
  BOOST_REQUIRE_EQUAL(l5->is_listening(), false);

  delete l5;

  NetworkManager::get().reset();
}

BOOST_AUTO_TEST_SUITE_END()
