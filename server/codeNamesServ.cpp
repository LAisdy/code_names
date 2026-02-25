#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <exception>
#include <windows.h>
#include <SFML/Network.hpp>
#include <deque>


// Constants
const int FIRST_TURN_TIME = 180;
const int REGULAR_TURN_TIME = 90;
const int MAX_WORDS = 396;
const std::filesystem::path WORDS_PATH = "./words.txt";
const sf::Time TIMEOUT = sf::seconds(300);
const sf::Time BROADCAST_INTERVAL = sf::seconds(1.0f);
const int CORRECT_GUESS_TIME_BONUS = 10;

// Command strings
const std::string CMD_PARAMS = "params";
const std::string CMD_BOARD = "board";
const std::string CMD_START = "start";
const std::string CMD_PREP = "prep";
const std::string CMD_SLOT = "slot";
const std::string CMD_FREE = "free";
const std::string CMD_CARD = "card";
const std::string CMD_FLAG = "flag";
const std::string CMD_CHAT = "chat";
const std::string CMD_NAME = "name";
const std::string CMD_END = "end";
const std::string CMD_RESTART = "restart";

// Enums
enum Color {
    NEUTRAL = 0,
    BLUE = 1,
    RED = 2,
    ASSASSIN = 3
};

enum Role {
    SPECTATOR,
    PLAYER,
    MASTER
};

// Structs
struct Player {
    std::unique_ptr<sf::TcpSocket> socket;
    int clientId;
    int team = -1;
    int slot = -1;
    Role role = SPECTATOR;
    std::string name;
    bool isHost = false;
    sf::Clock lastActivityClock;

    void setRole(Role newRole) { role = newRole; }
    void setTeam(int newTeam) { team = newTeam; }
};

struct WordCard {
    int order;
    std::string gameWord;
    Color color;
    bool isRevealed = false;
    std::vector<std::string> blueFlaggers; // Renamed for clarity, assuming teams
    std::vector<std::string> redFlaggers;

    WordCard(int num, std::string word, Color col) : order(num), gameWord(word), color(col) {}
};

//declarations
std::vector<WordCard> loadWords(const std::filesystem::path& filePath); //kostyl' 


struct Lobby {
    std::vector<std::vector<std::string>> slots{ 2 };

    void resizeTeams(int size) {
        slots[0].assign(size, "free");
        slots[1].assign(size, "free");
    }

    bool isSlotFree(int team, int slotIndex) const {
        return slots[team][slotIndex] == "free";
    }

    void occupySlot(int team, int slotIndex, const std::string& name) {
        slots[team][slotIndex] = name;
    }

    void freeSlot(int team, int slotIndex) {
        slots[team][slotIndex] = "free";
    }
};

struct GameStats {
    std::vector<WordCard> gameBoard;
    sf::Clock turnTimer;
    int turnTimeLeft = REGULAR_TURN_TIME;
    int curTeam = 0; // 0: Blue, 1: Red
    std::string curRole = "master"; // "master" or "player"
    bool existsWinner = false;
    int winTeam = -1;
    int blueWordsLeft = 9;
    int redWordsLeft = 8;

    GameStats(const std::vector<WordCard>& board) : gameBoard(board) { turnTimeLeft = FIRST_TURN_TIME; }

    void nextTurn() {
        if (curRole == "master") {
            curRole = "player";
        }
        else {
            curRole = "master";
            curTeam = 1 - curTeam;
        }
        turnTimeLeft = REGULAR_TURN_TIME;
        turnTimer.restart();
    }

    bool checkWinner() {
        if (blueWordsLeft == 0) {
            winTeam = 0;
            existsWinner = true;
            return true;
        }
        if (redWordsLeft == 0) {
            winTeam = 1;
            existsWinner = true;
            return true;
        }
        return false;
    }

    void updateScore(int team) {
        if (team == 0) {
            blueWordsLeft--;
            std::cout << "UPD: blue team words left: " << blueWordsLeft << '\n';
        }
        else if (team == 1) {
            redWordsLeft--;
            std::cout << "UPD: red team words left: " << redWordsLeft << '\n';
        }
        checkWinner();
    }

    void reveal(int order) {
        if (order < 0 || order >= static_cast<int>(gameBoard.size())) return;
        gameBoard[order].isRevealed = true;
    }

    float getCurrentTurnTimeLeft() const {
        return static_cast<float>(turnTimeLeft) - turnTimer.getElapsedTime().asSeconds();
    }
};

struct GameSession {
    GameStats gameState;
    Lobby lobby;
    std::vector<Player> players;
    std::unordered_map<std::string, Player*> playerLookup;
    bool isStarted = false;

    GameSession(const std::vector<WordCard>& board) : gameState(board) {}

    void addPlayer(Player& player) 
    {
        playerLookup[player.name] = &player;
    }
    void resetGame()
    {
        for (auto& teamSlots : lobby.slots) {
            for (auto& slot : teamSlots) {
                slot = "free";
            }
        }

        for (auto& player : players) {
            player.team = -1;
            player.slot = -1;
            player.role = SPECTATOR;
            player.isHost = (player.clientId == 0);
        }

        auto newBoard = loadWords(WORDS_PATH);
        gameState = GameStats(newBoard);
        isStarted = false;
    }
    void handlePacket(sf::Packet& packet, Player& sender);
    void handleParams(sf::Packet& packet, Player& sender);
    void handleBoard(sf::Packet& packet, Player& sender);
    void handleStart(sf::Packet& packet, Player& sender);
    void handlePrep(sf::Packet& packet, Player& sender);
    void handleSlot(sf::Packet& packet, Player& sender);
    void handleFree(Player& sender);
    void handleCard(sf::Packet& packet, Player& sender);
    void handleFlag(sf::Packet& packet, Player& sender);
    void handleChat(sf::Packet& packet, Player& sender);
    void handleEnd(Player& sender);
    void handleRestart(Player& sender);
};

// Utility functions
std::vector<int> getRandomUniqueNumbers(int maxNumber, int count) {
    if (count > maxNumber + 1) throw std::invalid_argument("Count exceeds max");
    std::unordered_set<int> uniqueNumbers;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, maxNumber);
    while (uniqueNumbers.size() < static_cast<size_t>(count)) {
        uniqueNumbers.insert(dis(gen));
    }
    return std::vector<int>(uniqueNumbers.begin(), uniqueNumbers.end());
}

std::vector<WordCard> loadWords(const std::filesystem::path& filePath) {
    std::vector<std::string> allWords;
    std::ifstream file(filePath);
    if (!file) throw std::runtime_error("Can't open file: " + filePath.string());
    std::string word;
    while (std::getline(file, word)) {
        allWords.push_back(word);
    }
    file.close();

    if (allWords.size() < 25) throw std::runtime_error("Insufficient words in file");

    // Select 24 random unique indices
    auto selectedIndices = getRandomUniqueNumbers(static_cast<int>(allWords.size()) - 1, 24);

    // Assign colors: 9 blue, 8 red, 6 neutral, 1 assassin
    std::vector<Color> colors(9, BLUE);
    colors.insert(colors.end(), 8, RED);
    colors.insert(colors.end(), 6, NEUTRAL);
    colors.push_back(ASSASSIN);

    // Shuffle colors for random assignment
    std::shuffle(colors.begin(), colors.end(), std::mt19937(std::random_device{}()));

    // Create cards with random board positions
    auto positions = getRandomUniqueNumbers(23, 24);
    std::vector<WordCard> wordsVec;
    for (size_t i = 0; i < 24; ++i) {
        wordsVec.emplace_back(positions[i], allWords[selectedIndices[i]], colors[i]);
    }

    // Sort by order for board layout
    std::sort(wordsVec.begin(), wordsVec.end(), [](const WordCard& a, const WordCard& b) {
        return a.order < b.order;
        });

    return wordsVec;
}

sf::Packet createLobbyStatePacket(const Lobby& lobby) {
    sf::Packet packet;
    packet << "update_lobby";
    for (int team = 0; team < 2; ++team) {
        for (size_t slot = 0; slot < lobby.slots[team].size(); ++slot) {
            std::string nick = lobby.slots[team][slot];
            packet << nick << team << static_cast<int>(slot);
        }
    }
    return packet;
}

bool sendSlotApprove(sf::TcpSocket& socket, bool approved) {
    sf::Packet packet;
    packet << "slot" << (approved ? 1 : 0);
    return socket.send(packet) == sf::Socket::Done;
}

bool sendLobbyState(sf::TcpSocket& socket, sf::Packet& packet) {
    return socket.send(packet) == sf::Socket::Done;
}

bool sendTeamSize(sf::TcpSocket& socket, int size) {
    sf::Packet packet;
    packet << "size" << size;
    return socket.send(packet) == sf::Socket::Done;
}

bool sendStart(sf::TcpSocket& socket) {
    sf::Packet packet;
    packet << "start";
    return socket.send(packet) == sf::Socket::Done;
}

bool sendWords(sf::TcpSocket& socket, Role role, const std::vector<WordCard>& board) {
    sf::Packet packet;
    packet << "board";
    int colorToSend = (role == MASTER) ? -1 : 4; // 4 for hidden
    for (const auto& card : board) {
        int col = (colorToSend == -1) ? static_cast<int>(card.color) : colorToSend;
        packet << card.gameWord << col;
    }
    return socket.send(packet) == sf::Socket::Done;
}

bool sendNextTurn(sf::TcpSocket& socket) {
    sf::Packet packet;
    packet << "next";
    return socket.send(packet) == sf::Socket::Done;
}


bool sendReveal(sf::TcpSocket& socket, int order, int color) {
    sf::Packet packet;
    packet << "reveal" << order << color;
    return socket.send(packet) == sf::Socket::Done;
}

bool sendFlagUpdate(sf::TcpSocket& socket, int order, int flagInd) {
    sf::Packet packet;
    packet << "flag" << order << flagInd;
    return socket.send(packet) == sf::Socket::Done;
}

bool sendAddTime(sf::TcpSocket& socket, int timeVal) {
    sf::Packet packet;
    packet << "timer_add" << timeVal;
    return socket.send(packet) == sf::Socket::Done;
}

bool sendUpdateTimer(sf::TcpSocket& socket, float length) {
    sf::Packet packet;
    packet << "update_timer" << length;
    return socket.send(packet) == sf::Socket::Done;
}

bool sendTimerSetup(sf::TcpSocket& socket, float val, int team) {
    sf::Packet packet;
    packet << "set_timer" << team << val;
    return socket.send(packet) == sf::Socket::Done;
}

bool sendNextAndTimer(sf::TcpSocket& socket, int curTeam, std::string& curRole, float timeLeft) {
    sf::Packet packet;
    packet << "next" << curTeam << curRole;
    socket.send(packet);
    return sendTimerSetup(socket, timeLeft, curTeam);
}
bool sendChatMsg(sf::TcpSocket& socket, int team, const std::string& msg) {
    sf::Packet packet;
    packet << "chat" << team << msg;
    return socket.send(packet) == sf::Socket::Done;
}

bool sendWinner(sf::TcpSocket& socket, int team) {
    sf::Packet packet;
    packet << "winner" << team;
    return socket.send(packet) == sf::Socket::Done;
}

// GameSession methods
void GameSession::handlePacket(sf::Packet& packet, Player& sender) 
{
    std::string command;
    packet >> command;
    std::cout << "Received command '" << command << "' from: " << sender.name << '\n';

    if (command == CMD_PARAMS) handleParams(packet, sender);
    else if (command == CMD_BOARD) handleBoard(packet, sender);
    else if (command == CMD_START) handleStart(packet, sender);
    else if (command == CMD_PREP) handlePrep(packet, sender);
    else if (command == CMD_SLOT) handleSlot(packet, sender);
    else if (command == CMD_FREE) handleFree(sender);
    else if (command == CMD_CARD) handleCard(packet, sender);
    else if (command == CMD_FLAG) handleFlag(packet, sender);
    else if (command == CMD_CHAT) handleChat(packet, sender);
    else if (command == CMD_END) handleEnd(sender);
    else if (command == CMD_RESTART) handleRestart(sender);
}

void GameSession::handleParams(sf::Packet& packet, Player& sender) {
    std::string roleStr;
    int team;
    packet >> roleStr >> team;

    Role newRole = (roleStr == "master") ? MASTER : (roleStr == "player") ? PLAYER : SPECTATOR;
    sender.setRole(newRole);
    sender.setTeam(team);

    sf::Packet response;
    response << "role_set" << roleStr << team;
    sender.socket->send(response);
}

void GameSession::handleBoard(sf::Packet& packet, Player& sender) {
    if (!sender.isHost) return;
    isStarted = true;
    for (auto& player : players) {
        sendWords(*player.socket, player.role, gameState.gameBoard);
    }
}

void GameSession::handleStart(sf::Packet& packet, Player& sender) {
    if (!sender.isHost) return;
    isStarted = true;

    gameState.turnTimeLeft = FIRST_TURN_TIME;
    gameState.turnTimer.restart();

    for (auto& player : players) {
        sendStart(*player.socket);
        sendTimerSetup(*player.socket, static_cast<float>(gameState.turnTimeLeft), gameState.curTeam);
    }
}

void GameSession::handlePrep(sf::Packet& packet, Player& sender) {
    if (!sender.isHost) return;
    int size;
    packet >> size;
    if (size > 0) {
        lobby.resizeTeams(size);
        std::cout << "Lobby created with " << size << " slots per team.\n";
    }
}

void GameSession::handleSlot(sf::Packet& packet, Player& sender) {
    int team, order;
    packet >> team >> order;

    if (lobby.isSlotFree(team, order)) {
        if (sender.slot != -1) {
            lobby.freeSlot(sender.team, sender.slot);
        }
        sender.setTeam(team);
        sender.setRole((order == 0) ? MASTER : PLAYER);
        lobby.occupySlot(team, order, sender.name);
        sender.slot = order;

        sendSlotApprove(*sender.socket, true);
        auto lobbyPacket = createLobbyStatePacket(lobby);
        for (auto& player : players) {
            sendLobbyState(*player.socket, lobbyPacket);
        }
    }
    else {
        sendSlotApprove(*sender.socket, false);
    }
}

void GameSession::handleFree(Player& sender) {
    if (sender.slot == -1) return;
    lobby.freeSlot(sender.team, sender.slot);
    sender.setTeam(-1);
    sender.slot = -1;
    sender.setRole(SPECTATOR);
    sendSlotApprove(*sender.socket, true);

    auto lobbyPacket = createLobbyStatePacket(lobby);
    for (auto& player : players) {
        sendLobbyState(*player.socket, lobbyPacket);
    }
}

void GameSession::handleCard(sf::Packet& packet, Player& sender) {
    if (sender.team != gameState.curTeam || sender.role != PLAYER || gameState.curRole != "player") return;

    int order;
    packet >> order;
    if (order < 0 || order >= static_cast<int>(gameState.gameBoard.size())) return;

    Color col = gameState.gameBoard[order].color;
    int colInt = static_cast<int>(col);
    int otherTeam = 1 - gameState.curTeam;

    gameState.reveal(order);

    for (auto& player : players) {
        sendReveal(*player.socket, order, colInt);
    }

    if (col == ASSASSIN) {
        gameState.winTeam = otherTeam;
        gameState.existsWinner = true;
        for (auto& player : players) {
            sendWinner(*player.socket, otherTeam);
        }
    }
    else if (col == NEUTRAL) {
        gameState.nextTurn();
        gameState.turnTimer.restart();
        for (auto& player : players) {
            sendNextAndTimer(*player.socket, gameState.curTeam, gameState.curRole, static_cast<float>(gameState.turnTimeLeft));
        }
    }
    else if (colInt == gameState.curTeam + 1) { // Own color
        gameState.updateScore(gameState.curTeam);
        gameState.turnTimeLeft += CORRECT_GUESS_TIME_BONUS;
        for (auto& player : players) {
            sendAddTime(*player.socket, CORRECT_GUESS_TIME_BONUS);
        }
        if (gameState.existsWinner) {
            for (auto& player : players) {
                sendWinner(*player.socket, gameState.winTeam);
            }
        }
    }
    else { // Opponent's color
        gameState.updateScore(otherTeam);
        gameState.nextTurn();
        gameState.turnTimer.restart();
        for (auto& player : players) {
            sendNextAndTimer(*player.socket, gameState.curTeam, gameState.curRole, static_cast<float>(gameState.turnTimeLeft));
        }
        if (gameState.existsWinner) {
            for (auto& player : players) {
                sendWinner(*player.socket, gameState.winTeam);
            }
        }
    }
}

void GameSession::handleFlag(sf::Packet& packet, Player& sender) {
    // TODO: Implement flag handling if needed
}

void GameSession::handleRestart(Player& sender)
{
    if (!sender.isHost) return;
    resetGame();

    sf::Packet p;
    p << "return_to_lobby";
    for (auto& player : players)
    {
        player.socket->send(p);
    }

    auto lobbyPacket = createLobbyStatePacket(lobby);
    for (auto& player : players) 
    {
        sendLobbyState(*player.socket, lobbyPacket);
        sendTeamSize(*player.socket, static_cast<int>(lobby.slots[0].size()));
    }

}

void GameSession::handleEnd(Player& sender) {
    if (sender.team != gameState.curTeam ||
        sender.role != (gameState.curRole == "master" ? MASTER : PLAYER))
        return;

    gameState.nextTurn();
    for (auto& player : players) {
        sendNextAndTimer(*player.socket, gameState.curTeam, gameState.curRole, static_cast<float>(gameState.turnTimeLeft));
    }
}

void GameSession::handleChat(sf::Packet& packet, Player& sender) {
    if (sender.team != gameState.curTeam || sender.role != MASTER || gameState.curRole != "master") return;

    std::string msg, fullMsg;
    while (packet >> msg) 
    {
        fullMsg += msg + " ";
    }
    if (!fullMsg.empty()) fullMsg.pop_back();

    for (auto& player : players) {
        sendChatMsg(*player.socket, gameState.curTeam, fullMsg);
    }
    gameState.nextTurn();
    gameState.turnTimer.restart();
    float timeLeft = gameState.getCurrentTurnTimeLeft();
    for (auto& player : players) {
        sendNextAndTimer(*player.socket, gameState.curTeam, gameState.curRole, timeLeft);
    }
}

int main() {
    std::cout << "Local address (getLocalAddress): " << sf::IpAddress::getLocalAddress() << "\n";
    std::cout << "Public address (getPublicAddress): " << sf::IpAddress::getPublicAddress() << "\n";

    SetConsoleOutputCP(CP_UTF8);
    auto words = loadWords(WORDS_PATH);
    GameSession game(words);

    sf::TcpListener listener;
    if (listener.listen(54000) != sf::Socket::Done) {
        std::cerr << "Error binding to port 54000.\n";
        return -1;
    }

    listener.setBlocking(false);
    sf::SocketSelector selector;
    selector.add(listener);

    static int nextClientId = 0;
    sf::Clock broadcastClock;

    std::cout << "Server running. Waiting for clients...\n";
    while (true) {
        const sf::Time SELECTOR_TIMEOUT = sf::milliseconds(100);
        if (selector.wait(SELECTOR_TIMEOUT)) {
            if (selector.isReady(listener)) {
                auto newSocket = std::make_unique<sf::TcpSocket>();
                if (listener.accept(*newSocket) == sf::Socket::Done) {
                    sf::Packet helloPack;
                    if (newSocket->receive(helloPack) == sf::Socket::Done) {
                        std::string command;
                        helloPack >> command;
                        if (command == CMD_NAME) {
                            std::string nickName;
                            helloPack >> nickName;
                            bool isHost = (nextClientId == 0);
                            game.players.push_back({ std::move(newSocket), nextClientId++, -1, -1, SPECTATOR, nickName, isHost });
                            game.addPlayer(game.players.back());
                            selector.add(*game.players.back().socket);

                            if (!isHost) {
                                sendTeamSize(*game.players.back().socket, static_cast<int>(game.lobby.slots[0].size()));
                            }
                            std::cout << "New client: " << nickName << " (ID: " << game.players.back().clientId << ") connected.\n";
                        }
                    }
                }
            }
            else {
                for (size_t i = 0; i < game.players.size(); ++i) {
                    auto& player = game.players[i];
                    if (selector.isReady(*player.socket)) {
                        sf::Packet packet;
                        auto status = player.socket->receive(packet);
                        if (status == sf::Socket::Done) {
                            player.lastActivityClock.restart();
                            game.handlePacket(packet, player);
                        }
                        else if (status == sf::Socket::Disconnected || player.lastActivityClock.getElapsedTime() > TIMEOUT) {
                            std::cout << "Client " << player.clientId << " disconnected/timed out.\n";
                            selector.remove(*player.socket);
                            player.socket->disconnect();
                            game.players.erase(game.players.begin() + i);
                            --i;
                        }
                    }
                }
            }
        }

        // Handle turn timer broadcasting and expiration
        if (game.isStarted && broadcastClock.getElapsedTime() >= BROADCAST_INTERVAL) {
            float currentTimeLeft = game.gameState.getCurrentTurnTimeLeft();
            if (currentTimeLeft <= 0.0f) {
                game.gameState.nextTurn();
                game.gameState.turnTimer.restart();
                for (auto& player : game.players) {
                    sendNextAndTimer(*player.socket, game.gameState.curTeam, game.gameState.curRole, static_cast<float>(game.gameState.turnTimeLeft));
                }
            }
            else {
                for (auto& player : game.players) {
                    sendUpdateTimer(*player.socket, currentTimeLeft);
                }
            }
            broadcastClock.restart();
        }
    }
    return 0;
}