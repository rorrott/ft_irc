/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rtorres <rtorres@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/27 10:25:49 by rtorres           #+#    #+#             */
/*   Updated: 2025/03/11 11:21:35 by rtorres          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"

Server *globalServer = NULL;

void signalHandler(int signum) {
    std::cout << std::endl;
    if (DEBUG)
        std::cout << "DEBUG: Caught signal " << signum << ". Shutting down...\n";
    if (globalServer) {
        globalServer->shutdownServer();
        delete globalServer;
        globalServer = NULL;
    }
    exit(0);
}


int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return (1);
    }
    std::string port = argv[1];
    std::string password = argv[2];
    Server *server = new Server(port, password);
    globalServer = server;
    signal(SIGINT, signalHandler);
    server->run();
    delete server; 
    globalServer = NULL;
    return (0);
}
