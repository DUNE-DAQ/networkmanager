#include "networkmanager/Subscriber.hpp"
#include "networkmanager/NetworkManager.hpp"

namespace dunedaq::networkmanager {
Subscriber::Subscriber(std::string const& connection_name)
  : m_connection_name(connection_name)
{}

Subscriber::~Subscriber() noexcept
{
  shutdown();
}

Subscriber::Subscriber(Subscriber&& other)
  : m_connection_name(other.m_connection_name)
  , m_callbacks(std::move(other.m_callbacks))
  , m_subscriber_thread(std::move(other.m_subscriber_thread))
  , m_is_running(other.m_is_running.load())
{}

Subscriber&
Subscriber::operator=(Subscriber&& other)
{
  m_connection_name = other.m_connection_name;
  m_callbacks = std::move(other.m_callbacks);
  m_subscriber_thread = std::move(other.m_subscriber_thread);
  m_is_running = other.m_is_running.load();
  return *this;
}

void
Subscriber::add_callback(std::function<void(ipm::Receiver::Response)> callback, std::string const& topic)
{
  if (has_callback(topic)) {
    throw CallbackAlreadyRegistered(ERS_HERE, m_connection_name, topic);
  }

  m_callbacks[topic] = callback;
  if (!is_running())
    startup();
}

void
Subscriber::remove_callback(std::string const& topic)
{
  if (!has_callback(topic)) {
    throw CallbackNotRegistered(ERS_HERE, m_connection_name, topic);
  }

  m_callbacks.erase(topic);
  if (m_callbacks.empty())
    shutdown();
}

bool
Subscriber::has_callback(std::string const& topic) const
{
  return m_callbacks.count(topic);
}

std::unordered_set<std::string> 
Subscriber::topics() const
{
  std::unordered_set<std::string> output;
  for (auto& topic_pair : m_callbacks) {
    output.insert(topic_pair.first);
  }
  return output;
}

void
Subscriber::startup()
{
  m_is_running = true;
  m_subscriber_thread.reset(new std::thread([&] { subscriber_thread_loop(); }));
}

void
Subscriber::shutdown()
{
  m_is_running = false;
  if (m_subscriber_thread && m_subscriber_thread->joinable())
    m_subscriber_thread->join();
  m_callbacks.clear();
}

void
Subscriber::subscriber_thread_loop()
{
  while (m_is_running) {
    try {
      auto response = NetworkManager::get().receive_from(m_connection_name, ipm::Receiver::s_no_block);

      if (m_callbacks.count(response.metadata)) {
        m_callbacks[response.metadata](response);
      }
      if (response.metadata != "" && m_callbacks.count("")) {
        m_callbacks[""](response);
      }
    } catch (ipm::ReceiveTimeoutExpired const& tmo) {
      usleep(1000);
    }
  }
}

} // namespace dunedaq::networkmanager