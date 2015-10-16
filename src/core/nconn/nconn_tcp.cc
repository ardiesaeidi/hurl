//: ----------------------------------------------------------------------------
//: Copyright (C) 2014 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    nconn_tcp.cc
//: \details: TODO
//: \author:  Reed P. Morrison
//: \date:    02/07/2014
//:
//:   Licensed under the Apache License, Version 2.0 (the "License");
//:   you may not use this file except in compliance with the License.
//:   You may obtain a copy of the License at
//:
//:       http://www.apache.org/licenses/LICENSE-2.0
//:
//:   Unless required by applicable law or agreed to in writing, software
//:   distributed under the License is distributed on an "AS IS" BASIS,
//:   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//:   See the License for the specific language governing permissions and
//:   limitations under the License.
//:
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: Includes
//: ----------------------------------------------------------------------------
#include "nconn_tcp.h"
#include "time_util.h"
#include "evr.h"
#include "ndebug.h"

// Fcntl and friends
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>

//: ----------------------------------------------------------------------------
//: Macros
//: ----------------------------------------------------------------------------
// Set socket option macro...
#define SET_SOCK_OPT(_sock_fd, _sock_opt_level, _sock_opt_name, _sock_opt_val) \
        do { \
                int _l__sock_opt_val = _sock_opt_val; \
                int _l_status = 0; \
                _l_status = ::setsockopt(_sock_fd, \
                                _sock_opt_level, \
                                _sock_opt_name, \
                                &_l__sock_opt_val, \
                                sizeof(_l__sock_opt_val)); \
                if (_l_status == -1) { \
                        NDBG_PRINT("STATUS_ERROR: Failed to set sock_opt: %s.  Reason: %s.\n", #_sock_opt_name, strerror(errno)); \
                        return NC_STATUS_ERROR;\
                } \
        } while(0)

namespace ns_hlx {

//: ----------------------------------------------------------------------------
//: Fwd Decl's
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nconn_tcp::set_opt(uint32_t a_opt, const void *a_buf, uint32_t a_len)
{
        switch(a_opt)
        {
        case OPT_TCP_RECV_BUF_SIZE:
        {
                m_sock_opt_recv_buf_size = a_len;
                break;
        }
        case OPT_TCP_SEND_BUF_SIZE:
        {
                m_sock_opt_send_buf_size = a_len;
                break;
        }
        case OPT_TCP_NO_DELAY:
        {
                m_sock_opt_no_delay = (bool)a_len;
                break;
        }
        default:
        {
                //NDBG_PRINT("Error unsupported option: %d\n", a_opt);
                return NC_STATUS_UNSUPPORTED;
        }
        }

        return NC_STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nconn_tcp::get_opt(uint32_t a_opt, void **a_buf, uint32_t *a_len)
{

        switch(a_opt)
        {
        default:
        {
                //NDBG_PRINT("Error unsupported option: %d\n", a_opt);
                return NC_STATUS_UNSUPPORTED;
        }
        }

        return NC_STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nconn_tcp::ncset_listening(evr_loop *a_evr_loop, int32_t a_val)
{
        m_fd = a_val;
        m_tcp_state = TCP_STATE_LISTENING;

        int l_opt = 1;
        ioctl(m_fd, FIONBIO, &l_opt);
#if 0
        // -------------------------------------------
        // Can set with set_sock_opt???
        // -------------------------------------------
        // Set the file descriptor to no-delay mode.
        const int flags = fcntl(m_fd, F_GETFL, 0);
        if (flags == -1)
        {
                NCONN_ERROR("HOST[%s]: Error getting flags for fd. Reason: %s\n", m_host.c_str(), strerror(errno));
                return NC_STATUS_ERROR;
        }

        if (fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0)
        {
                NCONN_ERROR("HOST[%s]: Error setting fd to non-block mode. Reason: %s\n", m_host.c_str(), strerror(errno));
                return NC_STATUS_ERROR;
        }
#endif

        // Add to event handler
        if (0 != a_evr_loop->add_fd(a_val,
                                    EVR_FILE_ATTR_MASK_READ|EVR_FILE_ATTR_MASK_RD_HUP,
                                    this))
        {
                NDBG_PRINT("Error: Couldn't add socket file descriptor\n");
                return NC_STATUS_ERROR;
        }

        return NC_STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nconn_tcp::ncset_accepting(evr_loop *a_evr_loop, int a_fd)
{
        m_fd = a_fd;
        m_tcp_state = TCP_STATE_ACCEPTING;

        // TODO Hack...
        // -------------------------------------------
        // Socket options
        // -------------------------------------------
        if(m_sock_opt_send_buf_size)
        {
                SET_SOCK_OPT(m_fd, SOL_SOCKET, SO_SNDBUF, m_sock_opt_send_buf_size);
        }

        if(m_sock_opt_recv_buf_size)
        {
                SET_SOCK_OPT(m_fd, SOL_SOCKET, SO_RCVBUF, m_sock_opt_recv_buf_size);
        }

        if(m_sock_opt_no_delay)
        {
                SET_SOCK_OPT(m_fd, SOL_TCP, TCP_NODELAY, 1);
        }

        // Using accept4 -TODO portable???
#if 0
        // -------------------------------------------
        // Can set with set_sock_opt???
        // -------------------------------------------
        // Set the file descriptor to no-delay mode.
        const int flags = fcntl(m_fd, F_GETFL, 0);
        if (flags == -1)
        {
                NCONN_ERROR("HOST[%s]: Error getting flags for fd. Reason: %s\n", m_host.c_str(), strerror(errno));
                return NC_STATUS_ERROR;
        }

        if (fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0)
        {
                NCONN_ERROR("HOST[%s]: Error setting fd to non-block mode. Reason: %s\n", m_host.c_str(), strerror(errno));
                return NC_STATUS_ERROR;
        }
#endif

        // Add to event handler
        if (0 != a_evr_loop->add_fd(m_fd,
                                    EVR_FILE_ATTR_MASK_READ|EVR_FILE_ATTR_MASK_RD_HUP|EVR_FILE_ATTR_MASK_ET,
                                    this))
        {
                NDBG_PRINT("Error: Couldn't add socket file descriptor\n");
                return NC_STATUS_ERROR;
        }

        return NC_STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nconn_tcp::ncread(char *a_buf, uint32_t a_buf_len)
{
        ssize_t l_status;
        int32_t l_bytes_read = 0;

        //l_status = read(m_fd, a_buf, a_buf_len);
        l_status = recvfrom(m_fd, a_buf, a_buf_len, 0, NULL, NULL);

        //NDBG_PRINT("%sHOST%s: %s fd[%3d] READ: %ld bytes. Reason: %s\n",
        //                ANSI_COLOR_FG_RED, ANSI_COLOR_OFF,
        //                m_host.c_str(),
        //                m_fd,
        //                l_status,
        //                strerror(errno));
        if(l_status > 0)
        {
                l_bytes_read += l_status;
                return l_bytes_read;
        }
        else if(l_status == 0)
        {
                //NDBG_PRINT("l_status: %ld --errno: %d -%s\n", l_status, errno, strerror(errno));
                return NC_STATUS_EOF;
        }
        else
        {
                switch(errno)
                {
                        case EAGAIN:
                        {
                                return NC_STATUS_AGAIN;
                        }
                        case ECONNRESET:
                        {
                                return NC_STATUS_ERROR;
                        }
                        default:
                        {
                                return NC_STATUS_ERROR;
                        }
                }
        }
        return l_bytes_read;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nconn_tcp::ncwrite(char *a_buf, uint32_t a_buf_len)
{
        int l_status;
        //NDBG_PRINT("%swrite%s: buf: %p fd: %d len: %d\n", ANSI_COLOR_BG_GREEN, ANSI_COLOR_OFF,
        //                l_buf + l_bytes_written, m_fd, a_buf_len - l_bytes_written);
        //mem_display((const uint8_t*)(a_buf), (uint32_t)(a_buf_len));
        l_status = send(m_fd, a_buf, a_buf_len, MSG_NOSIGNAL);
        //NDBG_PRINT("write: status: %d\n", l_status);
        if(l_status < 0)
        {
                NDBG_PRINT("HOST[%s]: Error: performing write.  Reason: %s.\n", m_host.c_str(), strerror(errno));
                //NCONN_ERROR("HOST[%s]: Error: performing write.  Reason: %s.\n", m_host.c_str(), strerror(errno));
                return NC_STATUS_ERROR;
        }
        // TODO REMOVE
        //else if((uint32_t)l_status < a_buf_len)
        //{
        //        NDBG_PRINT("l_status[%d] < a_buf_len[%u].\n", l_status, a_buf_len);
        //}
        // TODO EAGAIN
        return l_status;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nconn_tcp::ncsetup(evr_loop *a_evr_loop)
{
        // Make a socket.
        m_fd = socket(m_host_info.m_sock_family,
                      m_host_info.m_sock_type,
                      m_host_info.m_sock_protocol);

        //NDBG_OUTPUT("%sSOCKET %s[%3d]: \n", ANSI_COLOR_BG_BLUE, ANSI_COLOR_OFF, m_fd);

        if (m_fd < 0)
        {
                NCONN_ERROR("HOST[%s]: Error creating socket. Reason: %s\n", m_host.c_str(), strerror(errno));
                return NC_STATUS_ERROR;
        }

        // -------------------------------------------
        // Socket options
        // -------------------------------------------
        // TODO --set to REUSE????
        SET_SOCK_OPT(m_fd, SOL_SOCKET, SO_REUSEADDR, 1);
        if(m_sock_opt_send_buf_size)
        {
                SET_SOCK_OPT(m_fd, SOL_SOCKET, SO_SNDBUF, m_sock_opt_send_buf_size);
        }

        if(m_sock_opt_recv_buf_size)
        {
                SET_SOCK_OPT(m_fd, SOL_SOCKET, SO_RCVBUF, m_sock_opt_recv_buf_size);
        }

        if(m_sock_opt_no_delay)
        {
                SET_SOCK_OPT(m_fd, SOL_TCP, TCP_NODELAY, 1);
        }

        // -------------------------------------------
        // Can set with set_sock_opt???
        // -------------------------------------------
        // Set the file descriptor to no-delay mode.
        const int flags = fcntl(m_fd, F_GETFL, 0);
        if (flags == -1)
        {
                NCONN_ERROR("HOST[%s]: Error getting flags for fd. Reason: %s\n", m_host.c_str(), strerror(errno));
                return NC_STATUS_ERROR;
        }

        if (fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0)
        {
                NCONN_ERROR("HOST[%s]: Error setting fd to non-block mode. Reason: %s\n", m_host.c_str(), strerror(errno));
                return NC_STATUS_ERROR;
        }

        if (0 != a_evr_loop->add_fd(m_fd,
                                    EVR_FILE_ATTR_MASK_READ|EVR_FILE_ATTR_MASK_RD_HUP|EVR_FILE_ATTR_MASK_ET,
                                    this))
        {
                NCONN_ERROR("HOST[%s]: Error: Couldn't add socket file descriptor\n", m_host.c_str());
                return NC_STATUS_ERROR;
        }

        return NC_STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nconn_tcp::ncaccept(evr_loop *a_evr_loop)
{
        if(m_tcp_state == TCP_STATE_LISTENING)
        {
                int l_client_sock_fd;
                sockaddr_in l_client_address;
                uint32_t l_sockaddr_in_length;

                //NDBG_PRINT("%sRUN_STATE_MACHINE%s: ACCEPT[%d]\n", ANSI_COLOR_BG_RED, ANSI_COLOR_OFF, m_fd);

                //Set the size of the in-out parameter
                l_sockaddr_in_length = sizeof(sockaddr_in);

                //Wait for a client to connect
                // TODO -portable???
                l_client_sock_fd = accept4(m_fd, (struct sockaddr *)&l_client_address, &l_sockaddr_in_length, SOCK_NONBLOCK);
                if (l_client_sock_fd < 0)
                {
                        //NDBG_PRINT("Error accept failed. Reason[%d]: %s\n", errno, strerror(errno));
                        return NC_STATUS_ERROR;
                }
                return l_client_sock_fd;
        }
        m_tcp_state = TCP_STATE_CONNECTED;
        return nconn::NC_STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nconn_tcp::ncconnect(evr_loop *a_evr_loop)
{
        uint32_t l_retry_connect_count = 0;
        int l_connect_status = 0;

        // Set to connecting
        m_tcp_state = TCP_STATE_CONNECTING;

state_top:
        l_connect_status = connect(m_fd,
                                   (struct sockaddr*) &(m_host_info.m_sa),
                                   (m_host_info.m_sa_len));

        // TODO REMOVE
        //NDBG_PRINT_BT();
        //NDBG_PRINT("%sCONNECT%s[%3d]: Retry: %d Status %3d. Reason[%d]: %s\n",
        //           ANSI_COLOR_FG_CYAN, ANSI_COLOR_OFF,
        //           m_fd, l_retry_connect_count, l_connect_status,
        //           errno,
        //           strerror(errno));

        if (l_connect_status < 0)
        {
                switch (errno)
                {
                case EISCONN:
                {
                        // Is already connected drop out of switch and return OK
                        break;
                }
                case EINVAL:
                {
                        int l_err;
                        socklen_t l_errlen;
                        l_errlen = sizeof(l_err);
                        if (getsockopt(m_fd, SOL_SOCKET, SO_ERROR, (void*) &l_err, &l_errlen) < 0)
                        {
                                NCONN_ERROR("HOST[%s]: Error performing getsockopt. Unknown connect error\n",
                                                m_host.c_str());
                        }
                        else
                        {
                                NCONN_ERROR("HOST[%s]: Error performing getsockopt. %s\n", m_host.c_str(),
                                                strerror(l_err));
                        }
                        return NC_STATUS_ERROR;
                }
                case ECONNREFUSED:
                {
                        //if(m_verbose)
                        //{
                        //        NCONN_ERROR("HOST[%s]: Error Connection refused. Reason: %s\n", m_host.c_str(), strerror(errno));
                        //}
                        return NC_STATUS_ERROR;
                }
                case EAGAIN:
                case EINPROGRESS:
                {
                        //NDBG_PRINT("Error Connection in progress. Reason: %s\n", strerror(errno));

                        // Set to writeable and try again
                        if (0 != a_evr_loop->mod_fd(m_fd,
                                                    EVR_FILE_ATTR_MASK_WRITE|EVR_FILE_ATTR_MASK_ET,
                                                    this))
                        {
                                NCONN_ERROR("HOST[%s]: Error: Couldn't add socket file descriptor\n", m_host.c_str());
                                return NC_STATUS_ERROR;
                        }

                        // Return here -still in connecting state
                        return NC_STATUS_OK;
                }
                case EADDRNOTAVAIL:
                {
                        // TODO -bad to spin like this???
                        // Retry connect
                        //NDBG_PRINT("%sRETRY CONNECT%s\n", ANSI_COLOR_BG_YELLOW, ANSI_COLOR_OFF);
                        if(++l_retry_connect_count < 1024)
                        {
                                usleep(1000);
                                goto state_top;
                        }
                        else
                        {
                                NCONN_ERROR("HOST[%s]: Error connect().  Reason: %s\n", m_host.c_str(), strerror(errno));
                                return NC_STATUS_ERROR;
                        }
                        break;
                }
                default:
                {
                        NCONN_ERROR("HOST[%s]: Error Unkown. Reason: %s\n", m_host.c_str(), strerror(errno));
                        return NC_STATUS_ERROR;
                }
                }
        }

        // Set to connected state
        m_tcp_state = TCP_STATE_CONNECTED;

        // TODO Stats???
        //if(m_collect_stats_flag)
        //{
        //        m_stat.m_tt_connect_us = get_delta_time_us(m_connect_start_time_us);
        //        // Save last connect time for reuse
        //        m_last_connect_time_us = m_stat.m_tt_connect_us;
        //}

        // Add to readable
        if (0 != a_evr_loop->mod_fd(m_fd,
                                    EVR_FILE_ATTR_MASK_READ|EVR_FILE_ATTR_MASK_RD_HUP|EVR_FILE_ATTR_MASK_ET,
                                    this))
        {
                NCONN_ERROR("HOST[%s]: Error: Couldn't add socket file descriptor\n", m_host.c_str());
                return NC_STATUS_ERROR;
        }

        return NC_STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nconn_tcp::nccleanup(void)
{
        // Shut down connection
        //NDBG_PRINT("CLOSE[%d] %s--CONN--%s last_state: %d\n", m_fd, ANSI_COLOR_BG_RED, ANSI_COLOR_OFF, m_tcp_state);
        //NDBG_PRINT_BT();
        close(m_fd);
        m_fd = -1;

        // Reset all the values
        // TODO Make init function...
        // Set num use back to zero -we need reset method here?
        m_tcp_state = TCP_STATE_FREE;
        m_num_reqs = 0;

        return NC_STATUS_OK;
}

} //namespace ns_hlx {