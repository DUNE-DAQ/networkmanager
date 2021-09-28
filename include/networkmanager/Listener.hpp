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
  explicit Listener(std::string const& connection_name);

  virtual ~Listener() noexcept;
  Listener(Listener&&);
  Listener& operator=(Listener&&);

  Listener(Listener const&) = delete;
  Listener& operator=(Listener const&) = delete;

  void start_listening(std::function<void(ipm::Receiver::Response)> callback);
  void stop_listening();
  void shutdown();

  bool is_listening() const { return m_is_listening.load(); }

private:
  void startup();
  void listener_thread_loop();

  std::string m_connection_name = "";
  std::function<void(ipm::Receiver::Response)> m_callback;
  std::unique_ptr<std::thread> m_listener_thread{ nullptr };
  std::atomic<bool> m_is_listening{ false };
};
} // namespace networkmanager
} // namespace dunedaq

#endif // NETWORKMANAGER_INCLUDE_NETWORKMANAGER_LISTENER_HPP_
