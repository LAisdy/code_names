// NetworkClient.hpp
#pragma once

#include <SFML/Network.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    // Connect to server (blocking with timeout)
    bool connect(const sf::IpAddress& ip, unsigned short port, sf::Time timeout = sf::seconds(5));

    // Start/stop receive loop
    void startReceiveLoop();
    void stopReceiveLoop(); // blocking: waits for thread finish

    // Thread-safe send
    bool sendPacket(sf::Packet& packet);

    // Pop one packet (non-blocking). Returns true if a packet was popped.
    bool popPacket(sf::Packet& out);

    // Access to internal socket for legacy send functions (safe while connected).
    sf::TcpSocket& socket();

    bool isConnected() const;

private:
    void receiveLoop();

    sf::TcpSocket socket_;
    std::thread recvThread_;
    std::atomic<bool> running_{ false };

    std::mutex queueMutex_;
    std::deque<sf::Packet> incoming_;

    std::mutex sendMutex_;
};
