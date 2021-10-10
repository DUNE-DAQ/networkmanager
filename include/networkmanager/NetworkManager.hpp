/**
 *
 * @file NetworkManager.hpp NetworkManager Singleton class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef NETWORKMANAGER_INCLUDE_NETWORKMANAGER_NETWORKMANAGER_HPP_
#define NETWORKMANAGER_INCLUDE_NETWORKMANAGER_NETWORKMANAGER_HPP_

#include "networkmanager/Issues.hpp"
#include "networkmanager/Listener.hpp"
#include "networkmanager/Subscriber.hpp"
#include "networkmanager/nwmgr/Structs.hpp"

#include "ipm/Receiver.hpp"
#include "ipm/Sender.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

namespace dunedaq {

namespace networkmanager {
class NetworkManager
{

public:
  enum class ConnectionDirection
  {
    Send,
    Recv
  };

  static NetworkManager& get();

  // Receive via callback
  void start_listening(std::string const& connection_name, std::function<void(ipm::Receiver::Response)> callback);
  void stop_listening(std::string const& connection_name);

  void add_subscriber(std::string const& connection_name,
                      std::string const& topic,
                      std::function<void(ipm::Receiver::Response)> callback);
  void remove_subscriber(std::string const& connection_name, std::string const& topic);

  // Direct Send/Receive
  void send_to(std::string const& connection_name,
               const void* buffer,
               size_t size,
               ipm::Sender::duration_t timeout,
               std::string const& topic = "");
  ipm::Receiver::Response receive_from(std::string const& connection_name,
                                       ipm::Receiver::duration_t timeout,
                                       std::string const& topic = "");

  void configure(const nwmgr::Connections& connections);
  void reset();

  std::string get_connection_string(std::string const& connection_name) const;

  bool is_connection_open(std::string const& connection_name,
                          ConnectionDirection direction = ConnectionDirection::Recv) const;
  bool is_listening(std::string const& connection_name) const;
  bool has_subscriber(std::string const& connection_name, std::string const& topic) const;

private:
  static std::unique_ptr<NetworkManager> s_instance;

  NetworkManager() = default;

  NetworkManager(NetworkManager const&) = delete;
  NetworkManager(NetworkManager&&) = delete;
  NetworkManager& operator=(NetworkManager const&) = delete;
  NetworkManager& operator=(NetworkManager&&) = delete;

  void create_receiver(std::string const& connection_name);
  void create_sender(std::string const& connection_name);

  std::unordered_map<std::string, nwmgr::Connection> m_connection_map;
  std::unordered_map<std::string, std::shared_ptr<ipm::Receiver>> m_receiver_plugins;
  std::unordered_map<std::string, std::shared_ptr<ipm::Sender>> m_sender_plugins;
  std::unordered_map<std::string, Listener> m_registered_listeners;
  std::unordered_map<std::string, Subscriber> m_registered_subscribers;

  std::unique_lock<std::mutex> get_connection_lock(std::string const& connection_name) const;
  mutable std::unordered_map<std::string, std::mutex> m_connection_mutexes;
  std::mutex m_registration_mutex;
};
} // namespace networkmanager
} // namespace dunedaq

#endif // NETWORKMANAGER_INCLUDE_NETWORKMANAGER_NETWORKMANAGER_HPP_
