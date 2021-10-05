/**
 * @file NetworkManager.hpp NetworkManager Class implementations
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "networkmanager/NetworkManager.hpp"
#include "ipm/Subscriber.hpp"
#include "logging/Logging.hpp"

namespace dunedaq::networkmanager {

std::unique_ptr<NetworkManager> NetworkManager::s_instance = nullptr;

NetworkManager&
NetworkManager::get()
{
  if (!s_instance) {
    s_instance.reset(new NetworkManager());
  }
  return *s_instance;
}

void
NetworkManager::start_listening(std::string const& connection_name,
                                std::function<void(ipm::Receiver::Response)> callback)
{
  if (!m_connection_map.count(connection_name)) {
    throw ConnectionNotFound(ERS_HERE, connection_name);
  }

  if (m_connection_map[connection_name].type == networkmanager::Type::Subscriber) {
    throw ConnectionTypeMismatch(ERS_HERE, connection_name, "Subscriber", "add_subscriber");
  }

  if (is_listening(connection_name)) {
    throw ListenerAlreadyRegistered(ERS_HERE, connection_name);
  }

  if (!m_registered_listeners.count(connection_name)) {
    m_registered_listeners[connection_name] = std::move(Listener(connection_name));
  }
  m_registered_listeners[connection_name].start_listening(callback);
}

void
NetworkManager::stop_listening(std::string const& connection_name)
{
  if (!is_listening(connection_name)) {
    throw ListenerNotRegistered(ERS_HERE, connection_name);
  }

  m_registered_listeners[connection_name].stop_listening();
}

void
NetworkManager::add_subscriber(std::string const& connection_name,
                               std::string const& topic,
                               std::function<void(ipm::Receiver::Response)> callback)
{
  if (!m_connection_map.count(connection_name)) {
    throw ConnectionNotFound(ERS_HERE, connection_name);
  }

  if (m_connection_map[connection_name].type == networkmanager::Type::Receiver) {
    throw ConnectionTypeMismatch(ERS_HERE, connection_name, "Receiver", "add_listener");
  }

  if (has_subscriber(connection_name, topic)) {
    throw SubscriberAlreadyRegistered(ERS_HERE, connection_name, topic);
  }

  if (!m_receiver_plugins.count(connection_name)) {
    create_receiver(connection_name);
  }

  auto subscriber = std::dynamic_pointer_cast<ipm::Subscriber>(m_receiver_plugins[connection_name]);
  subscriber->subscribe(topic);

  if (!m_registered_subscribers.count(connection_name)) {
    m_registered_subscribers[connection_name] = std::move(Subscriber(connection_name));
  }
  m_registered_subscribers[connection_name].add_callback(callback, topic);
}

void
NetworkManager::remove_subscriber(std::string const& connection_name, std::string const& topic)
{
  if (!has_subscriber(connection_name, topic)) {
    throw SubscriberNotRegistered(ERS_HERE, connection_name, topic);
  }

  auto subscriber = std::dynamic_pointer_cast<ipm::Subscriber>(m_receiver_plugins[connection_name]);
  subscriber->unsubscribe(topic);
  m_registered_subscribers[connection_name].remove_callback(topic);
}

void
NetworkManager::configure(networkmanager::Conf configuration)
{
  if (!m_connection_map.empty()) {
    throw NetworkManagerAlreadyConfigured(ERS_HERE);
  }

  for (auto& connection : configuration.connections) {
    m_connection_map[connection.name] = connection;
  }
}

void
NetworkManager::reset()
{
  for (auto& listener_pair : m_registered_listeners) {
    listener_pair.second.shutdown();
  }
  for (auto& subscriber_pair : m_registered_subscribers) {
    subscriber_pair.second.shutdown();
  }
  m_registered_listeners.clear();
  m_registered_subscribers.clear();
  m_sender_plugins.clear();
  m_receiver_plugins.clear();
  m_connection_map.clear();
  m_connection_mutexes.clear();
}
std::string
NetworkManager::get_connection_string(std::string const& connection_name) const
{
  if (!m_connection_map.count(connection_name)) {
    throw ConnectionNotFound(ERS_HERE, connection_name);
  }

  return m_connection_map.at(connection_name).address;
}

bool
NetworkManager::is_connection_open(std::string const& connection_name,
                                   NetworkManager::ConnectionDirection direction) const
{
  switch (direction) {
    case ConnectionDirection::Recv:
      return m_receiver_plugins.count(connection_name);
    case ConnectionDirection::Send:
      return m_sender_plugins.count(connection_name);
  }

  return false;
}

bool
NetworkManager::is_listening(std::string const& connection_name) const
{
  if (!m_registered_listeners.count(connection_name))
    return false;

  return m_registered_listeners.at(connection_name).is_listening();
}

bool
NetworkManager::has_subscriber(std::string const& connection_name, std::string const& topic) const
{
  if (!m_registered_subscribers.count(connection_name))
    return false;

  return m_registered_subscribers.at(connection_name).has_callback(topic);
}

ipm::Receiver::Response
NetworkManager::receive_from(std::string const& connection_name,
                             ipm::Receiver::duration_t timeout,
                             std::string const& topic)
{
  TLOG_DEBUG(9) << "START";
  if (!m_connection_map.count(connection_name)) {
    throw ConnectionNotFound(ERS_HERE, connection_name);
  }

  if (!m_receiver_plugins.count(connection_name)) {
    create_receiver(connection_name);
  }

  auto is_subscriber = topic != "" && m_connection_map[connection_name].type == networkmanager::Type::Subscriber;
  if (topic != "" && !is_subscriber) {
    throw ConnectionTypeMismatch(ERS_HERE, connection_name, "Receiver", "receive_from without a topic");
  }

  if (is_subscriber) {
    auto subscriber = std::dynamic_pointer_cast<ipm::Subscriber>(m_receiver_plugins[connection_name]);
    subscriber->subscribe(topic);
  }

  auto res = m_receiver_plugins[connection_name]->receive(timeout);
  if (is_subscriber) {
    auto subscriber = std::dynamic_pointer_cast<ipm::Subscriber>(m_receiver_plugins[connection_name]);
    subscriber->unsubscribe(topic);
  }

  TLOG_DEBUG(9) << "END";
  return res;
}

void
NetworkManager::send_to(std::string const& connection_name,
                        const void* buffer,
                        size_t size,
                        ipm::Sender::duration_t timeout,
                        std::string const& topic)
{
  TLOG_DEBUG(10) << "Getting connection lock for connection " << connection_name;
  auto send_mutex = get_connection_lock(connection_name);

  TLOG_DEBUG(10) << "Checking connection map";
  if (!m_connection_map.count(connection_name)) {
    throw ConnectionNotFound(ERS_HERE, connection_name);
  }

  TLOG_DEBUG(10) << "Checking sender plugins";
  if (!m_sender_plugins.count(connection_name)) {
    create_sender(connection_name);
  }

  TLOG_DEBUG(10) << "Sending message";
  return m_sender_plugins[connection_name]->send(buffer, size, timeout, topic);
}

void
NetworkManager::create_receiver(std::string const& connection_name)
{
  std::lock_guard<std::mutex> lk(m_receiver_create_mutex);
  if (m_receiver_plugins.count(connection_name))
    return;

  auto plugin_type =
    m_connection_map[connection_name].type == networkmanager::Type::Receiver ? "ZmqReceiver" : "ZmqSubscriber";

  m_receiver_plugins[connection_name] = dunedaq::ipm::make_ipm_receiver(plugin_type);
  m_receiver_plugins[connection_name]->connect_for_receives(
    { { "connection_string", m_connection_map[connection_name].address } });
}

void
NetworkManager::create_sender(std::string const& connection_name)
{
  TLOG_DEBUG(11) << "Getting create mutex";
  std::lock_guard<std::mutex> lk(m_sender_create_mutex);
  TLOG_DEBUG(11) << "Checking plugin list";
  if (m_sender_plugins.count(connection_name))
    return;

  TLOG_DEBUG(11) << "Creating sender plugin for connection " << connection_name;
  auto plugin_type =
    m_connection_map[connection_name].type == networkmanager::Type::Receiver ? "ZmqSender" : "ZmqPublisher";

  m_sender_plugins[connection_name] = dunedaq::ipm::make_ipm_sender(plugin_type);
  TLOG_DEBUG(11) << "Connecting sender plugin for connection " << connection_name;
  m_sender_plugins[connection_name]->connect_for_sends(
    { { "connection_string", m_connection_map[connection_name].address } });
}

std::unique_lock<std::mutex>
NetworkManager::get_connection_lock(std::string const& connection_name) const
{
  static std::mutex connection_map_mutex;
  std::lock_guard<std::mutex> lk(connection_map_mutex);

  auto& mut = m_connection_mutexes[connection_name];
  TLOG_DEBUG(13) << "Mutex for connection " << connection_name << " is at " << &mut;
  std::unique_lock<std::mutex> conn_lk(mut);
  return conn_lk;
}

} // namespace dunedaq::networkmanager
