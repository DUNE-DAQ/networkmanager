/**
 * @file NetworkManager.hpp NetworkManager Class implementations
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "networkmanager/NetworkManager.hpp"
#include "ipm/PluginInfo.hpp"
#include "ipm/Subscriber.hpp"
#include "logging/Logging.hpp"

#include <memory>
#include <string>
#include <vector>

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
NetworkManager::configure(const nwmgr::Connections& connections)
{
  if (!m_connection_map.empty()) {
    throw NetworkManagerAlreadyConfigured(ERS_HERE);
  }

  for (auto& connection : connections) {
    TLOG_DEBUG(15) << "Adding connection " << connection.name << " to connection map";
    if (m_connection_map.count(connection.name) || m_topic_map.count(connection.name)) {
      TLOG_DEBUG(15) << "Name collision for connection name " << connection.name
                     << " connection_map.count: " << m_connection_map.count(connection.name)
                     << ", topic_map.count: " << m_topic_map.count(connection.name);
      reset();
      throw NameCollision(ERS_HERE, connection.name);
    }
    m_connection_map[connection.name] = connection;
    if (!connection.topics.empty()) {
      for (auto& topic : connection.topics) {
        TLOG_DEBUG(15) << "Adding topic " << topic << " for connection name " << connection.name << " to topics map";
        if (m_connection_map.count(topic)) {
          reset();
          TLOG_DEBUG(15) << "Name collision with existing connection for topic " << topic << " on connection "
                         << connection.name;
          throw NameCollision(ERS_HERE, topic);
        }
        m_topic_map[topic].push_back(connection.name);
      }
    }
  }
}

void
NetworkManager::reset()
{
  std::lock_guard<std::mutex> lk(m_registration_mutex);
  for (auto& listener_pair : m_registered_listeners) {
    listener_pair.second.shutdown();
  }
  m_registered_listeners.clear();
  {
    std::lock_guard<std::mutex> lk(m_sender_plugin_map_mutex);
    m_sender_plugins.clear();
  }
  {
    std::lock_guard<std::mutex> lk(m_receiver_plugin_map_mutex);
    m_receiver_plugins.clear();
  }
  m_topic_map.clear();
  m_connection_map.clear();
  m_connection_mutexes.clear();
}

void
NetworkManager::start_listening(std::string const& connection_name)
{
  TLOG_DEBUG(5) << "Start listening on connection " << connection_name;
  std::lock_guard<std::mutex> lk(m_registration_mutex);
  if (!m_connection_map.count(connection_name)) {
    throw ConnectionNotFound(ERS_HERE, connection_name);
  }

  /* if (is_pubsub_connection(connection_name)) {
    throw OperationFailed(ERS_HERE, "Connection is pub/sub type, call start_listening on desired topic(s) instead!");
  }*/

  if (is_listening(connection_name)) {
    throw ListenerAlreadyRegistered(ERS_HERE, connection_name);
  }

  m_registered_listeners[connection_name].start_listening(connection_name);
}

void
NetworkManager::stop_listening(std::string const& connection_name)
{
  TLOG_DEBUG(5) << "Stop listening on connection " << connection_name;
  std::lock_guard<std::mutex> lk(m_registration_mutex);
  if (!is_listening(connection_name)) {
    throw ListenerNotRegistered(ERS_HERE, connection_name);
  }

  m_registered_listeners[connection_name].stop_listening();
}

void
NetworkManager::register_callback(std::string const& connection_or_topic,
                                  std::function<void(ipm::Receiver::Response)> callback)
{
  TLOG_DEBUG(5) << "Registering callback on connection or topic " << connection_or_topic;
  std::lock_guard<std::mutex> lk(m_registration_mutex);
  if (!m_connection_map.count(connection_or_topic) && !m_topic_map.count(connection_or_topic)) {
    throw ConnectionNotFound(ERS_HERE, connection_or_topic);
  }

  /* if (is_pubsub_connection(connection_or_topic)) {
    throw OperationFailed(ERS_HERE, "Connection is pub/sub type, call register_callback on desired topic(s) instead!");
  }*/

  if (!is_listening(connection_or_topic)) {
    throw ListenerNotRegistered(ERS_HERE, connection_or_topic);
  }

  m_registered_listeners[connection_or_topic].set_callback(callback);
}

void
NetworkManager::clear_callback(std::string const& connection_or_topic)
{
  TLOG_DEBUG(5) << "Setting callback on " << connection_or_topic << " to null";
  register_callback(connection_or_topic, nullptr);
}

void
NetworkManager::subscribe(std::string const& topic)
{
  TLOG_DEBUG(5) << "Start listening on topic " << topic;
  std::lock_guard<std::mutex> lk(m_registration_mutex);
  if (!m_topic_map.count(topic)) {
    throw TopicNotFound(ERS_HERE, topic);
  }

  if (is_listening(topic)) {
    throw ListenerAlreadyRegistered(ERS_HERE, topic);
  }

  m_registered_listeners[topic].start_listening(topic);
}

void
NetworkManager::unsubscribe(std::string const& topic)
{
  TLOG_DEBUG(5) << "Stop listening on topic " << topic;
  std::lock_guard<std::mutex> lk(m_registration_mutex);
  if (!is_listening(topic)) {
    throw ListenerNotRegistered(ERS_HERE, topic);
  }

  m_registered_listeners[topic].stop_listening();
}

void
NetworkManager::start_publisher(std::string const& connection_name)
{
  TLOG_DEBUG(10) << "Getting connection lock for connection " << connection_name;
  auto send_mutex = get_connection_lock(connection_name);

  if (!m_connection_map.count(connection_name)) {
    throw ConnectionNotFound(ERS_HERE, connection_name);
  }
  if (m_connection_map[connection_name].topics.empty()) {
    throw OperationFailed(ERS_HERE, "Connection is not pub/sub type, cannot start sender early");
  }

  if (!m_sender_plugins.count(connection_name)) {
    create_sender(connection_name);
  }
}

void
NetworkManager::send_to(std::string const& connection_name,
                        const void* buffer,
                        size_t size,
                        ipm::Sender::duration_t timeout,
                        std::string const& topic)
{
  TLOG_DEBUG(10) << "Getting connection lock for connection " << connection_name;
  auto send_lock = get_connection_lock(connection_name);

  TLOG_DEBUG(10) << "Checking connection map";
  if (!m_connection_map.count(connection_name)) {
    throw ConnectionNotFound(ERS_HERE, connection_name);
  }

  if (topic != "") {
    bool match = false;
    for (auto& configured_topic : m_connection_map.at(connection_name).topics) {
      if (topic == configured_topic) {
        match = true;
        break;
      }
    }
    if (!match) {
      ers::warning(ConnectionTopicNotFound(ERS_HERE, topic, connection_name));
    }
  }

  TLOG_DEBUG(10) << "Checking sender plugins";
  if (!m_sender_plugins.count(connection_name)) {
    create_sender(connection_name);
  }

  TLOG_DEBUG(10) << "Sending message";
  std::shared_ptr<ipm::Sender> sender_ptr;
  {
    std::lock_guard<std::mutex> lk(m_sender_plugin_map_mutex);
    sender_ptr = m_sender_plugins[connection_name];
  }
  sender_ptr->send(buffer, size, timeout, topic);
}

ipm::Receiver::Response
NetworkManager::receive_from(std::string const& connection_or_topic, ipm::Receiver::duration_t timeout)
{
  TLOG_DEBUG(9) << "START";

  if (!m_connection_map.count(connection_or_topic) && !m_topic_map.count(connection_or_topic)) {
    throw ConnectionNotFound(ERS_HERE, connection_or_topic);
  }

  if (!m_receiver_plugins.count(connection_or_topic)) {
    TLOG_DEBUG(9) << "Creating receiver for connection or topic " << connection_or_topic;
    create_receiver(connection_or_topic);
  }

  TLOG_DEBUG(9) << "Calling receive on connection or topic " << connection_or_topic;

  std::shared_ptr<ipm::Receiver> receiver_ptr;
  {
    std::lock_guard<std::mutex> lk(m_receiver_plugin_map_mutex);
    receiver_ptr = m_receiver_plugins[connection_or_topic];
  }
  auto res = receiver_ptr->receive(timeout);

  TLOG_DEBUG(9) << "END";
  return res;
}

std::string
NetworkManager::get_connection_string(std::string const& connection_name) const
{
  if (!m_connection_map.count(connection_name)) {
    throw ConnectionNotFound(ERS_HERE, connection_name);
  }

  return m_connection_map.at(connection_name).address;
}

std::vector<std::string>
NetworkManager::get_connection_strings(std::string const& topic) const
{
  if (!m_topic_map.count(topic)) {
    throw TopicNotFound(ERS_HERE, topic);
  }

  std::vector<std::string> output;
  for (auto& connection : m_topic_map.at(topic)) {
    output.push_back(m_connection_map.at(connection).address);
  }

  return output;
}

bool
NetworkManager::is_topic(std::string const& topic) const
{
  if (m_connection_map.count(topic)) {
    return false;
  }
  if (!m_topic_map.count(topic)) {
    return false;
  }

  return true;
}

bool
NetworkManager::is_connection(std::string const& connection_name) const
{
  if (m_topic_map.count(connection_name)) {
    return false;
  }
  if (!m_connection_map.count(connection_name)) {
    return false;
  }

  return true;
}

bool
NetworkManager::is_pubsub_connection(std::string const& connection_name) const
{
  if (is_connection(connection_name)) {
    return !m_connection_map.at(connection_name).topics.empty();
  }

  return false;
}

bool
NetworkManager::is_listening(std::string const& connection_or_topic) const
{
  if (!m_registered_listeners.count(connection_or_topic))
    return false;

  return m_registered_listeners.at(connection_or_topic).is_listening();
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

void
NetworkManager::create_receiver(std::string const& connection_or_topic)
{
  TLOG_DEBUG(12) << "START";
  std::lock_guard<std::mutex> lk(m_receiver_plugin_map_mutex);
  if (m_receiver_plugins.count(connection_or_topic))
    return;

  auto plugin_type = ipm::get_recommended_plugin_name(is_topic(connection_or_topic) ? ipm::IpmPluginType::Subscriber
                                                                                    : ipm::IpmPluginType::Receiver);

  TLOG_DEBUG(12) << "Creating plugin for connection or topic " << connection_or_topic << " of type " << plugin_type;
  m_receiver_plugins[connection_or_topic] = dunedaq::ipm::make_ipm_receiver(plugin_type);
  try {
    nlohmann::json config_json;
    if (is_topic(connection_or_topic)) {
      config_json["connection_strings"] = get_connection_strings(connection_or_topic);
    } else {
      config_json["connection_string"] = get_connection_string(connection_or_topic);
    }
    m_receiver_plugins[connection_or_topic]->connect_for_receives(config_json);

    if (is_topic(connection_or_topic)) {
      TLOG_DEBUG(12) << "Subscribing to topic " << connection_or_topic << " after connect_for_receives";
      auto subscriber = std::dynamic_pointer_cast<ipm::Subscriber>(m_receiver_plugins[connection_or_topic]);
      subscriber->subscribe(connection_or_topic);
    }

    if (is_pubsub_connection(connection_or_topic)) {
      TLOG_DEBUG(12) << "Subscribing to topics on " << connection_or_topic << " after connect_for_receives";
      auto subscriber = std::dynamic_pointer_cast<ipm::Subscriber>(m_receiver_plugins[connection_or_topic]);
      for (auto& topic : m_connection_map[connection_or_topic].topics) {
        subscriber->subscribe(topic);
      }
    }

  } catch (ers::Issue const&) {
    m_receiver_plugins.erase(connection_or_topic);
    throw;
  }
  TLOG_DEBUG(12) << "END";
}

void
NetworkManager::create_sender(std::string const& connection_name)
{
  TLOG_DEBUG(11) << "Getting create mutex";
  std::lock_guard<std::mutex> lk(m_sender_plugin_map_mutex);
  TLOG_DEBUG(11) << "Checking plugin list";
  if (m_sender_plugins.count(connection_name))
    return;

  auto plugin_type = ipm::get_recommended_plugin_name(
    is_pubsub_connection(connection_name) ? ipm::IpmPluginType::Publisher : ipm::IpmPluginType::Sender);

  TLOG_DEBUG(11) << "Creating sender plugin for connection " << connection_name << " of type " << plugin_type;
  m_sender_plugins[connection_name] = dunedaq::ipm::make_ipm_sender(plugin_type);
  TLOG_DEBUG(11) << "Connecting sender plugin for connection " << connection_name;
  try {
    m_sender_plugins[connection_name]->connect_for_sends(
      { { "connection_string", m_connection_map[connection_name].address } });
  } catch (ers::Issue const&) {
    m_sender_plugins.erase(connection_name);
    throw;
  }
}

std::unique_lock<std::mutex>
NetworkManager::get_connection_lock(std::string const& connection_name) const
{
  static std::mutex connection_map_mutex;
  std::unique_lock<std::mutex> lk(connection_map_mutex);
  auto& mut = m_connection_mutexes[connection_name];
  lk.unlock();

  TLOG_DEBUG(13) << "Mutex for connection " << connection_name << " is at " << &mut;
  std::unique_lock<std::mutex> conn_lk(mut);
  return conn_lk;
}

} // namespace dunedaq::networkmanager
