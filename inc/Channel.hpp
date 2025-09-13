/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rtorres <rtorres@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/27 10:26:15 by rtorres           #+#    #+#             */
/*   Updated: 2025/03/10 17:51:53 by rtorres          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sys/socket.h>
#include <sstream>
#include "Client.hpp"
#include "Server.hpp"

class Channel {
private:
    std::string name;
    std::string topic;
    std::map<int, Client *> users;
    std::map<int, bool> operators;
    std::map<char, bool> modes;
    int userLimit;
    std::string password;
public:
    Channel(const std::string &channelName);
    void setTopic(const std::string &newTopic);
    std::string getTopic() const;
    void addUser(Client *client);
    void removeUser(int clientFd);
    bool isUserInChannel(int clientFd) const;
    Client *getUserByNick(const std::string &nickname) const;
    std::vector<std::string> listUsers() const;
    bool isOperator(int clientFd) const;
    void addOperator(int clientFd);
    void removeOperator(int clientFd);
    void broadcastMessage(const std::string &message, int senderFd);
    void broadcastToOps(const std::string &message);
    std::string getName() const;
    bool hasMode(char mode) const;
    void setMode(char mode);
    void unsetMode(char mode);
    std::string getModes() const;
    void log(const std::string &msg) const;
    void setUserLimit(int limit);
    int getUserLimit() const;
    bool isFull() const;
    void kickUser(int operatorFd, const std::string &targetNick, const std::string &reason);
    void inviteUser(Client *operatorClient, Client *targetClient);
    void setPassword(const std::string &pass);
    bool checkPassword(const std::string &pass) const;
    std::string getPassword() const;
};

#endif

