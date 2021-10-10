/**
 * @file NetworkManager_test.cxx NetworkManager class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "networkmanager/NetworkManager.hpp"
#include "networkmanager/nwmgr/Structs.hpp"

#include "logging/Logging.hpp"

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
    nwmgr::Connections testConfig;
    nwmgr::Connection testConn;
    testConn.name = "foo";
    testConn.address = "inproc://bar";
    testConn.type = nwmgr::Type::Subscriber;
    testConfig.push_back(testConn);
    NetworkManager::get().configure(testConfig);
  }
  ~NetworkManagerSubscriberTestFixture() { NetworkManager::get().reset(); }
};

struct NetworkManagerReceiverTestFixture
{
  NetworkManagerReceiverTestFixture()
  {
    nwmgr::Connections testConfig;
    nwmgr::Connection testConn;
    testConn.name = "foo";
    testConn.address = "inproc://bar";
    testConn.type = nwmgr::Type::Receiver;
    testConfig.push_back(testConn);
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

  nwmgr::Connections testConfig;
  nwmgr::Connection testConn;
  testConn.name = "oof";
  testConn.address = "inproc://rab";
  testConn.type = nwmgr::Type::Receiver;
  testConfig.push_back(testConn);
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
  BOOST_REQUIRE(!NetworkManager::get().is_listening("foo"));
  NetworkManager::get().start_listening("foo", [&](dunedaq::ipm::Receiver::Response) { return; });
  BOOST_REQUIRE(NetworkManager::get().is_listening("foo"));

  BOOST_REQUIRE(!NetworkManager::get().has_subscriber("foo", ""));

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().start_listening("foo", [&](dunedaq::ipm::Receiver::Response) { return; }),
    ListenerAlreadyRegistered,
    [&](ListenerAlreadyRegistered const&) { return true; });

  NetworkManager::get().stop_listening("foo");

  BOOST_REQUIRE(!NetworkManager::get().is_listening("bar"));
  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().start_listening("bar", [&](dunedaq::ipm::Receiver::Response) { return; }),
    ConnectionNotFound,
    [&](ConnectionNotFound const&) { return true; });
  BOOST_REQUIRE(!NetworkManager::get().is_listening("bar"));

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().stop_listening("foo"),
                          ListenerNotRegistered,
                          [&](ListenerNotRegistered const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().add_subscriber("foo", "bar", [&](dunedaq::ipm::Receiver::Response) { return; }),
    ConnectionTypeMismatch,
    [&](ConnectionTypeMismatch const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(Subscriber, NetworkManagerSubscriberTestFixture)
{
  BOOST_REQUIRE(!NetworkManager::get().has_subscriber("foo", "baz"));
  NetworkManager::get().add_subscriber("foo", "baz", [&](dunedaq::ipm::Receiver::Response) { return; });
  BOOST_REQUIRE(NetworkManager::get().has_subscriber("foo", "baz"));

  BOOST_REQUIRE(!NetworkManager::get().is_listening("foo"));

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().add_subscriber("foo", "baz", [&](dunedaq::ipm::Receiver::Response) { return; }),
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
  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().add_subscriber("bar", "baz", [&](dunedaq::ipm::Receiver::Response) { return; }),
    ConnectionNotFound,
    [&](ConnectionNotFound const&) { return true; });
  BOOST_REQUIRE(!NetworkManager::get().has_subscriber("bar", "baz"));

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().remove_subscriber("foo", "blah"),
                          SubscriberNotRegistered,
                          [&](SubscriberNotRegistered const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().start_listening("foo", [&](dunedaq::ipm::Receiver::Response) { return; }),
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

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().receive_from("foo", dunedaq::ipm::Receiver::s_no_block, "bar"),
                          ConnectionTypeMismatch,
                          [&](ConnectionTypeMismatch const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(SendTo, NetworkManagerReceiverTestFixture)
{
  std::string sent_string;
  std::string received_string;

  std::function<void(dunedaq::ipm::Receiver::Response)> callback = [&](dunedaq::ipm::Receiver::Response response) {
    received_string = std::string(response.data.begin(), response.data.end());
  };
  NetworkManager::get().start_listening("foo", callback);

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

  BOOST_REQUIRE(!NetworkManager::get().is_connection_open("foo"));

  std::function<void(dunedaq::ipm::Receiver::Response)> callback = [&](dunedaq::ipm::Receiver::Response response) {
    received_string = std::string(response.data.begin(), response.data.end());
  };
  NetworkManager::get().add_subscriber("foo", "bar", callback);

  BOOST_REQUIRE(NetworkManager::get().is_connection_open("foo"));

  received_string = "";
  sent_string = "this is the first test string";
  BOOST_REQUIRE(!NetworkManager::get().is_connection_open("foo", NetworkManager::ConnectionDirection::Send));
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "bar");
  BOOST_REQUIRE(NetworkManager::get().is_connection_open("foo", NetworkManager::ConnectionDirection::Send));

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

  std::string received_string2;
  std::function<void(dunedaq::ipm::Receiver::Response)> callback2 = [&](dunedaq::ipm::Receiver::Response response) {
    received_string2 = std::string(response.data.begin(), response.data.end());
  };
  NetworkManager::get().add_subscriber("foo", "", callback2);

  sent_string = "this is a third test string";
  received_string = "";
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "bar");

  while (received_string == "") {
    usleep(1000);
  }
  BOOST_REQUIRE_EQUAL(received_string, sent_string);
  BOOST_REQUIRE_EQUAL(received_string2, sent_string);
}

BOOST_FIXTURE_TEST_CASE(SendThreadSafety, NetworkManagerReceiverTestFixture)
{
  TLOG_DEBUG(12) << "SendThreadSafety test case BEGIN";
  const std::string pattern_string =
    "aaaaabbbbbcccccdddddeeeeefffffggggghhhhhiiiiijjjjjkkkkklllllmmmmmnnnnnooooopppppqqqqqrrrrrssssstttttuuuuuvvvvvwwww"
    "wxxxxxyyyyyzzzzzAAAAABBBBBCCCCCDDDDDEEEEEFFFFFGGGGGHHHHHIIIIIJJJJJKKKKKLLLLLMMMMMNNNNNOOOOOPPPPPQQQQQRRRRRSSSSSTTT"
    "TTUUUUUVVVVVWWWWWXXXXXYYYYYZZZZZ";

  auto substr_proc = [&](int idx) {
    auto string_idx = idx % pattern_string.size();
    if (string_idx + 5 < pattern_string.size()) {
      return pattern_string.substr(string_idx, 5);
    } else {
      return pattern_string.substr(string_idx, 5) + pattern_string.substr(0, string_idx - pattern_string.size() + 5);
    }
  };

  auto send_proc = [&](int idx) {
    std::string buf = std::to_string(idx) + substr_proc(idx);
    TLOG_DEBUG(10) << "Sending " << buf << " for idx " << idx;
    NetworkManager::get().send_to("foo", buf.c_str(), buf.size(), dunedaq::ipm::Sender::s_block);
  };

  auto recv_proc = [&](dunedaq::ipm::Receiver::Response response) {
    BOOST_REQUIRE(response.data.size() > 0);
    auto received_idx = std::stoi(std::string(response.data.begin(), response.data.end()));
    auto idx_string = std::to_string(received_idx);
    auto received_string = std::string(response.data.begin() + idx_string.size(), response.data.end());

    TLOG_DEBUG(11) << "Received " << received_string << " for idx " << received_idx;

    BOOST_REQUIRE_EQUAL(received_string.size(), 5);

    std::string check = substr_proc(received_idx);

    BOOST_REQUIRE_EQUAL(received_string, check);
  };

  NetworkManager::get().start_listening("foo", recv_proc);

  const int thread_count = 1000;
  std::array<std::thread, thread_count> threads;

  TLOG_DEBUG(12) << "Before starting send threads";
  for (int idx = 0; idx < thread_count; ++idx) {
    threads[idx] = std::thread(send_proc, idx);
  }
  TLOG_DEBUG(12) << "After starting send threads";
  for (int idx = 0; idx < thread_count; ++idx) {
    threads[idx].join();
  }
  TLOG_DEBUG(12) << "SendThreadSafety test case END";
}

BOOST_FIXTURE_TEST_CASE(OneListenerThreaded, NetworkManagerReceiverTestFixture)
{
  auto callback = [&](dunedaq::ipm::Receiver::Response) { return; };
  const int thread_count = 1000;
  std::atomic<size_t> num_connected = 0;
  std::atomic<size_t> num_fail = 0;

  auto reg_proc = [&](int idx) {
    try {
      NetworkManager::get().start_listening("foo", callback);
    } catch (ListenerAlreadyRegistered const&) {
      num_fail++;
      TLOG_DEBUG(13) << "Listener " << idx << " failed to register";
      return;
    }
    TLOG_DEBUG(13) << "Listener " << idx << " successfully started";
    num_connected++;
  };

  std::array<std::thread, thread_count> threads;

  for (int idx = 0; idx < thread_count; ++idx) {
    threads[idx] = std::thread(reg_proc, idx);
  }
  for (int idx = 0; idx < thread_count; ++idx) {
    threads[idx].join();
  }

  BOOST_REQUIRE_EQUAL(num_connected.load(), 1);
  BOOST_REQUIRE_EQUAL(num_fail.load(), thread_count - 1);
}

BOOST_AUTO_TEST_CASE(ManyThreadsSendingAndReceiving)
{
  const int num_sending_threads = 100;
  const int num_receivers = 50;

  networkmanager::Conf testConfig;
  for (int i = 0; i < num_receivers; ++i) {
    networkmanager::Connection testConn;
    testConn.name = "foo" + std::to_string(i);
    testConn.address = "inproc://bar" + std::to_string(i);
    testConn.type = networkmanager::Type::Receiver;
    testConfig.connections.push_back(testConn);
  }
  NetworkManager::get().configure(testConfig);

  const std::string pattern_string =
    "aaaaabbbbbcccccdddddeeeeefffffggggghhhhhiiiiijjjjjkkkkklllllmmmmmnnnnnooooopppppqqqqqrrrrrssssstttttuuuuuvvvvvwwww"
    "wxxxxxyyyyyzzzzzAAAAABBBBBCCCCCDDDDDEEEEEFFFFFGGGGGHHHHHIIIIIJJJJJKKKKKLLLLLMMMMMNNNNNOOOOOPPPPPQQQQQRRRRRSSSSSTTT"
    "TTUUUUUVVVVVWWWWWXXXXXYYYYYZZZZZ";

  auto substr_proc = [&](int idx) {
    auto string_idx = idx % pattern_string.size();
    if (string_idx + 5 < pattern_string.size()) {
      return pattern_string.substr(string_idx, 5);
    } else {
      return pattern_string.substr(string_idx, 5) + pattern_string.substr(0, string_idx - pattern_string.size() + 5);
    }
  };
  auto send_proc = [&](int idx) {
    std::string buf = std::to_string(idx) + substr_proc(idx);
    for (int i = 0; i < num_receivers; ++i) {
      TLOG_DEBUG(14) << "Sending " << buf << " for idx " << idx << " to receiver " << i;
      NetworkManager::get().send_to("foo" + std::to_string(i), buf.c_str(), buf.size(), dunedaq::ipm::Sender::s_block);
    }
  };

  std::array<std::atomic<size_t>, num_receivers> messages_received;
  std::array<std::function<void(dunedaq::ipm::Receiver::Response)>, num_receivers> recv_procs;

  for (int i = 0; i < num_receivers; ++i) {
    messages_received[i] = 0;
    recv_procs[i] = [&,i](dunedaq::ipm::Receiver::Response response) {
      BOOST_REQUIRE(response.data.size() > 0);
      auto received_idx = std::stoi(std::string(response.data.begin(), response.data.end()));
      auto idx_string = std::to_string(received_idx);
      auto received_string = std::string(response.data.begin() + idx_string.size(), response.data.end());

      TLOG_DEBUG(14) << "Receiver " << i << " received " << received_string << " for idx " << received_idx;

      BOOST_REQUIRE_EQUAL(received_string.size(), 5);

      std::string check = substr_proc(received_idx);

      BOOST_REQUIRE_EQUAL(received_string, check);
      messages_received[i]++;
    };
    NetworkManager::get().start_listening("foo" + std::to_string(i), recv_procs[i]);
  }

  std::array<std::thread, num_sending_threads> threads;

  TLOG_DEBUG(14) << "Before starting send threads";
  for (int idx = 0; idx < num_sending_threads; ++idx) {
    threads[idx] = std::thread(send_proc, idx);
  }
  TLOG_DEBUG(14) << "After starting send threads";
  for (int idx = 0; idx < num_sending_threads; ++idx) {
    threads[idx].join();
  }

  NetworkManager::get().reset();

  for (auto i = 0; i < num_receivers; ++i) {
    BOOST_REQUIRE_EQUAL(messages_received[i], num_sending_threads);
  }
}

BOOST_AUTO_TEST_SUITE_END()
