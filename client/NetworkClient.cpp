// NetworkClient.cpp
#include "NetworkClient.hpp"

NetworkClient::NetworkClient()
{
    socket_.setBlocking(false);
}

NetworkClient::~NetworkClient()
{
    stopReceiveLoop();
    // Disconnect if connected
    if (socket_.getRemoteAddress() != sf::IpAddress::None)
    {
        socket_.disconnect();
    }
}

bool NetworkClient::connect(const sf::IpAddress& ip, unsigned short port, sf::Time timeout)
{
    // Temporarily make blocking for connect with timeout, then switch to non-blocking
    socket_.setBlocking(true);
    sf::Socket::Status status = socket_.connect(ip, port, timeout);
    socket_.setBlocking(false);
    return status == sf::Socket::Done;
}

void NetworkClient::startReceiveLoop()
{
    if (running_) return;
    running_ = true;
    recvThread_ = std::thread(&NetworkClient::receiveLoop, this);
}

void NetworkClient::stopReceiveLoop()
{
    running_ = false;
    if (recvThread_.joinable())
        recvThread_.join();
}

bool NetworkClient::sendPacket(sf::Packet& packet)
{
    std::lock_guard<std::mutex> lk(sendMutex_);
    if (socket_.send(packet) == sf::Socket::Done) return true;
    return false;
}

bool NetworkClient::popPacket(sf::Packet& out)
{
    std::lock_guard<std::mutex> lk(queueMutex_);
    if (incoming_.empty()) return false;
    out = std::move(incoming_.front());
    incoming_.pop_front();
    return true;
}

sf::TcpSocket& NetworkClient::socket()
{
    return socket_;
}

bool NetworkClient::isConnected() const
{
    return socket_.getRemoteAddress() != sf::IpAddress::None;
}

void NetworkClient::receiveLoop()
{
    sf::SocketSelector selector;
    selector.add(socket_);
    while (running_)
    {
        // Wait with a timeout so we can stop quickly
        if (selector.wait(sf::milliseconds(200)))
        {
            if (selector.isReady(socket_))
            {
                sf::Packet packet;
                sf::Socket::Status status = socket_.receive(packet);
                if (status == sf::Socket::Done)
                {
                    std::lock_guard<std::mutex> lk(queueMutex_);
                    incoming_.push_back(std::move(packet));
                }
                else if (status == sf::Socket::Disconnected)
                {
                    running_ = false;
                    break;
                }
                // NotReady case is handled by the selector.wait timeout
            }
        }
        // loop continues checking running_
    }
}
