/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rtorres <rtorres@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/27 10:25:22 by rtorres           #+#    #+#             */
/*   Updated: 2025/03/11 15:59:39 by rtorres          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sstream>
#include <cstdio>
#include <map>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fstream>
#include <csignal>
#include "Client.hpp"
#include "Channel.hpp"

#define DEBUG false
#define BACKLOG 100
#define BUFFER_SIZE 1024

class Channel;

class Server {
private:
    int listener;
    std::map<int, Client *> clients;
    std::map<std::string, Client *> registeredUsers;
    std::map<std::string, Channel *> channels;
    std::vector<struct pollfd> pfds;
    std::string port;
    std::string password;
    bool running;
    std::ofstream logFile;

    void setupSocket();
    void addClient(int newfd, const std::string &ip);
    void removeClient(int fd);
    void handleNewConnection();
    void handleClientMessage(int client_fd);
    void parseCommand(Client *client, const std::string &message);
    void sendToClient(int client_fd, const std::string &message);
    void broadcastMessage(const std::string &message, int exclude_fd = -1);
    void logMessage(const std::string &message);
    void handlePING(Client *client, const std::vector<std::string> &params);
    void handlePASS(Client *client, const std::vector<std::string> &params);
    void handleUSER(Client *client, const std::vector<std::string> &params);
    void handleNICK(Client *client, const std::vector<std::string> &params);
    void handleJOIN(Client *client, const std::vector<std::string> &params);
    void handlePRIVMSG(Client *client, const std::vector<std::string> &params);
    void handleMODE(Client *client, const std::vector<std::string> &params);
    void handleQUIT(Client *client, const std::vector<std::string> &params);
    void handlePART(Client *client, const std::vector<std::string> &params);
    void handleTOPIC(Client *client, const std::vector<std::string> &params);
    void handleKICK(Client *client, const std::vector<std::string> &params);
    void handleINVITE(Client *client, const std::vector<std::string> &params);
public:
    Server(const std::string &port, const std::string &password);
    ~Server();
    void shutdownServer();
    void run();
};

typedef void (Server::*t_handlers)(Client *client, const std::vector<std::string> &params);

#endif
