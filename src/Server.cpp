/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rtorres <rtorres@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/27 10:25:12 by rtorres           #+#    #+#             */
/*   Updated: 2025/03/11 15:57:20 by rtorres          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"

Server::Server(const std::string &port, const std::string &password) 
    : port(port), password(password), running(true) {
    setupSocket();
    logFile.open("server.log", std::ios::app);
}

Server::~Server() {
    logFile.close();
    if (listener >= 0) {
        close(listener);
        listener = -1;
    }
    std::map<int, Client *>::iterator it;
    for (it = clients.begin(); it != clients.end(); ++it) {
        Client *client = it->second;
        if (client) {
            if (client->getSocket() >= 0) {
                close(client->getSocket());
            }
            delete client;
        }
    }
    clients.clear();
}

void Server::setupSocket() {
    struct addrinfo hints, *res;
    int yes = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port.c_str(), &hints, &res) != 0)
        throw std::runtime_error("Error: getaddrinfo failed");
    listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (listener < 0)
        throw std::runtime_error("Error: socket creation failed");
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (bind(listener, res->ai_addr, res->ai_addrlen) < 0)
        throw std::runtime_error("Error: bind failed");
    if (listen(listener, BACKLOG) < 0)
        throw std::runtime_error("Error: listen failed");
    freeaddrinfo(res);
    if (DEBUG)
        std::cout << "DEBUG: Listener socket created: " << listener << std::endl;
    struct pollfd pfd;
    pfd.fd = listener;
    pfd.events = POLLIN;
    pfds.push_back(pfd);
    logMessage("Server started on port " + port);
}

void Server::shutdownServer() {
    std::cout << "Shutting down server..." << std::endl;
    running = false;

    for (std::map<int, Client *>::iterator it = clients.begin(); it != clients.end(); ++it) {
        send(it->first, "Server shutting down. Goodbye!\r\n", 32, 0);
        close(it->first);
        delete it->second;
    }
    clients.clear();
    for (std::map<std::string, Channel *>::iterator it = channels.begin(); it != channels.end(); ++it) {
        delete it->second;
    }
    channels.clear();
    close(listener);
    logMessage("Server is shutting down.");
}

void Server::run() {
    std::cout << "IRC server is running..." << std::endl;
    while (running) {
        int poll_count = poll(&pfds[0], pfds.size(), -1);
        if (poll_count < 0)
            throw std::runtime_error("Error: poll failed");
        for (size_t i = 0; i < pfds.size(); ++i) {
            if (pfds[i].revents & POLLIN) {
                if (pfds[i].fd == listener)
                    handleNewConnection();
                else
                    handleClientMessage(pfds[i].fd);
            }
        }
    }
    shutdownServer();
}

void Server::handlePING(Client *client, const std::vector<std::string> &params) {
    if (params.empty())
    {
        sendToClient(client->getSocket(), "461 PING :Not enough parameters\r\n");
        return;
    }
    std::string response = "PONG :" + params[0] + "\r\n";
    sendToClient(client->getSocket(), response);
}

void Server::handleNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int newfd = accept(listener, (struct sockaddr *)&client_addr, &addr_len);

    if (newfd < 0)
        return;
    std::string ip = inet_ntoa(client_addr.sin_addr);
    addClient(newfd, ip);
    logMessage("New connection from " + ip);
}

void Server::addClient(int newfd, const std::string &ip) {
    if (clients.find(newfd) != clients.end())
        delete clients[newfd];
    Client *client = new Client(newfd, ip);
    clients[newfd] = client;

    struct pollfd pfd;
    pfd.fd = newfd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    pfds.push_back(pfd);
    std::cout << "New client connected from " << ip << " on socket " << newfd << std::endl;
}

void Server::handleClientMessage(int client_fd) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_received <= 0) {
        removeClient(client_fd);
        return;
    }
    Client *client = clients[client_fd];
    client->setBuffer(client->getBuffer() + std::string(buffer, bytes_received));
    std::string &message = client->getBuffer();
    size_t pos;
    while ((pos = message.find_first_of("\r\n")) != std::string::npos) {
        std::string command = message.substr(0, pos);
        if (message[pos] == '\r' && pos + 1 < message.size() && message[pos + 1] == '\n')
            message.erase(0, pos + 2);
        else
            message.erase(0, pos + 1);
        if (DEBUG)
            std::cout << "DEBUG: Raw Command Received: " << command << std::endl;
        if (!command.empty() && command[0] == ':') {
            if (DEBUG)
                std::cout << "DEBUG: Ignored server message: " << command << std::endl;
            continue;
        }
        parseCommand(client, command);
    }
    client->setBuffer(message);
}

void Server::removeClient(int fd) {
    std::map<int, Client *>::iterator it = clients.find(fd);
    if (it != clients.end()) {
        logMessage("Client disconnected: " + it->second->getIpAddress());
        for (std::map<std::string, Channel *>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt)
            chanIt->second->removeUser(fd);
        close(fd);
        delete it->second;
        clients.erase(it);
    }    
    for (std::vector<struct pollfd>::iterator pfdIt = pfds.begin(); pfdIt != pfds.end(); ++pfdIt) {
        if (pfdIt->fd == fd) {
            pfds.erase(pfdIt);
            break;
        }
    }
}

void Server::parseCommand(Client *client, const std::string &message) {
    if (message.empty())
        return;

    std::istringstream iss(message);
    std::vector<std::string> params;
    std::string command;
    iss >> command;

    std::string param;
    while (iss >> param) {
        if (!param.empty() && param[0] == ':') {  
            std::string rest;
            std::getline(iss, rest);
            param = param.substr(1) + (rest.empty() ? "" : " " + rest);  
            params.push_back(param);
            break;
        }
        params.push_back(param);
    }
    if (DEBUG) {
        std::cout << "DEBUG: Command = \"" << command << "\"\n";
        std::cout << "DEBUG: params.size() = " << params.size() << "\n";
        for (size_t i = 0; i < params.size(); ++i)
            std::cout << "DEBUG: params[" << i << "] = \"" << params[i] << "\"\n";
    }
    std::transform(command.begin(), command.end(), command.begin(), static_cast<int(*)(int)>(std::toupper));
    
    std::string commands[] = {"PING", "PASS", "USER", "NICK", "JOIN", "PRIVMSG", "MODE", "QUIT", "PART", "TOPIC", "KICK", "INVITE"};
    t_handlers handlers[] = {&Server::handlePING, &Server::handlePASS, &Server::handleUSER, &Server::handleNICK,
        &Server::handleJOIN, &Server::handlePRIVMSG, &Server::handleMODE, &Server::handleQUIT,
        &Server::handlePART, &Server::handleTOPIC, &Server::handleKICK, &Server::handleINVITE}; 
    
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (command == commands[i]) {
           (this->*handlers[i])(client, params);
            return;
        }
    }
    if (command == "CAP" || command == "WHOIS" || command == "WHO") {
        if (DEBUG)
            std::cout << "DEBUG: Raw Command Ignored: " << command << "\r\n";
    } else
        sendToClient(client->getSocket(), "421 " + command + " :Unknown command\r\n");
}

void Server::sendToClient(int client_fd, const std::string &message) {
    if (send(client_fd, message.c_str(), message.length(), 0) < 0) {
        std::ostringstream oss;
        oss << "Error sending to client " << client_fd;
        logMessage(oss.str());
    }
}

void Server::broadcastMessage(const std::string &message, int exclude_fd) {
    for (std::map<int, Client *>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->first != exclude_fd)
            sendToClient(it->first, message);
    }
}

void Server::logMessage(const std::string &message) {
    logFile << message << std::endl;
    logFile.flush();
}

void Server::handlePASS(Client *client, const std::vector<std::string> &params) {
    if (params.empty()) {
        sendToClient(client->getSocket(), ":server 461 PASS :Not enough parameters\r\n");
        return;
    }
    if (client->isRegistered())
    {
        sendToClient(client->getSocket(), ":server 462 :You may not reregister\r\n");
        return ;
    }
    std::string receivedPassword = params[0];
    if (client->checkPassword(receivedPassword, this->password)) {
        client->setAuthenticated(true);
        std::string nick = client->getNickName();
        if (nick.empty())
            nick = "*";
        sendToClient(client->getSocket(), ":server 001 " + nick + " :Password accepted, proceed to register\r\n");
        usleep(1000);
    } else {
        sendToClient(client->getSocket(), ":server 464 :Password incorrect\r\n");
        close(client->getSocket());
        clients.erase(client->getSocket());
        delete client;
    }
}

void Server::handleNICK(Client *client, const std::vector<std::string> &params) {
    if (!this->password.empty() && !client->isAuthenticated()) {
        sendToClient(client->getSocket(), "462 :You must provide the correct PASS before registering\r\n");
        return ;
    }
    if (params.empty()) {
        sendToClient(client->getSocket(), "431 :No nickname given\r\n");
        return;
    }
    std::string newNick = params[0];
    if (newNick.length() > 9)
    {
        sendToClient(client->getSocket(), "432 " + newNick + " :Nickname must not exceed 9 characters\r\n");
        return ;
    }
    if (!isalpha(newNick[0])) {
        sendToClient(client->getSocket(), "432 " + newNick + " :Invalid nickname format\r\n");
        return;
    }
    for (size_t i = 1; i < newNick.length(); ++i) {
        if (!isalnum(newNick[i]) && newNick[i] != '-' && newNick[i] != '_') {
            sendToClient(client->getSocket(), "432 " + newNick + " :Invalid nickname format\r\n");
            return;
        }
    }
    std::map<int, Client *>::iterator it;
    for (it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getNickName() == newNick) {
            sendToClient(client->getSocket(), "433 " + client->getNickName() + " " + newNick + " :Nickname already in use\r\n");
            return;
        }
    }
    std::string oldNick = client->getNickName();
    client->setNickName(newNick);
    sendToClient(client->getSocket(), ":" + oldNick + "!" + client->getUserName() + "@" + client->getIpAddress() + " NICK " + newNick + "\r\n");
    if (!client->getUserName().empty()) {
        client->setRegistered(true);
        sendToClient(client->getSocket(), "001 " + newNick + " :Welcome to the IRC server\r\n");
    }
}


void Server::handleUSER(Client *client, const std::vector<std::string> &params) {
    if (DEBUG) {
        std::cout << "DEBUG: params.size() = " << params.size() << std::endl;
        for (size_t i = 0; i < params.size(); ++i) {
            std::cout << "DEBUG: paramsss[" << i << "] = \"" << params[i] << "\"" << std::endl;
        }
    }
    if (params.size() < 4) {
        sendToClient(client->getSocket(), "461 USER :Not enough parameters\r\n");
        return;
    }
    if (client->isRegistered()) {
        sendToClient(client->getSocket(), "462 :You may not reregister\r\n");
        return;
    }
    client->setUserName(params[0]);
    std::string realName = params[3];
    if (realName[0] == ':')
        realName = realName.substr(1);
    for (size_t i = 4; i < params.size(); ++i) {
        realName += " " + params[i];
    }
    client->setRealName(realName);
    if (!client->getNickName().empty()) {
        client->setRegistered(true);
        sendToClient(client->getSocket(), "001 " + client->getNickName() + " :Welcome to the IRC server\r\n");
    }
}

void Server::handleQUIT(Client *client, const std::vector<std::string> &params) {
    if (!client)
        return;
    std::string quitMsg = params.empty() ? "Client Quit" : params[0];
    std::map<std::string, Channel *>::iterator it = channels.begin();
    while (it != channels.end()) {
        Channel *channel = it->second;
        if (channel->isUserInChannel(client->getSocket())) {
            std::string quitMessage = ":" + client->getNickName() + " QUIT :" + quitMsg + "\r\n";
            channel->broadcastMessage(quitMessage, client->getSocket());
            channel->removeUser(client->getSocket());
            if (channel->listUsers().empty()) {
                delete channel; 
                std::map<std::string, Channel *>::iterator temp = it; 
                ++it;
                channels.erase(temp);
                continue;
            }
        }
        ++it;
    }
    sendToClient(client->getSocket(), ":" + client->getNickName() + " QUIT :" + quitMsg + "\r\n");
    close(client->getSocket());
}

void Server::handleJOIN(Client *client, const std::vector<std::string> &params) {
    if (params.empty()) {
        sendToClient(client->getSocket(), "461 JOIN :Not enough parameters\r\n");
        return;
    }
    if (!client->isRegistered()) {
        sendToClient(client->getSocket(), "451 JOIN :You have not registered\r\n");
        return;
    }
    std::string channelName = params[0];

    if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '+' &&
        channelName[0] != '!' && channelName[0] != '&')) {
        sendToClient(client->getSocket(), "403 " + channelName + " :No such channel\r\n");
        return;
    }
    if (channelName.length() > 50)
    {
        sendToClient(client->getSocket(), "417 " + channelName + " :channelname must not exceed 50 characters\r\n");
        return ;
    }
    if (channels.find(channelName) == channels.end()) {
        channels[channelName] = new Channel(channelName);
    }
    Channel *channel = channels[channelName];
    if (channel->hasMode('l') && channel->isFull())
    {
        sendToClient(client->getSocket(), "471 " + client->getNickName() + " " + channelName + " :Cannot join channel (+l) - channel is full\r\n");
        return;
    }
    if (channel->hasMode('i') && !client->isInvitedToChannel(channelName)) {
        sendToClient(client->getSocket(), "473 " + client->getNickName() + " " + channelName + " :Cannot join channel (+i)\r\n");
        return;
    }
    if (channel->hasMode('k')) {
        if (params.size() < 2) {
            sendToClient(client->getSocket(), "475 " + client->getNickName() + " " + channelName + " :Cannot join channel (+k) - Missing password\r\n");
            return;
        }        
        std::string providedPassword = params[1];
        if (!channel->checkPassword(providedPassword)) {
            sendToClient(client->getSocket(), "475 " + client->getNickName() + " " + channelName + " :Cannot join channel (+k) - Incorrect password\r\n");
            return;
        }
    } 
    if (channel->isUserInChannel(client->getSocket())) {
        sendToClient(client->getSocket(), "443 " + client->getNickName() + " " + channelName + " :You're already in the channel\r\n");
        return;
    }
    channel->addUser(client);
    std::string joinMessage = ":" + client->getNickName() + "!" + client->getUserName() + "@" + client->getIpAddress() + " JOIN " + channelName + "\r\n";
    sendToClient(client->getSocket(), joinMessage);
    channel->broadcastMessage(joinMessage, client->getSocket());
    if (!channel->getTopic().empty()) {
        sendToClient(client->getSocket(), "332 " + client->getNickName() + " " + channelName + " :" + channel->getTopic() + "\r\n");
    }
    std::vector<std::string> users = channel->listUsers();
    std::string userList = "353 " + client->getNickName() + " @ " + channelName + " :";
    for (size_t i = 0; i < users.size(); i++) {
        Client *userClient = channel->getUserByNick(users[i]);
        if (userClient && channel->isOperator(userClient->getSocket()))
            userList += "@" + users[i] + " ";
        else
            userList += users[i] + " ";
    }
    userList += "\r\n";
    sendToClient(client->getSocket(), userList);
    sendToClient(client->getSocket(), "366 " + client->getNickName() + " " + channelName + " :End of /NAMES list\r\n");
}

void Server::handlePRIVMSG(Client *client, const std::vector<std::string> &params) {
    if (params.size() < 2) {
        sendToClient(client->getSocket(), "461 PRIVMSG :Not enough parameters\r\n");
        return;
    }
    std::string target = params[0];
    std::string message;
    for (size_t i = 1; i < params.size(); i++) {
        message += params[i] + " ";
    }
    message = message.substr(0, message.length() - 1);
    if (message.length() > 256) {
        sendToClient(client->getSocket(), "417 PRIVMSG :Message too long (max 256 characters)\r\n");
        return;
    }
    if (target[0] == '#' || target[0] == '!' || target[0] == '&' || target[0] == '+') { 
        std::map<std::string, Channel*>::iterator channelIt = channels.find(target);
        if (channelIt == channels.end()) {
            sendToClient(client->getSocket(), "403 " + target + " :No such channel\r\n");
            return;
        }
        Channel *channel = channelIt->second;
        if (!channel->isUserInChannel(client->getSocket())) {
            sendToClient(client->getSocket(), "404 " + target + " :Cannot send to channel\r\n");
            return;
        }
        channel->broadcastMessage(":" + client->getNickName() + "!" + "~" + client->getUserName()
            + "@localhost" + " PRIVMSG " + target + " :" + message + "\r\n", client->getSocket());
    } 
    else {
        bool found = false;
        for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
            if (it->second->getNickName() == target) {
                sendToClient(it->second->getSocket(), ":" + client->getNickName() + " PRIVMSG " + target + " :" + message + "\r\n");
                found = true;
                break;
            }
        }
        if (!found) {
            sendToClient(client->getSocket(), "401 " + target + " :No such nick\r\n");
        }
    }
}

void Server::handleMODE(Client *client, const std::vector<std::string> &params) {
    if (params.empty()) {
        sendToClient(client->getSocket(), "461 MODE :Not enough parameters\r\n");
        return;
    }
    std::string target = params[0];
    if (!target.empty() && (target[0] == '#' || target[0] == '!' || target[0] == '&' || target[0] == '+')) {
        std::map<std::string, Channel*>::iterator channelIt = channels.find(target);
        if (channelIt == channels.end()) {
            sendToClient(client->getSocket(), "403 " + target + " :No such channel\r\n");
            return;
        }
        Channel *channel = channelIt->second;
        if (params.size() < 2) {
            std::stringstream ss;
            ss << " Currents modes:" << " i->" << (channel->hasMode('i') ? "yes" : "no")
                << " t->" << (channel->hasMode('t') ? "yes" : "no")
                << " k->" << (channel->hasMode('k') ? "yes" : "no")
                << " l->" <<(channel->hasMode('l') ? "yes" : "no") << "\r\n";
            sendToClient(client->getSocket(), "482 " + target + ss.str());
            return;
        }
        if (!channel->isOperator(client->getSocket())) {
            sendToClient(client->getSocket(), "482 " + target + " :You're not channel operator\r\n");
            return;
        }

        std::string mode = params[1];
        bool adding = mode[0] == '+';
        char modeChar = mode[1];
        std::string modeArg = "";
        if (params.size() >= 3)
            modeArg = params[2];
        else if (mode.length() > 2)
            modeArg = mode.substr(2);
        if (modeChar == 'i') {
            if (adding)
                channel->setMode('i');
            else
                channel->unsetMode('i');
        } 
        else if (modeChar == 't') {
            if (adding) 
                channel->setMode('t');
            else
                channel->unsetMode('t');
        } 
        else if (modeChar == 'k' && params.size() >= 3) {
            if (adding) {
                channel->setMode('k');
                channel->setPassword(params[2]); 
            } else {
                channel->unsetMode('k');
                channel->setPassword("");
            }
        } 
        else if (modeChar == 'o' && params.size() >= 3) {
            Client *targetClient = channel->getUserByNick(params[2]);
            if (!targetClient) {
                sendToClient(client->getSocket(), "401 " + params[2] + " :No such nick\r\n");
                return;
            }
            if (adding)
                channel->addOperator(targetClient->getSocket());
            else
                channel->removeOperator(targetClient->getSocket());
        } 
        else if (modeChar == 'l')
        {
            int limit = modeArg.empty() ? 0 : atoi(modeArg.c_str());
            if (adding && limit > 0) {
                channel->setMode('l');
                channel->setUserLimit(limit);
            } else {
                channel->unsetMode('l');
                channel->setUserLimit(0);
            }
        }
        else {
            sendToClient(client->getSocket(), "501 " + mode + " :Unknown mode flag\r\n");
            return;
        }
        std::string modeMessage = ":" + client->getNickName() + "!" + client->getUserName() + "@127.0.0.1 MODE " + target + " " + mode + "\r\n";
        channel->broadcastMessage(modeMessage, client->getSocket());
        sendToClient(client->getSocket(), modeMessage);
    } 
    else {
        std::map<std::string, Client*>::iterator clientIt = registeredUsers.find(target);
        if (clientIt == registeredUsers.end())
            return;
        Client *targetClient = clientIt->second;
        if (client != targetClient) {
            sendToClient(client->getSocket(), "502 " + target + " :You can't change modes for other users\r\n");
            return;
        }
        if (params.size() < 2) {
            sendToClient(client->getSocket(), "461 MODE :Not enough parameters\r\n");
            return;
        }
        std::string mode = params[1];
        if (mode == "+i") {
            targetClient->setOperator(true);
        } else if (mode == "-i") {
            targetClient->setOperator(false);
        } else {
            sendToClient(client->getSocket(), "501 " + mode + " :Unknown mode flag\r\n");
            return;
        }
        std::string modeMessage = ":" + client->getNickName() + " MODE " + target + " " + mode + "\r\n";
        sendToClient(targetClient->getSocket(), modeMessage);
    }
}

void Server::handlePART(Client *client, const std::vector<std::string> &params) {
    if (!client)
        return;
    if (params.empty()) {
        sendToClient(client->getSocket(), "461 PART :Not enough parameters\r\n");
        return;
    }
    std::string channelName = params[0];
    std::map<std::string, Channel *>::iterator it = channels.find(channelName);
    if (it == channels.end()) {
        sendToClient(client->getSocket(), "403 " + channelName + " :No such channel\r\n");
        return;
    }
    Channel *channel = it->second;
    if (!channel->isUserInChannel(client->getSocket())) {
        sendToClient(client->getSocket(), "442 " + channelName + " :You're not in that channel\r\n");
        return;
    }
    std::string partMessage = ":" + client->getNickName() + "!" + client->getUserName() + "@127.0.0.1 PART " + channelName + "\r\n";
    channel->broadcastMessage(partMessage, client->getSocket());
    sendToClient(client->getSocket(), partMessage);
    channel->removeUser(client->getSocket());
    if (channel->listUsers().empty()) {
        delete channel; 
        channels.erase(it);
    }
}

void Server::handleTOPIC(Client *client, const std::vector<std::string> &params) {
    if (params.empty()) {
        sendToClient(client->getSocket(), "461 TOPIC :Not enough parameters\r\n");
        return;
    }
    std::string channelName = params[0];
    std::map<std::string, Channel *>::iterator it = channels.find(channelName);
    if (it == channels.end()) {
        sendToClient(client->getSocket(), "403 " + channelName + " :No such channel\r\n");
        return;
    }
    Channel *channel = it->second;
    if (!channel->isUserInChannel(client->getSocket())) {
        sendToClient(client->getSocket(), "442 " + channelName + " :You're not in that channel\r\n");
        return;
    }
    if (params.size() == 1) {
        if (channel->getTopic().empty()) {
            sendToClient(client->getSocket(), "331 " + channelName + " :No topic is set\r\n");
        } else {
            sendToClient(client->getSocket(), "332 " + channelName + " :" + channel->getTopic() + "\r\n");
        }
        return;
    }
    if (channel->hasMode('t') && !channel->isOperator(client->getSocket())) {
        sendToClient(client->getSocket(), "482 " + channelName + " :You're not channel operator\r\n");
        return;
    }
    std::string newTopic;
    for (size_t i = 1; i < params.size(); ++i) {
        if (i > 1)
            newTopic += " ";
        newTopic += params[i];
    }
    channel->setTopic(newTopic);    
    std::string topicChangeMsg = ":" + client->getHostname() + " TOPIC " + channelName + " :" + newTopic + "\r\n";
    channel->broadcastMessage(topicChangeMsg, client->getSocket());
    sendToClient(client->getSocket(), topicChangeMsg);
}

void Server::handleKICK(Client *client, const std::vector<std::string> &params) {
    if (params.size() < 2) {
        sendToClient(client->getSocket(), "461 KICK :Not enough parameters\r\n");
        return;
    }
    std::string channelName = params[0];
    std::string targetNick = params[1];
    std::string reason = (params.size() > 2) ? params[2] : "Kicked by operator";

    if (channels.find(channelName) == channels.end()) {
        sendToClient(client->getSocket(), "403 " + channelName + " :No such channel\r\n");
        return;
    }
    Channel *channel = channels[channelName];
    if (!channel->isUserInChannel(client->getSocket())) {
        sendToClient(client->getSocket(), "442 " + channelName + " :You're not on that channel\r\n");
        return;
    }
    if (!channel->isOperator(client->getSocket())) {
        sendToClient(client->getSocket(), "482 " + channelName + " :You're not a channel operator\r\n");
        return;
    }
    Client *targetClient = channel->getUserByNick(targetNick);
    if (!targetClient) {
        sendToClient(client->getSocket(), "441 " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
        return;
    }
    if (targetNick == client->getNickName()) {
        sendToClient(client->getSocket(), "401" + targetNick + " : You cannot kick yourself\r\n");
        return;
    }
    std::string kickMsg = ":" + client->getNickName() + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    channel->broadcastMessage(kickMsg, client->getSocket());
    sendToClient(client->getSocket(), kickMsg);
    channel->removeUser(targetClient->getSocket());
}

void Server::handleINVITE(Client *client, const std::vector<std::string> &params) {
    if (params.size() < 2) {
        sendToClient(client->getSocket(), "461 INVITE :Not enough parameters\r\n");
        return;
    }
    std::string targetNick = params[0];
    std::string channelName = params[1];
    if (channels.find(channelName) == channels.end()) {
        sendToClient(client->getSocket(), "403 " + channelName + " :No such channel\r\n");
        return;
    }
    Channel *channel = channels[channelName];
    if (!channel->isUserInChannel(client->getSocket())) {
        sendToClient(client->getSocket(), "442 " + channelName + " :You're not on that channel\r\n");
        return;
    }
    if (channel->hasMode('i') && !channel->isOperator(client->getSocket())) { 
        sendToClient(client->getSocket(), "482 " + channelName + " :You're not a channel operator\r\n");
        return;
    }
    Client *targetClient = NULL;
    for (std::map<int, Client *>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getNickName() == targetNick) {
            targetClient = it->second;
            break;
        }
    }
    if (!targetClient) {
        sendToClient(client->getSocket(), "401 " + targetNick + " :No such nick/channel\r\n");
        return;
    }
    if (channel->isUserInChannel(targetClient->getSocket())) {
        sendToClient(client->getSocket(), "443 " + targetNick + " " + channelName + " :is already on channel\r\n");
        return;
    }
    channel->inviteUser(client, targetClient);
    sendToClient(client->getSocket(), "341 " + client->getNickName() + " " + targetNick + " " + channelName + "\r\n");
    sendToClient(targetClient->getSocket(), ":" + client->getNickName() + " INVITE " + targetNick + " :" + channelName + "\r\n");
}
