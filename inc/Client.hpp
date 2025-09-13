/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rtorres <rtorres@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/27 10:26:22 by rtorres           #+#    #+#             */
/*   Updated: 2025/03/10 18:29:24 by rtorres          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <algorithm>

class Client {
private:
    int fd;
    bool isOperator;
    bool registered;
    bool authenticated;
    bool logedin;
    std::string nickname;
    std::string username;
    std::string realname;
    std::string ipadd;
    std::string password;
    std::string buffer;
    std::vector<std::string> ChannelsInvite;
public:
    Client();
    Client(int fd, const std::string &ip);
    ~Client();
    int getSocket() const;
    std::string getNickName() const;
    std::string getUserName() const;
    std::string getRealName() const;
    std::string getIpAddress() const;
    std::string &getBuffer();
    std::string getHostname() const;
    bool isAuthenticated() const;
    bool isRegistered() const;
    bool isOperatorStatus() const;
    bool isLoggedIn() const;
    bool isInvitedToChannel(const std::string &chName) const;
    void setNickName(const std::string &nick);
    void setUserName(const std::string &user);
    void setRealName(const std::string &real);
    void setPassword(const std::string &pass);
    void setIpAddress(const std::string &ip);
    void setOperator(bool value);
    void setAuthenticated(bool value);
    void setRegistered(bool value);
    void setLoggedIn(bool value);
    void setBuffer(const std::string &data);
    bool checkPassword(const std::string &inputPassword, const std::string &correctPassword);
    void registerUser();
    void clearBuffer();
    void addChannelInvite(const std::string &chname);
    void removeChannelInvite(const std::string &chname);
};

#endif
