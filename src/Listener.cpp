#include "networkmanager/Listener.hpp"
#include "networkmanager/NetworkManager.hpp"

namespace dunedaq::networkmanager {
Listener::Listener(std::string const& connection_name)
  : m_connection_name(connection_name)
{}

Listener::~Listener() noexcept
{
  shutdown();
}

Listener::Listener(Listener&& other)
  : m_connection_name(other.m_connection_name)
  , m_callback(std::move(other.m_callback))
  , m_listener_thread(std::move(other.m_listener_thread))
  , m_is_listening(other.m_is_listening.load())
{}

Listener&
Listener::operator=(Listener&& other)
{
  m_connection_name = other.m_connection_name;
  m_callback = std::move(other.m_callback);
  m_listener_thread = std::move(other.m_listener_thread);
  m_is_listening = other.m_is_listening.load();
  return *this;
}

void
Listener::start_listening(std::function<void(ipm::Receiver::Response)> callback)
{
  if (m_callback != nullptr) {
    throw ListenerAlreadyRegistered(ERS_HERE, m_connection_name);
  }

  m_callback = callback;
  if (!is_listening())
    startup();
}

void
Listener::stop_listening()
{
  if (m_callback == nullptr) {
    throw ListenerNotRegistered(ERS_HERE, m_connection_name);
  }

  shutdown();
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
  m_callback = nullptr;
}

void
Listener::listener_thread_loop()
{
  while (m_is_listening) {
    try {
      auto response = NetworkManager::get().receive_from(m_connection_name, ipm::Receiver::s_no_block);

      if (m_callback != nullptr) {
        m_callback(response);
      }
    } catch (ipm::ReceiveTimeoutExpired const& tmo) {
      usleep(1000);
    }
  }
}

} // namespace dunedaq::networkmanager