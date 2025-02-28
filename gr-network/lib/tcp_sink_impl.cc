/* -*- c++ -*- */
/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tcp_sink_impl.h"
#include <gnuradio/io_signature.h>

#include <chrono>
#include <sstream>
#include <thread>

namespace gr {
namespace network {

tcp_sink::sptr tcp_sink::make(
    size_t itemsize, size_t veclen, const std::string& host, int port, int sinkmode)
{
    return gnuradio::make_block_sptr<tcp_sink_impl>(
        itemsize, veclen, host, port, sinkmode);
}

/*
 * The private constructor
 */
tcp_sink_impl::tcp_sink_impl(
    size_t itemsize, size_t veclen, const std::string& host, int port, int sinkmode)
    : gr::sync_block("tcp_sink",
                     gr::io_signature::make(1, 1, itemsize * veclen),
                     gr::io_signature::make(0, 0, 0)),
      d_itemsize(itemsize),
      d_veclen(veclen),
      d_host(host),
      d_port(port),
      d_sinkmode(sinkmode),
      d_thread_running(false),
      d_stop_thread(false),
      d_listener_thread(NULL),
      d_start_new_listener(false),
      d_initial_connection(true)
{
    d_block_size = d_itemsize * d_veclen;
}

bool tcp_sink_impl::start()
{
    if (d_sinkmode == TCPSINKMODE_CLIENT) {
        // In this mode, we're connecting to a remote TCP service listener
        // as a client.
        d_logger->info("[TCP Sink] connecting to {:s} on port {:d}", d_host, d_port);

        asio::error_code err;
        d_tcpsocket = new asio::ip::tcp::socket(d_io_context);

        std::string s_port = std::to_string(d_port);
        asio::ip::tcp::resolver resolver(d_io_context);
        asio::ip::tcp::resolver::query query(
            d_host, s_port, asio::ip::resolver_query_base::passive);

        d_endpoint = *resolver.resolve(query, err);

        if (err) {
            throw std::runtime_error(
                std::string("[TCP Sink] Unable to resolve host/IP: ") + err.message());
        }

        if (d_host.find(":") != std::string::npos)
            d_is_ipv6 = true;
        else {
            // This block supports a check that a name rather than an IP is provided.
            // the endpoint is then checked after the resolver is done.
            if (d_endpoint.address().is_v6())
                d_is_ipv6 = true;
            else
                d_is_ipv6 = false;
        }

        d_tcpsocket->connect(d_endpoint, err);
        if (err) {
            throw std::runtime_error(std::string("[TCP Sink] Connection error: ") +
                                     err.message());
        }

        d_connected = true;

        asio::socket_base::keep_alive option(true);
        d_tcpsocket->set_option(option);
    } else {
        // In this mode, we're starting a local port listener and waiting
        // for inbound connections.
        d_start_new_listener = true;
        d_listener_thread = new std::thread([this] { run_listener(); });
    }

    return true;
}

void tcp_sink_impl::run_listener()
{
    d_thread_running = true;

    while (!d_stop_thread) {
        // this will block
        if (d_start_new_listener) {
            d_start_new_listener = false;
            connect(d_initial_connection);
            d_initial_connection = false;
        } else
            std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    d_thread_running = false;
}

void tcp_sink_impl::accept_handler(asio::ip::tcp::socket* new_connection,
                                   const asio::error_code& error)
{
    if (!error) {
        d_logger->info("Client connection received.");

        // Accept succeeded.
        d_tcpsocket = new_connection;

        asio::socket_base::keep_alive option(true);
        d_tcpsocket->set_option(option);
        d_connected = true;

    } else {
        d_logger->error("Error code {:s} accepting TCP session.", error.message());

        // Boost made a copy so we have to clean up
        delete new_connection;

        // safety settings.
        d_connected = false;
        d_tcpsocket = NULL;
    }
}

void tcp_sink_impl::connect(bool initial_connection)
{
    d_logger->info("Waiting for connection on port {:d}", d_port);

    if (initial_connection) {
        if (d_is_ipv6)
            d_acceptor = new asio::ip::tcp::acceptor(
                d_io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), d_port));
        else
            d_acceptor = new asio::ip::tcp::acceptor(
                d_io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), d_port));
    } else {
        d_io_context.reset();
    }

    if (d_tcpsocket) {
        delete d_tcpsocket;
    }
    d_tcpsocket = NULL;
    d_connected = false;

    asio::ip::tcp::socket* tmpSocket = new asio::ip::tcp::socket(d_io_context);
    d_acceptor->async_accept(*tmpSocket,
                             [this, tmpSocket](const asio::error_code& error) {
                                 accept_handler(tmpSocket, error);
                             });

    d_io_context.run();
}

/*
 * Our virtual destructor.
 */
tcp_sink_impl::~tcp_sink_impl() { stop(); }

bool tcp_sink_impl::stop()
{
    if (d_thread_running) {
        d_stop_thread = true;
    }

    if (d_tcpsocket) {
        d_tcpsocket->close();
        delete d_tcpsocket;
        d_tcpsocket = NULL;
    }

    d_io_context.reset();
    d_io_context.stop();

    if (d_acceptor) {
        delete d_acceptor;
        d_acceptor = NULL;
    }

    if (d_listener_thread) {
        while (d_thread_running)
            std::this_thread::sleep_for(std::chrono::microseconds(5));

        delete d_listener_thread;
        d_listener_thread = NULL;
    }

    return true;
}

int tcp_sink_impl::work(int noutput_items,
                        gr_vector_const_void_star& input_items,
                        gr_vector_void_star& output_items)
{
    gr::thread::scoped_lock guard(d_setlock);

    if (!d_connected)
        return noutput_items;

    unsigned int noi = noutput_items * d_block_size;
    int bytes_written;
    int bytes_remaining = noi;

    ec.clear();

    char* p_buff;
    p_buff = (char*)input_items[0];

    while ((bytes_remaining > 0) && (!ec)) {
        bytes_written = asio::write(
            *d_tcpsocket, asio::buffer((const void*)p_buff, bytes_remaining), ec);
        bytes_remaining -= bytes_written;
        p_buff += bytes_written;

        if (ec == asio::error::connection_reset || ec == asio::error::broken_pipe) {

            // Connection was reset
            d_connected = false;
            bytes_remaining = 0;

            if (d_sinkmode == TCPSINKMODE_CLIENT) {
                d_logger->warn("Server closed the connection. Stopping processing.");

                return WORK_DONE;
            } else {
                d_logger->info("Client disconnected. Waiting for new connection.");

                // start waiting for another connection
                d_start_new_listener = true;
            }
        }
    }

    return noutput_items;
}
} /* namespace network */
} /* namespace gr */
