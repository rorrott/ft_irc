# ft_irc

ft_irc is a lightweight IRC (Internet Relay Chat) server developed as part of the 42 School curriculum.

The goal is to implement an RFC-compliant IRC server that allows multiple clients to connect and communicate in real time.

This project introduces students to network programming, sockets, asynchronous I/O, and the IRC protocol.

## Features

- Server implementation:

- - Handles multiple simultaneous clients

- - Uses non-blocking sockets and multiplexing (poll/select)

- Authentication:

- - PASS, NICK, USER commands

- Channel management:

- - Create/join/leave channels

- - Invite users, set topics, and manage operators

- Messaging:

- - Private messages and channel-wide broadcasts

- RFC 2812 compliance (minimum subset required)

## Key Concepts

- Socket programming (TCP/IP)

- Multiplexing with poll/select

- Client-server architecture

- Protocol design & parsing

- Concurrency and scalability

## Project Rules (42)

- Written in C++98 standard

- Must handle multiple clients efficiently

- Must comply with a subset of the IRC RFC 2812

- No external networking libraries allowed (only system calls)

- Must handle invalid input and disconnects gracefully
