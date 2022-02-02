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

struct NetworkManagerTestFixture
{
  NetworkManagerTestFixture()
  {
    nwmgr::Connections testConfig;
    nwmgr::Connection testConn;
    testConn.name = "foo";
    testConn.address = "inproc://foo";
    testConfig.push_back(testConn);
    testConn.name = "bar";
    testConn.address = "inproc://bar";
    testConn.topics = { "bax", "bay", "baz" };
    testConfig.push_back(testConn);
    testConn.name = "rab";
    testConn.address = "inproc://rab";
    testConn.topics = { "bav", "baw", "baz" };
    testConfig.push_back(testConn);
    testConn.name = "abr";
    testConn.address = "inproc://abr";
    testConn.topics = { "bau", "bav", "bax" };
    testConfig.push_back(testConn);
    NetworkManager::get().configure(testConfig);

    NetworkManager::get().start_publisher("bar");
    NetworkManager::get().start_publisher("rab");
    NetworkManager::get().start_publisher("abr");
  }
  ~NetworkManagerTestFixture() { NetworkManager::get().reset(); }

  NetworkManagerTestFixture(NetworkManagerTestFixture const&) = default;
  NetworkManagerTestFixture(NetworkManagerTestFixture&&) = default;
  NetworkManagerTestFixture& operator=(NetworkManagerTestFixture const&) = default;
  NetworkManagerTestFixture& operator=(NetworkManagerTestFixture&&) = default;
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

BOOST_FIXTURE_TEST_CASE(FakeConfigure, NetworkManagerTestFixture)
{
  BOOST_REQUIRE_EQUAL(NetworkManager::get().get_connection_string("foo"), "inproc://foo");

  auto strings = NetworkManager::get().get_connection_strings("baz");
  BOOST_REQUIRE_EQUAL(strings.size(), 2);
  BOOST_REQUIRE(strings[0] == "inproc://bar" || strings[1] == "inproc://bar");
  BOOST_REQUIRE(strings[0] == "inproc://rab" || strings[1] == "inproc://rab");

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().get_connection_string("blahblah"),
                          ConnectionNotFound,
                          [&](ConnectionNotFound const&) { return true; });
  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().get_connection_strings("foo"), TopicNotFound, [&](TopicNotFound const&) { return true; });

  BOOST_REQUIRE(NetworkManager::get().is_connection("foo"));
  BOOST_REQUIRE(NetworkManager::get().is_connection("bar"));
  BOOST_REQUIRE(!NetworkManager::get().is_connection("baz"));
  BOOST_REQUIRE(!NetworkManager::get().is_connection("unknown_connection"));
  BOOST_REQUIRE(!NetworkManager::get().is_connection("unknown_topic"));

  BOOST_REQUIRE(!NetworkManager::get().is_topic("foo"));
  BOOST_REQUIRE(!NetworkManager::get().is_topic("bar"));
  BOOST_REQUIRE(NetworkManager::get().is_topic("baz"));
  BOOST_REQUIRE(!NetworkManager::get().is_topic("unknown_connection"));
  BOOST_REQUIRE(!NetworkManager::get().is_topic("unknown_topic"));

  BOOST_REQUIRE(!NetworkManager::get().is_pubsub_connection("foo"));
  BOOST_REQUIRE(NetworkManager::get().is_pubsub_connection("bar"));
  BOOST_REQUIRE(!NetworkManager::get().is_pubsub_connection("baz"));
  BOOST_REQUIRE(!NetworkManager::get().is_pubsub_connection("unknown_connection"));
  BOOST_REQUIRE(!NetworkManager::get().is_pubsub_connection("unknown_topic"));

  nwmgr::Connections testConfig;
  nwmgr::Connection testConn;
  testConn.name = "oof";
  testConn.address = "inproc://rab";
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

BOOST_FIXTURE_TEST_CASE(NameCollisionInConfiguration, NetworkManagerTestFixture)
{
  NetworkManager::get().reset();
  nwmgr::Connections testConfig;
  testConfig.push_back({ "foo", "inproc://foo", {} });
  testConfig.push_back({ "foo", "inproc://bar", {} });
  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().configure(testConfig), NameCollision, [&](NameCollision const&) { return true; });

  nwmgr::Connections testConfig2;
  testConfig2.push_back({ "foo", "inproc://foo", {} });
  testConfig2.push_back({ "bar", "inproc://bar", { "foo" } });
  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().configure(testConfig2), NameCollision, [&](NameCollision const&) { return true; });

  nwmgr::Connections testConfig3;
  testConfig3.push_back({ "foo", "inproc://foo", {} });
  testConfig3.push_back({ "bar", "inproc://bar", { "bax" } });
  testConfig3.push_back({ "baz", "inproc://baz", { "bax" } });
  NetworkManager::get().configure(testConfig3);
}

BOOST_FIXTURE_TEST_CASE(Listener, NetworkManagerTestFixture)
{
  BOOST_REQUIRE(NetworkManager::get().is_connection("foo"));
  BOOST_REQUIRE(!NetworkManager::get().is_pubsub_connection("foo"));
  BOOST_REQUIRE(!NetworkManager::get().is_topic("foo"));

  BOOST_REQUIRE(!NetworkManager::get().is_listening("foo"));
  NetworkManager::get().start_listening("foo");
  BOOST_REQUIRE(NetworkManager::get().is_listening("foo"));

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().start_listening("foo"),
                          ListenerAlreadyRegistered,
                          [&](ListenerAlreadyRegistered const&) { return true; });

  NetworkManager::get().stop_listening("foo");

  BOOST_REQUIRE(!NetworkManager::get().is_listening("foo"));
  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().register_callback("foo", [&](dunedaq::ipm::Receiver::Response) {}),
                          ListenerNotRegistered,
                          [&](ListenerNotRegistered const&) { return true; });
  BOOST_REQUIRE(!NetworkManager::get().is_listening("foo"));

  BOOST_REQUIRE(!NetworkManager::get().is_listening("bar"));
  NetworkManager::get().start_listening("bar");
  BOOST_REQUIRE(NetworkManager::get().is_listening("bar"));
  BOOST_REQUIRE(!NetworkManager::get().is_listening("baz"));
  NetworkManager::get().register_callback("bar", [&](dunedaq::ipm::Receiver::Response) {});
  BOOST_REQUIRE(NetworkManager::get().is_listening("bar"));

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().stop_listening("foo"),
                          ListenerNotRegistered,
                          [&](ListenerNotRegistered const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().start_listening("unknown_connection"),
                          ConnectionNotFound,
                          [&](ConnectionNotFound const&) { return true; });
  BOOST_REQUIRE(!NetworkManager::get().is_listening("unknown_connection"));

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().register_callback("unknown_connection", [&](dunedaq::ipm::Receiver::Response) {}),
    ConnectionNotFound,
    [&](ConnectionNotFound const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().stop_listening("unknown_connection"),
                          ListenerNotRegistered,
                          [&](ListenerNotRegistered const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(StartPublisher, NetworkManagerTestFixture)
{
  NetworkManager::get().reset();

  nwmgr::Connections testConfig;
  nwmgr::Connection testConn;
  testConn.name = "foo";
  testConn.address = "inproc://foo";
  testConfig.push_back(testConn);
  testConn.name = "bar";
  testConn.address = "inproc://bar";
  testConn.topics = { "bax", "bay", "baz" };
  testConfig.push_back(testConn);
  NetworkManager::get().configure(testConfig);

  BOOST_REQUIRE(!NetworkManager::get().is_connection_open("bar", NetworkManager::ConnectionDirection::Send));
  NetworkManager::get().start_publisher("bar");
  BOOST_REQUIRE(NetworkManager::get().is_connection_open("bar", NetworkManager::ConnectionDirection::Send));

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().start_publisher("bax"), ConnectionNotFound, [&](ConnectionNotFound const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().start_publisher("foo"), OperationFailed, [&](OperationFailed const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(Subscriber, NetworkManagerTestFixture)
{
  BOOST_REQUIRE(!NetworkManager::get().is_listening("baz"));
  NetworkManager::get().subscribe("baz");
  BOOST_REQUIRE(NetworkManager::get().is_listening("baz"));

  BOOST_REQUIRE(!NetworkManager::get().is_listening("bar"));

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().subscribe("baz"),
                          ListenerAlreadyRegistered,
                          [&](ListenerAlreadyRegistered const&) { return true; });

  NetworkManager::get().subscribe("bax");
  BOOST_REQUIRE(NetworkManager::get().is_listening("bax"));

  NetworkManager::get().unsubscribe("bax");
  BOOST_REQUIRE(!NetworkManager::get().is_listening("bax"));
  BOOST_REQUIRE(NetworkManager::get().is_listening("baz"));

  BOOST_REQUIRE(!NetworkManager::get().is_listening("bax"));
  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().register_callback("bax", [&](dunedaq::ipm::Receiver::Response) {}),
                          ListenerNotRegistered,
                          [&](ListenerNotRegistered const&) { return true; });
  BOOST_REQUIRE(!NetworkManager::get().is_listening("bax"));

  NetworkManager::get().subscribe("bax");
  BOOST_REQUIRE(NetworkManager::get().is_listening("bax"));

  BOOST_REQUIRE(!NetworkManager::get().is_listening("bay"));

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().subscribe("unknown_topic"), TopicNotFound, [&](TopicNotFound const&) { return true; });
  BOOST_REQUIRE(!NetworkManager::get().is_listening("unknown_topic"));

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().unsubscribe("unknown_topic"),
                          ListenerNotRegistered,
                          [&](ListenerNotRegistered const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(ReceiveFrom, NetworkManagerTestFixture)
{
  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().receive_from("foo", dunedaq::ipm::Receiver::s_no_block),
                          dunedaq::ipm::ReceiveTimeoutExpired,
                          [&](dunedaq::ipm::ReceiveTimeoutExpired const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().receive_from("oof", dunedaq::ipm::Receiver::s_no_block),
                          ConnectionNotFound,
                          [&](ConnectionNotFound const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().receive_from("foo", dunedaq::ipm::Receiver::s_no_block),
                          dunedaq::ipm::ReceiveTimeoutExpired,
                          [&](dunedaq::ipm::ReceiveTimeoutExpired const&) { return true; });

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().receive_from("baz", dunedaq::ipm::Receiver::s_no_block),
                          dunedaq::ipm::ReceiveTimeoutExpired,
                          [&](dunedaq::ipm::ReceiveTimeoutExpired const&) { return true; });
}

BOOST_FIXTURE_TEST_CASE(SendTo, NetworkManagerTestFixture)
{
  std::string sent_string;
  std::string received_string;

  std::function<void(dunedaq::ipm::Receiver::Response)> callback = [&](dunedaq::ipm::Receiver::Response response) {
    received_string = std::string(response.data.begin(), response.data.end());
  };
  NetworkManager::get().start_listening("foo");
  NetworkManager::get().register_callback("foo", callback);

  received_string = "";
  sent_string = "this is the first test string";
  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block);

  while (received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(received_string, sent_string);

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().send_to("baz", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block),
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

BOOST_FIXTURE_TEST_CASE(Publish, NetworkManagerTestFixture)
{
  std::string sent_string;
  std::string received_string;

  BOOST_REQUIRE(!NetworkManager::get().is_listening("baz"));

  std::function<void(dunedaq::ipm::Receiver::Response)> callback = [&](dunedaq::ipm::Receiver::Response response) {
    received_string = std::string(response.data.begin(), response.data.end());
  };
  NetworkManager::get().subscribe("baz");

  BOOST_REQUIRE(NetworkManager::get().is_listening("baz"));
  BOOST_REQUIRE(NetworkManager::get().is_connection_open("baz"));

  NetworkManager::get().register_callback("baz", callback);

  received_string = "";
  sent_string = "this is the first test string";
  NetworkManager::get().send_to("bar", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "baz");

  while (received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(received_string, sent_string);

  BOOST_REQUIRE_EXCEPTION(
    NetworkManager::get().send_to("baz", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block),
    ConnectionNotFound,
    [&](ConnectionNotFound const&) { return true; });

  NetworkManager::get().send_to("foo", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "baz");

  BOOST_REQUIRE_EXCEPTION(NetworkManager::get().receive_from("baz", dunedaq::ipm::Receiver::s_no_block),
                          dunedaq::ipm::ReceiveTimeoutExpired,
                          [&](dunedaq::ipm::ReceiveTimeoutExpired const&) { return true; });

  NetworkManager::get().receive_from("foo", dunedaq::ipm::Receiver::s_no_block);

  sent_string = "this is another test string";
  received_string = "";
  NetworkManager::get().send_to("rab", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "baz");

  while (received_string == "") {
    usleep(1000);
  }

  BOOST_REQUIRE_EQUAL(received_string, sent_string);

  std::string received_string2;
  std::function<void(dunedaq::ipm::Receiver::Response)> callback2 = [&](dunedaq::ipm::Receiver::Response response) {
    received_string2 = std::string(response.data.begin(), response.data.end());
  };
  NetworkManager::get().subscribe("bax");
  NetworkManager::get().register_callback("bax", callback2);

  sent_string = "this is a third test string";
  received_string = "";
  received_string2 = "";
  NetworkManager::get().send_to("bar", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "bax");

  while (received_string2 == "") {
    usleep(1000);
  }
  BOOST_REQUIRE_EQUAL(received_string2, sent_string);
  BOOST_REQUIRE_EQUAL(received_string, "");
}

BOOST_FIXTURE_TEST_CASE(SingleConnectionSubscriber, NetworkManagerTestFixture)
{

  std::string sent_string;
  std::string received_string_topic;
  std::string received_string_conn;

  std::function<void(dunedaq::ipm::Receiver::Response)> callback_topic =
    [&](dunedaq::ipm::Receiver::Response response) {
      received_string_topic = std::string(response.data.begin(), response.data.end());
    };
  NetworkManager::get().subscribe("baz");
  NetworkManager::get().register_callback("baz", callback_topic);

  std::function<void(dunedaq::ipm::Receiver::Response)> callback_conn = [&](dunedaq::ipm::Receiver::Response response) {
    received_string_conn = std::string(response.data.begin(), response.data.end());
  };
  NetworkManager::get().start_listening("bar");
  NetworkManager::get().register_callback("bar", callback_conn);

  sent_string = "this is the first test string";
  received_string_conn = "";
  received_string_topic = "";

  NetworkManager::get().send_to("bar", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "baz");
  while (received_string_conn == "" || received_string_topic == "") {
    usleep(1000);
  }
  BOOST_REQUIRE_EQUAL(received_string_conn, sent_string);
  BOOST_REQUIRE_EQUAL(received_string_topic, sent_string);

  sent_string = "this is the second test string";
  received_string_conn = "";
  received_string_topic = "";

  NetworkManager::get().send_to("bar", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "bax");
  while (received_string_conn == "") {
    usleep(1000);
  }
  BOOST_REQUIRE_EQUAL(received_string_conn, sent_string);
  BOOST_REQUIRE_EQUAL(received_string_topic, "");

  sent_string = "this is the third test string";
  received_string_conn = "";
  received_string_topic = "";

  NetworkManager::get().send_to("rab", sent_string.c_str(), sent_string.size(), dunedaq::ipm::Sender::s_block, "baz");
  while (received_string_topic == "") {
    usleep(1000);
  }
  BOOST_REQUIRE_EQUAL(received_string_conn, "");
  BOOST_REQUIRE_EQUAL(received_string_topic, sent_string);
}

BOOST_FIXTURE_TEST_CASE(SendThreadSafety, NetworkManagerTestFixture)
{
  TLOG_DEBUG(12) << "SendThreadSafety test case BEGIN";

  auto substr_proc = [&](int idx) {
    const std::string pattern_string =
      "aaaaabbbbbcccccdddddeeeeefffffggggghhhhhiiiiijjjjjkkkkklllllmmmmmnnnnnooooopppppqqqqqrrrrrssssstttttuuuuuvvvvvww"
      "wwwxxxxxyyyyyzzzzzAAAAABBBBBCCCCCDDDDDEEEEEFFFFFGGGGGHHHHHIIIIIJJJJJKKKKKLLLLLMMMMMNNNNNOOOOOPPPPPQQQQQRRRRRSSSS"
      "STTTTTUUUUUVVVVVWWWWWXXXXXYYYYYZZZZZ";
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

  NetworkManager::get().start_listening("foo");

  NetworkManager::get().register_callback("foo", recv_proc);

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

BOOST_FIXTURE_TEST_CASE(OneListenerThreaded, NetworkManagerTestFixture)
{
  auto callback = [&](dunedaq::ipm::Receiver::Response) { return; };
  const int thread_count = 1000;
  std::atomic<size_t> num_connected = 0;
  std::atomic<size_t> num_fail = 0;

  auto reg_proc = [&](int idx) {
    try {
      NetworkManager::get().start_listening("foo");
      NetworkManager::get().register_callback("foo", callback);
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

  nwmgr::Connections testConfig;
  for (int i = 0; i < num_receivers; ++i) {
    nwmgr::Connection testConn;
    testConn.name = "foo" + std::to_string(i);
    testConn.address = "inproc://bar" + std::to_string(i);
    testConfig.push_back(testConn);
  }
  NetworkManager::get().configure(testConfig);

  auto substr_proc = [](int idx) {
    const std::string pattern_string =
      "aaaaabbbbbcccccdddddeeeeefffffggggghhhhhiiiiijjjjjkkkkklllllmmmmmnnnnnooooopppppqqqqqrrrrrssssstttttuuuuuvvvvvww"
      "wwwxxxxxyyyyyzzzzzAAAAABBBBBCCCCCDDDDDEEEEEFFFFFGGGGGHHHHHIIIIIJJJJJKKKKKLLLLLMMMMMNNNNNOOOOOPPPPPQQQQQRRRRRSSSS"
      "STTTTTUUUUUVVVVVWWWWWXXXXXYYYYYZZZZZ";

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
  std::array<std::atomic<size_t>, num_receivers> num_empty_responses;
  std::array<std::atomic<size_t>, num_receivers> num_size_errors;
  std::array<std::atomic<size_t>, num_receivers> num_content_errors;
  std::array<std::function<void(dunedaq::ipm::Receiver::Response)>, num_receivers> recv_procs;

  for (int i = 0; i < num_receivers; ++i) {
    messages_received[i] = 0;
    num_empty_responses[i] = 0;
    num_size_errors[i] = 0;
    num_content_errors[i] = 0;
    recv_procs[i] = [&, i](dunedaq::ipm::Receiver::Response response) {
      if (response.data.size() == 0) {
        num_empty_responses[i]++;
      }
      auto received_idx = std::stoi(std::string(response.data.begin(), response.data.end()));
      auto idx_string = std::to_string(received_idx);
      auto received_string = std::string(response.data.begin() + idx_string.size(), response.data.end());

      TLOG_DEBUG(14) << "Receiver " << i << " received " << received_string << " for idx " << received_idx;

      if (received_string.size() != 5) {
        num_size_errors[i]++;
      }

      std::string check = substr_proc(received_idx);

      if (received_string != check) {
        num_content_errors[i]++;
      }
      messages_received[i]++;
    };
    NetworkManager::get().start_listening("foo" + std::to_string(i));
    NetworkManager::get().register_callback("foo" + std::to_string(i), recv_procs[i]);
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

  TLOG_DEBUG(14) << "Sleeping to allow all messages to be processed";
  std::this_thread::sleep_for(std::chrono::seconds(1));

  for (auto i = 0; i < num_receivers; ++i) {
    TLOG_DEBUG(14) << "Shutting down receiver " << i;
    NetworkManager::get().stop_listening("foo" + std::to_string(i));
    BOOST_CHECK_EQUAL(messages_received[i], num_sending_threads);
    BOOST_REQUIRE_EQUAL(num_empty_responses[i], 0);
    BOOST_REQUIRE_EQUAL(num_size_errors[i], 0);
    BOOST_REQUIRE_EQUAL(num_content_errors[i], 0);
  }

  TLOG_DEBUG(14) << "Resetting NetworkManager";
  NetworkManager::get().reset();
}

BOOST_AUTO_TEST_SUITE_END()
