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

#include "networkmanager/Listener.hpp"
#include "networkmanager/networkmanager/Structs.hpp"
#include "networkmanager/Issues.hpp"

#include "ipm/Receiver.hpp"
#include "ipm/Sender.hpp"

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
class NetworkManager
{

public:
  static NetworkManager& get();

  // Receive via callback
  void add_listener(std::string const& connection_name, std::function<void(ipm::Receiver::Response)> callback);
  void remove_listener(std::string const& connection_name);
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

  void configure(networkmanager::Conf configuration);
  void reset();

  std::string get_connection_string(std::string const& connection_name) const;

  bool has_listener(std::string const& connection_name) const;
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

  std::unordered_map<std::string, networkmanager::Connection> m_connection_map;
  std::unordered_map<std::string, std::shared_ptr<ipm::Receiver>> m_receiver_plugins;
  std::unordered_map<std::string, std::shared_ptr<ipm::Sender>> m_sender_plugins;
  std::unordered_map<std::string, Listener> m_registered_listeners;
};
} // namespace networkmanager
} // namespace dunedaq

#endif // NETWORKMANAGER_INCLUDE_NETWORKMANAGER_NETWORKMANAGER_HPP_
