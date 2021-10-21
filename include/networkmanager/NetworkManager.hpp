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

  void configure(const nwmgr::Connections& connections);
  void reset();

  // Receive via callback
  void start_listening(std::string const& connection_name);
  void stop_listening(std::string const& connection_name);
  void register_callback(std::string const& connection_or_topic, std::function<void(ipm::Receiver::Response)> callback);
  void clear_callback(std::string const& connection_or_topic);
  void subscribe(std::string const& topic);
  void unsubscribe(std::string const& topic);

  // Direct Send/Receive
  void start_publisher(std::string const& connection_name);
  void send_to(std::string const& connection_name,
               const void* buffer,
               size_t size,
               ipm::Sender::duration_t timeout,
               std::string const& topic = "");
  ipm::Receiver::Response receive_from(std::string const& connection_or_topic,
                                       ipm::Receiver::duration_t timeout);

  std::string get_connection_string(std::string const& connection_name) const;
  std::vector<std::string> get_connection_strings(std::string const& topic) const;

  bool is_topic(std::string const& topic) const;
  bool is_connection(std::string const& connection_name) const;
  bool is_pubsub_connection(std::string const& connection_name) const;
  bool is_listening(std::string const& connection_or_topic) const;

  bool is_connection_open(std::string const& connection_name,
                          ConnectionDirection direction = ConnectionDirection::Recv) const;

private:
  static std::unique_ptr<NetworkManager> s_instance;

  NetworkManager() = default;

  NetworkManager(NetworkManager const&) = delete;
  NetworkManager(NetworkManager&&) = delete;
  NetworkManager& operator=(NetworkManager const&) = delete;
  NetworkManager& operator=(NetworkManager&&) = delete;

  void create_receiver(std::string const& connection_or_topic);
  void create_sender(std::string const& connection_name);

  std::unordered_map<std::string, nwmgr::Connection> m_connection_map;
  std::unordered_map<std::string, std::vector<std::string>> m_topic_map;
  std::unordered_map<std::string, std::shared_ptr<ipm::Receiver>> m_receiver_plugins;
  std::unordered_map<std::string, std::shared_ptr<ipm::Sender>> m_sender_plugins;
  std::unordered_map<std::string, Listener> m_registered_listeners;

  std::unique_lock<std::mutex> get_connection_lock(std::string const& connection_name) const;
  mutable std::unordered_map<std::string, std::mutex> m_connection_mutexes;
  std::mutex m_receiver_plugin_map_mutex;
  std::mutex m_sender_plugin_map_mutex;
  std::mutex m_registration_mutex;
};
} // namespace networkmanager
} // namespace dunedaq

#endif // NETWORKMANAGER_INCLUDE_NETWORKMANAGER_NETWORKMANAGER_HPP_
