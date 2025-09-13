/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rtorres <rtorres@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/27 10:25:59 by rtorres           #+#    #+#             */
/*   Updated: 2025/03/11 10:05:28 by rtorres          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Client.hpp"

Client::Client() 
    : fd(-1), isOperator(false), registered(false), authenticated(false), logedin(false) {}

Client::Client(int fd, const std::string &ip) 
    : fd(fd), isOperator(false), registered(false), authenticated(false), logedin(false), ipadd(ip) {
        if (ipadd.empty())
            ipadd = "unknown.host";
    }

Client::~Client() {}

int Client::getSocket() const { return fd; }

std::string Client::getNickName() const { return nickname; }

std::string Client::getUserName() const { return username; }

std::string Client::getRealName() const { return realname; }

std::string Client::getIpAddress() const { return ipadd; }

std::string &Client::getBuffer() { return buffer; }

bool Client::isAuthenticated() const { return authenticated; }

bool Client::isRegistered() const { return registered; }

bool Client::isOperatorStatus() const { return isOperator; }

bool Client::isLoggedIn() const { return logedin; }

std::string Client::getHostname() const {
    return nickname + "!" + username + "@" + (ipadd.empty() ? "unknown.host" : ipadd);
}

bool Client::isInvitedToChannel(const std::string &chName) const {
    for (size_t i = 0; i < ChannelsInvite.size(); i++) {
        if (ChannelsInvite[i] == chName)
            return true;
    }
    return false;
}

void Client::setNickName(const std::string &nick) {
    nickname = nick;
    if (!username.empty()) registerUser();
}

void Client::setUserName(const std::string &user) {
    username = user;
    if (!nickname.empty()) registerUser();
}

void Client::setRealName(const std::string &real) { realname = real; }

void Client::setPassword(const std::string &pass) { password = pass; }

void Client::setIpAddress(const std::string &ip) { ipadd = ip; }

void Client::setOperator(bool value) { isOperator = value; }

void Client::setAuthenticated(bool value) { authenticated = value; }

void Client::setRegistered(bool value) { registered = value; }

void Client::setLoggedIn(bool value) { logedin = value; }

void Client::setBuffer(const std::string &data) { buffer += data; }

bool Client::checkPassword(const std::string &inputPassword, const std::string &correctPassword) {
    std::string trimmedPassword = inputPassword;
    
    trimmedPassword.erase(std::remove(trimmedPassword.begin(), trimmedPassword.end(), '\n'), trimmedPassword.end());
    trimmedPassword.erase(std::remove(trimmedPassword.begin(), trimmedPassword.end(), '\r'), trimmedPassword.end());
    if (trimmedPassword == correctPassword) {
        authenticated = true;
        return true;
    }
    return false;
}

void Client::registerUser() {
    if (!nickname.empty() && !username.empty()) {
        registered = true;
    }
}

void Client::clearBuffer() {
    buffer.clear();
}

void Client::addChannelInvite(const std::string &chname) {
    ChannelsInvite.push_back(chname);
}

void Client::removeChannelInvite(const std::string &chname) {
    for (size_t i = 0; i < ChannelsInvite.size(); i++) {
        if (ChannelsInvite[i] == chname) {
            ChannelsInvite.erase(ChannelsInvite.begin() + i);
            return;
        }
    }
}
