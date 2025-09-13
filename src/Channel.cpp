/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rtorres <rtorres@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/27 10:25:32 by rtorres           #+#    #+#             */
/*   Updated: 2025/03/11 15:55:45 by rtorres          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Channel.hpp"

Channel::Channel(const std::string &channelName) : name(channelName) {
    log("Channel created: " + name);
}

void Channel::log(const std::string &msg) const {
    if (DEBUG)
        std::cout << "DEBUG: [Channel: " << name << "] " << msg << std::endl;
}

void Channel::setTopic(const std::string &newTopic) {
    topic = newTopic;
    log("Topic set to: " + topic);
}

std::string Channel::getTopic() const {
    return topic;
}

void Channel::addUser(Client *client) {
    if (DEBUG)
        std::cout << "DEBUG: Adding user: " << client->getNickName() << " (FD: " << client->getSocket() << ") to channel: " << name << std::endl;

    users[client->getSocket()] = client;
    if (users.size() == 1) {
        if (DEBUG)
            std::cout << "DEBUG: First user in channel, making operator..." << std::endl;
        addOperator(client->getSocket());
    }
}

void Channel::removeUser(int clientFd) {
    std::map<int, Client *>::iterator it = users.find(clientFd);
    if (it != users.end()) {
        std::string nickname = it->second ? it->second->getNickName() : "(unknown)";
        log(it->second->getNickName() + " left channel.");
        users.erase(it);
        operators.erase(clientFd);
    }
    if (DEBUG) {
        std::cout << "DEBUG: Remaining users in channel: ";
        for (std::map<int, Client*>::iterator iter = users.begin(); iter != users.end(); ++iter) {
            std::cout << iter->second->getNickName() << " ";
        }
        std::cout << std::endl;
    }
}

bool Channel::isUserInChannel(int clientFd) const {
    return users.find(clientFd) != users.end();
}
Client *Channel::getUserByNick(const std::string &nickname) const {
    for (std::map<int, Client *>::const_iterator it = users.begin(); it != users.end(); ++it) {
        if (it->second->getNickName() == nickname) {
            return it->second;
        }
    }
    return NULL;
}

bool Channel::isOperator(int clientFd) const {
    if (DEBUG) {
        std::cout << "DEBUG: Checking operator status for FD: " << clientFd << std::endl;
        if (operators.empty()) {
            std::cout << "DEBUG: Operator list is empty!" << std::endl;
        } else {
            std::cout << "DEBUG: Current Operators: ";
            std::map<int, bool>::const_iterator it;
            for (it = operators.begin(); it != operators.end(); ++it) {
                std::cout << it->first << "(" << (it->second ? "OP" : "NO OP") << ") ";
            }
            std::cout << std::endl;
        }
    }
    std::map<int, bool>::const_iterator it = operators.find(clientFd);
    return (it != operators.end() && it->second);
}

void Channel::addOperator(int clientFd) {
    if (isUserInChannel(clientFd)) {
        operators[clientFd] = true;
        if (DEBUG)
            std::cout << "DEBUG: Added operator: " << clientFd << " (" << users[clientFd]->getNickName() << ")" << std::endl;
    } else {
        if (DEBUG)
            std::cout << "DEBUG: Error: Tried to add operator for FD: " << clientFd << ", but user is not in channel!" << std::endl;
    }
}

void Channel::removeOperator(int clientFd) {
    if (isOperator(clientFd)) {
        log(users[clientFd]->getNickName() + " is no longer an operator.");
        operators.erase(clientFd);
    }
}

void Channel::broadcastMessage(const std::string &message, int senderFd) {
    std::map<int, Client *>::iterator senderIt = users.find(senderFd);
    if (senderIt == users.end()) {
        return;
    }       
    std::string joinMessage = message;
    for (std::map<int, Client *>::iterator it = users.begin(); it != users.end(); ++it) {
        if (it->first != senderFd)
            if (send(it->first, joinMessage.c_str(), joinMessage.size(), 0) < 0)
                log("Error sending message to " + it->second->getNickName());
    }
}

void Channel::broadcastToOps(const std::string &message) {
    for (std::map<int, bool>::iterator it = operators.begin(); it != operators.end(); ++it) {
        if (it->second) {
            send(it->first, message.c_str(), message.size(), 0);
        }
    }
}

std::vector<std::string> Channel::listUsers() const {
    std::vector<std::string> userList;
    std::map<int, Client *>::const_iterator it;
    for (it = users.begin(); it != users.end(); ++it) {
        userList.push_back(it->second->getNickName());
    }
    return userList;
}

std::string Channel::getName() const { return (name); }

bool Channel::hasMode(char mode) const {
    std::map<char, bool>::const_iterator it = modes.find(mode);
    return (it != modes.end() && it->second);
}

void Channel::setMode(char mode) {
    modes[mode] = true;
    log("Mode +" + std::string(1, mode) + " set.");
}

void Channel::unsetMode(char mode) {
    modes[mode] = false;
    log("Mode -" + std::string(1, mode) + " unset.");
}

std::string Channel::getModes() const {
    std::string modeStr;
    for (std::map<char, bool>::const_iterator it = modes.begin(); it != modes.end(); ++it) {
        if (it->second) {
            modeStr += it->first;
        }
    }
    return (modeStr.empty() ? "" : "+" + modeStr);
}

void Channel::setUserLimit(int limit) { 
    if (limit > 0){
        userLimit = limit;
        setMode('l');
    }else
        unsetMode('l');
}

int Channel::getUserLimit() const {
    return (userLimit);
}

bool Channel::isFull() const {
    return (hasMode('l') && users.size() >= (unsigned long)userLimit);
}

void Channel::kickUser(int operatorFd, const std::string &targetNick, const std::string &reason) {
    Client *targetClient = getUserByNick(targetNick);
    if (!targetClient)
    {
        log("User " + targetNick + " not found in channel. ");
        return ;
    }
    if (!isOperator(operatorFd))
    {
        log("Error: only channel operators can kick users.");
        return;
    }
    int targetFd = targetClient->getSocket();
    std::string KickMessage = ":" + users[operatorFd]->getNickName() +
        " KICK #" + name + " " + targetNick + " :" + reason + "\r\n";

    broadcastMessage(KickMessage, -1);
    removeUser(targetFd);
}

void Channel::inviteUser(Client *operatorClient, Client *targetClient) {
    if (!isOperator(operatorClient->getSocket())) {
        log("Error: Only channel operators can invite users.");
        return;
    }
    targetClient->addChannelInvite(name);
}

void Channel::setPassword(const std::string &pass) {
    if (!pass.empty()) {
        password = pass;
        setMode('k');
    } else {
        password.clear();
        unsetMode('k');
    }
}

bool Channel::checkPassword(const std::string &pass) const {
    return (password == pass);
}

std::string Channel::getPassword() const {
    return (password);
}

