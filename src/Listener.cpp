/**
 *
 * @file Listener.cpp NETWORKMANAGER Listener class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "networkmanager/Listener.hpp"
#include "networkmanager/NetworkManager.hpp"

#include "logging/Logging.hpp"

#include <string>
#include <utility>

namespace dunedaq::networkmanager {

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
Listener::start_listening(std::string const& connection_name)
{
  if (m_connection_name != "" && connection_name != m_connection_name) {
    throw OperationFailed(ERS_HERE, "Listener started with different connection name");
  }
  m_connection_name = connection_name;
  if (!is_listening())
    startup();
  else
    ers::warning(OperationFailed(ERS_HERE, "Listener is already running"));
}

void
Listener::stop_listening()
{
  if (is_listening())
    shutdown();
  else
    ers::warning(OperationFailed(ERS_HERE, "Listener is not running"));
}

void
Listener::set_callback(std::function<void(ipm::Receiver::Response)> callback)
{
  m_callback = callback;
}

void
Listener::startup()
{
  shutdown();
  m_listener_thread.reset(new std::thread([&] { listener_thread_loop(); }));

  while (!m_is_listening.load()) {
    usleep(1000);
  }
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
  bool first = true;
  do {
    try {
      auto response = NetworkManager::get().receive_from(m_connection_name, ipm::Receiver::s_no_block);

      TLOG_DEBUG(25) << "Received " << response.data.size() << " bytes. Dispatching to callback.";

      if (m_callback != nullptr) {
        m_callback(response);
      }
    } catch (ipm::ReceiveTimeoutExpired const& tmo) {
      usleep(10000);
    }

    // All initialization complete
    if (first) {
      m_is_listening = true;
      first = false;
    }
  } while (m_is_listening.load());
}

} // namespace dunedaq::networkmanager