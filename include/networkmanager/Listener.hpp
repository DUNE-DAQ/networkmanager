/**
 *
 * @file Listener.hpp NETWORKMANAGER Listener class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef NETWORKMANAGER_INCLUDE_NETWORKMANAGER_LISTENER_HPP_
#define NETWORKMANAGER_INCLUDE_NETWORKMANAGER_LISTENER_HPP_

#include "networkmanager/Issues.hpp"

#include "ipm/Receiver.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

namespace dunedaq {
namespace networkmanager {

class Listener
{
public:
  Listener() = default; // So that Listener can be used with operator[] in a map
  explicit Listener(std::string const& connection_name, bool is_subscriber);

  virtual ~Listener() noexcept;
  Listener(Listener&&);
  Listener& operator=(Listener&&);

  Listener(Listener const&) = delete;
  Listener& operator=(Listener const&) = delete;

  void add_callback(std::function<void(ipm::Receiver::Response)> callback, std::string const& topic = "");
  void remove_callback(std::string const& topic = "");
  bool has_callback(std::string const& topic = "") const;
  size_t num_callbacks() const { return m_callbacks.size(); }

  bool is_listening() const { return m_is_listening.load(); }
  bool is_subscriber() const { return m_is_subscriber; }

  void shutdown();

private:
  void startup();
  void listener_thread_loop();

  std::string m_connection_name = "";
  bool m_is_subscriber{ false };
  std::unordered_map<std::string, std::function<void(ipm::Receiver::Response)>> m_callbacks;
  std::unique_ptr<std::thread> m_listener_thread{ nullptr };
  std::atomic<bool> m_is_listening{ false };
  std::atomic<bool> m_callbacks_updated{ true };
};
} // namespace networkmanager
} // namespace dunedaq

#endif // NETWORKMANAGER_INCLUDE_NETWORKMANAGER_LISTENER_HPP_
