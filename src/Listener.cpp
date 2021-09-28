#include "networkmanager/Listener.hpp"
#include "networkmanager/NetworkManager.hpp"

namespace dunedaq::networkmanager {
Listener::Listener(std::string const& connection_name, bool is_subscriber)
  : m_connection_name(connection_name)
  , m_is_subscriber(is_subscriber)
{}

Listener::~Listener() noexcept
{
  shutdown();
}

Listener::Listener(Listener&& other)
  : m_connection_name(other.m_connection_name)
  , m_is_subscriber(other.m_is_subscriber)
  , m_callbacks(std::move(other.m_callbacks))
  , m_listener_thread(std::move(other.m_listener_thread))
  , m_is_listening(other.m_is_listening.load())
  , m_callbacks_updated(true)
{}

Listener&
Listener::operator=(Listener&& other)
{
  m_connection_name = other.m_connection_name;
  m_is_subscriber = other.m_is_subscriber;
  m_callbacks = std::move(other.m_callbacks);
  m_listener_thread = std::move(other.m_listener_thread);
  m_is_listening = other.m_is_listening.load();
  m_callbacks_updated = true;
  return *this;
}

void
Listener::add_callback(std::function<void(ipm::Receiver::Response)> callback, std::string const& topic)
{
  if (!m_is_subscriber && topic != "") {
    throw SubscriptionsNotSupported(ERS_HERE, m_connection_name, topic);
  }
  if (has_callback(topic)) {
    throw CallbackAlreadyRegistered(ERS_HERE, m_connection_name, topic);
  }

  m_callbacks[topic] = callback;
  if (!is_listening())
    startup();
}

void
Listener::remove_callback(std::string const& topic)
{
  if (!m_is_subscriber && topic != "") {
    throw SubscriptionsNotSupported(ERS_HERE, m_connection_name, topic);
  }
  if (!has_callback(topic)) {
    throw CallbackNotRegistered(ERS_HERE, m_connection_name, topic);
  }

  m_callbacks.erase(topic);
  if (m_callbacks.empty())
    shutdown();
}

bool
Listener::has_callback(std::string const& topic) const
{
  return m_callbacks.count(topic);
}

void
Listener::startup()
{
  m_is_listening = true;
  m_listener_thread.reset(new std::thread([&] { listener_thread_loop(); }));
}

void
Listener::shutdown()
{
  m_is_listening = false;
  if (m_listener_thread && m_listener_thread->joinable())
    m_listener_thread->join();
  m_callbacks.clear();
}

void
Listener::listener_thread_loop()
{
  while (m_is_listening) {
    try {
      auto response = NetworkManager::get().receive_from(m_connection_name, ipm::Receiver::s_no_block);

      if (m_is_subscriber && m_callbacks.count(response.metadata)) {
        m_callbacks[response.metadata](response);
      }
      if (m_callbacks.count("")) {
        m_callbacks[""](response);
      }
    } catch (ipm::ReceiveTimeoutExpired const& tmo) {
      usleep(1000);
    }
  }
}

} // namespace dunedaq::networkmanager