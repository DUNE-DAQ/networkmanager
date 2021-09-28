/**
 *
 * @file Subscriber.hpp NetworkManager Subscriber class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef NETWORKMANAGER_INCLUDE_NETWORKMANAGER_SUBSCRIBER_HPP_
#define NETWORKMANAGER_INCLUDE_NETWORKMANAGER_SUBSCRIBER_HPP_

#include "networkmanager/Issues.hpp"

#include "ipm/Receiver.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace dunedaq {
namespace networkmanager {

class Subscriber
{
public:
  Subscriber() = default; // So that Subscriber can be used with operator[] in a map
  explicit Subscriber(std::string const& connection_name);

  virtual ~Subscriber() noexcept;
  Subscriber(Subscriber&&);
  Subscriber& operator=(Subscriber&&);

  Subscriber(Subscriber const&) = delete;
  Subscriber& operator=(Subscriber const&) = delete;

  void add_callback(std::function<void(ipm::Receiver::Response)> callback, std::string const& topic = "");
  void remove_callback(std::string const& topic = "");
  bool has_callback(std::string const& topic = "") const;
  size_t num_callbacks() const { return m_callbacks.size(); }
  std::unordered_set<std::string> topics() const;

  bool is_running() const { return m_is_running.load(); }

  void shutdown();

private:
  void startup();
  void subscriber_thread_loop();

  std::string m_connection_name = "";
  std::unordered_map<std::string, std::function<void(ipm::Receiver::Response)>> m_callbacks;
  std::unique_ptr<std::thread> m_subscriber_thread{ nullptr };
  std::atomic<bool> m_is_running{ false };
};
} // namespace networkmanager
} // namespace dunedaq

#endif // NETWORKMANAGER_INCLUDE_NETWORKMANAGER_SUBSCRIBER_HPP_
