/**
 * @file Issues.hpp ERS issues used by the toolbox functions and classes
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef NETWORKMANAGER_INCLUDE_NETWORKMANAGER_ISSUES_HPP_
#define NETWORKMANAGER_INCLUDE_NETWORKMANAGER_ISSUES_HPP_

#include <ers/Issue.hpp>

#include <string>

namespace dunedaq {

// Disable coverage collection LCOV_EXCL_START

ERS_DECLARE_ISSUE(networkmanager, OperationFailed, message, ((std::string)message))

ERS_DECLARE_ISSUE(networkmanager, ConnectionNotFound, "Connection named " << name << " not found", ((std::string)name))
ERS_DECLARE_ISSUE(networkmanager, TopicNotFound, "Topic named " << name << " not found for connection " << connection, ((std::string)name)((std::string)connection))
ERS_DECLARE_ISSUE(networkmanager, NameCollision, "Multiple instances of name " << name << " exist", ((std::string)name))

ERS_DECLARE_ISSUE(networkmanager,
                  ConnectionAlreadyOpen,
                  "Connection named " << name << " has already been opened for " << direction,
                  ((std::string)name)((std::string)direction))
ERS_DECLARE_ISSUE(networkmanager,
                  ConnectionNotOpen,
                  "Connection named " << name << " is not open for " << direction,
                  ((std::string)name)((std::string)direction))
ERS_DECLARE_ISSUE(networkmanager, NetworkManagerAlreadyConfigured, "The NetworkManager has already been configured", )
ERS_DECLARE_ISSUE(networkmanager,
                  ListenerAlreadyRegistered,
                  "A listener callback has already been registered for name " << name,
                  ((std::string)name))
ERS_DECLARE_ISSUE(networkmanager,
                  ListenerNotRegistered,
                  "No listener has been registered with name " << name,
                  ((std::string)name))
// Reenable coverage collection LCOV_EXCL_STOP
} // namespace dunedaq

#endif // NETWORKMANAGER_INCLUDE_NETWORKMANAGER_ISSUES_HPP_
