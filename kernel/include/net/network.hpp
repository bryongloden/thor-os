//=======================================================================
// Copyright Baptiste Wicht 2013-2016.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://www.opensource.org/licenses/MIT)
//=======================================================================

#ifndef NETWORK_H
#define NETWORK_H

#include <types.hpp>
#include <string.hpp>
#include <circular_buffer.hpp>
#include <mutex.hpp>
#include <semaphore.hpp>
#include <lock_guard.hpp>
#include <tuple.hpp>
#include <expected.hpp>

#include "tlib/net_constants.hpp"

#include "net/ethernet_packet.hpp"

namespace network {

using socket_fd_t = size_t;

struct interface_descriptor {
    size_t id;                       ///< The interface ID
    bool enabled;                    ///< true if the interface is enabled
    std::string name;                ///< The name of the interface
    std::string driver;              ///< The driver of the interface
    size_t pci_device;               ///< The pci information
    size_t mac_address;              ///< The inteface MAC address
    void* driver_data;               ///<  The driver data
    network::ip::address ip_address; ///< The interface IP address
    network::ip::address gateway;    ///< The interface IP gateway

    mutable mutex<> tx_lock; //To synchronize the queue
    mutable semaphore tx_sem;
    mutable semaphore rx_sem;

    size_t rx_thread_pid;
    size_t tx_thread_pid;

    circular_buffer<ethernet::packet, 32> rx_queue;
    circular_buffer<ethernet::packet, 32> tx_queue;

    void (*hw_send)(interface_descriptor&, ethernet::packet& p);

    void send(ethernet::packet& p){
        std::lock_guard<mutex<>> l(tx_lock);
        tx_queue.push(p);
        tx_sem.release();
    }

    bool is_loopback() const {
        return driver == "loopback";
    }
};

void init();        // Called early on
void finalize();    // Called after scheduler is initialized

size_t number_of_interfaces();

interface_descriptor& interface(size_t index);

/*!
 * \brief Open a new socket
 * \param domain The socket domain
 * \param type The socket type
 * \param protocol The socket protocol
 * \return The file descriptor on success, a negative error code otherwise
 */
std::expected<socket_fd_t> open(network::socket_domain domain, network::socket_type type, network::socket_protocol protocol);

/*!
 * \brief Close the given socket file descriptor
 */
void close(size_t fd);

/*!
 * \brief Prepare a packet
 * \param socket_fd The file descriptor of the packet
 * \param desc The packet descriptor to send (depending on the protocol)
 * \þaram buffer The buffer to hold the packet payload
 * \return a tuple containing the packet file descriptor and the packet payload index
 */
std::tuple<size_t, size_t> prepare_packet(socket_fd_t socket_fd, void* desc, char* buffer);

/*!
 * \brief Finalize a packet (send it)
 * \param socket_fd The file descriptor of the packet
 * \param packet_fd The file descriptor of the packet
 * \return 0 on success and a negative error code otherwise
 */
std::expected<void> finalize_packet(socket_fd_t socket_fd, size_t packet_fd);

/*!
 * \brief Listen to a socket or not
 * \param socket_fd The file descriptor of the packet
 * \param listen Indicates if listen or not
 * \return 0 on success and a negative error code otherwise
 */
std::expected<void> listen(socket_fd_t socket_fd, bool listen);

/*!
 * \brief Bind a socket datagram as a client (bind a local random port)
 * \param socket_fd The file descriptor of the packet
 * \return the allocated port on success and a negative error code otherwise
 */
std::expected<size_t> client_bind(socket_fd_t socket_fd);

/*!
 * \brief Bind a socket stream as a client (bind a local random port)
 * \param socket_fd The file descriptor of the packet
 * \param server The ip address of the server
 * \param port The port of the server
 * \return the allocated port on success and a negative error code otherwise
 */
std::expected<size_t> connect(socket_fd_t socket_fd, network::ip::address address, size_t port);

/*!
 * \brief Disconnect from  a socket stream
 * \param socket_fd The file descriptor of the packet
 * \return the allocated port on success and a negative error code otherwise
 */
std::expected<void> disconnect(socket_fd_t socket_fd);

/*!
 * \brief Wait for a packet
 * \param socket_fd The file descriptor of the packet
 * \return the packet index
 */
std::expected<size_t> wait_for_packet(char* buffer, socket_fd_t socket_fd);

/*!
 * \brief Wait for a packet, for some time
 * \param socket_fd The file descriptor of the packet
 * \param ms The maximum time, in milliseconds, to wait for a packet
 * \return the packet index
 */
std::expected<size_t> wait_for_packet(char* buffer, socket_fd_t socket_fd, size_t ms);

void propagate_packet(const ethernet::packet& packet, socket_protocol protocol);

} // end of network namespace

#endif
