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
#include "SFML/Network.hpp"


//const params:
const int MAX_WORDS = 7088;
const std::filesystem::path WORDS_PATH = "E:/Repos/test/pyt/codeNamesServ/word_col/words.txt";
const sf::Time TIMEOUT = sf::seconds(300);

//game vars:

std::vector<std::vector<std::string>> slots
{
	{},
	{}
};

//enums:
enum Color
{
	WHITE = 0,
	ORANGE = 1,
	BROWN = 2,
	BLACK = 3
};

//structs:
struct Player
{
	std::unique_ptr<sf::TcpSocket> socket;
	int clientId;
	int team;
	int slot;
	std::string role;  //"spec", "player", "master"
	std::string name;
	bool isHost = false;
	sf::Clock lastActivityClock;

	void setRole(const std::string& newRole)
	{
		role = newRole;
	}

	void setTeam(int newTeam)
	{
		team = newTeam;
	}
};

struct WordCard
{
	WordCard(int num, std::string word, Color col)
		:order(num), gameWord(word), color(col)
	{
		isRevealed = false;
	};
	int order;
	std::string gameWord;
	Color color;
	bool isRevealed;
	std::vector<std::string> redFlaggers;
	std::vector<std::string> greenFlaggers;
};

struct GameStats
{
	std::vector<WordCard> gameBoard;

	sf::Clock timer;
	float timerVal = 100;
	float timeLeft = 0;

	int curTurn = 0b00;
	bool existsWinner = false;
	int turnTimeLeft = 60;
	int brownCount = 8;
	int orangeCount = 9;
	int curTeam = 0;
	int winTeam = -1;
	std::string curRole = "master";

	GameStats(const std::vector<WordCard>& board)
		:gameBoard(board)
	{
		curTeam = 0;
	}

	void nextTurn()
	{
		curTurn = (curTurn + 0b01) & 0b11;
		switch (curTurn)
		{
		case 0b00:
		{
			curRole = "master";
			curTeam = 0;
			break;
		}
		case 0b01:
		{
			curRole = "player";
			curTeam = 0;
			break;
		}
		case 0b10:
		{
			curRole = "master";
			curTeam = 1;
			break;
		}
		case 0b11:
		{
			curRole = "player";
			curTeam = 1;
			break;
		}
		default:
			throw std::runtime_error("Invalid curTurn value");
		}
		turnTimeLeft = 60;
	}

	bool checkWinner()
	{
		if (orangeCount == 0)
		{
			existsWinner = true;
			return true;
		}

		if (brownCount == 0)
		{
			existsWinner = true;
			return true;
		}
		return false;
	}

	void updateScore(int team)
	{
		if (team == 0)
		{
			orangeCount--;
			std::cout << "\nUPD: blue team score is: " << orangeCount << '\n';
		}
		else if (team == 1)
		{
			brownCount--;
			std::cout << "\nUPD: red team score is: " << orangeCount << '\n';
		}
		checkWinner();
	}

	float updateTimer()
	{
		timeLeft -= timer.restart().asSeconds();
		if (timeLeft < 0.f)
		{
			timeLeft = 0.f;
		}
		return timeLeft;
	}

	void resetGame()
	{
		gameBoard.clear();
		curTurn = 0;
		existsWinner = false;
		orangeCount = 9;
		brownCount = 8;
		curTeam = 0;
		turnTimeLeft = 60;
	}

	void reveal(int order)
	{
		if (order < 0 || order > 23)
			return;
		gameBoard[order].isRevealed = true;
	}
};

struct GameSession
{
	GameStats gameState;
	std::vector<Player> players;
	bool isStarted = false;

	GameSession(const std::vector<WordCard>& board) : gameState(board) {}
};

//player look-up functionality:

std::unordered_map<std::string, Player*> playerLookup;

void addPlayer(Player& player)
{
	playerLookup[player.name] = &player;
	std::cout << playerLookup[player.name]->name << " player added.\n";
	std::cout << playerLookup[player.name]->socket << " socket added.\n";
}

//funcs:
std::vector<int> getRandomUniqueNumbers(int maxNumber, int count)
{
	if (count > maxNumber + 1)
	{
		throw std::invalid_argument("Count cannot be greater than maxNumber + 1");
	}

	std::unordered_set<int> uniqueNumbers;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, maxNumber);

	while (uniqueNumbers.size() < count)
	{
		uniqueNumbers.insert(dis(gen));
	}
	return std::vector<int>(uniqueNumbers.begin(), uniqueNumbers.end());
}
std::vector<int> getRandomUniqueNumbersAsc(int maxNumber, int count)
{
	if (count > maxNumber + 1)
	{
		throw std::invalid_argument("Count cannot be greater than maxNumber + 1");
	}

	std::unordered_set<int> uniqueNumbers;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, maxNumber);

	while (uniqueNumbers.size() < count)
	{
		uniqueNumbers.insert(dis(gen));
	}
	std::vector<int> res(uniqueNumbers.begin(), uniqueNumbers.end());
	std::sort(res.begin(), res.end());
	return res;
}
std::vector<WordCard> getWordsList(const std::filesystem::path& filePath, std::vector<int>& inds)
{
	int orangeCount = 9;
	int brownCount = 8;
	int whiteCount = 6;
	int blackCount = 1;
	int order = 0;
	int ind = 0;
	std::vector<int> orderVec = getRandomUniqueNumbers(23, 24);
	std::vector<WordCard> wordsVec;
	std::ifstream words(filePath);
	if (!words)
	{
		throw std::runtime_error("Can't open file: " + filePath.filename().string());
	}
	std::string word;
	while (std::getline(words, word) && ind < inds.size())
	{
		if (orangeCount > 0 && order == inds[ind])
		{
			WordCard elem = WordCard(orderVec[ind++], word, ORANGE);
			order++;
			wordsVec.push_back(elem);
			orangeCount--;
		}
		else if (brownCount > 0 && order == inds[ind])
		{
			WordCard elem = WordCard(orderVec[ind++], word, BROWN);
			order++;
			wordsVec.push_back(elem);
			brownCount--;
		}
		else if (whiteCount > 0 && order == inds[ind])
		{
			WordCard elem = WordCard(orderVec[ind++], word, WHITE);
			order++;
			wordsVec.push_back(elem);
			whiteCount--;
		}
		else if (blackCount > 0 && order == inds[ind])
		{
			WordCard elem = WordCard(orderVec[ind++], word, BLACK);
			order++;
			wordsVec.push_back(elem);
			blackCount--;
		}
		else
		{
			order++;
		}
	}
	words.close();
	return wordsVec;
}
sf::Packet getLobbyState(std::vector<Player>& players)
{
	sf::Packet packet;
	packet << "update_lobby";
	bool res = true;
	for (int j = 0; j <= 1; ++j)
	{
		for (int i = 0; i < slots[0].size(); ++i)
		{
			std::string nick = slots[j][i];
			if (nick == "free")
			{
				packet << nick << j << i;
				std::cout << "\n*\n" << " nick: " << nick << "\n team: " << j << "\n slot: " << i << "\n*\n";
			}
			else
			{
				int team = j;
				packet << nick << team << i;
				std::cout << "\n*\n" << " nick: " << nick << "\n team: " << j << "\n slot: " << i << "\n*\n";
			}

		}
	}
	return packet;
}
bool sendSlotAprove(int isDone, sf::TcpSocket& socket)
{
	sf::Packet request;
	request << "slot" << isDone;
	if (socket.send(request) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "\nSlot approve sending failed\n";
	return false;
}
bool sendLobbyState(sf::TcpSocket& socket, sf::Packet& msg)
{
	if (socket.send(msg) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "\nLobby state sending failed\n";
	return false;
}
bool sendTeamSize(sf::TcpSocket& socket, int size)
{
	sf::Packet teamSize;
	teamSize << "size" << size;
	if (socket.send(teamSize) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "\nTeam size sending failed\n";
	return false;
}
bool sendStart(sf::TcpSocket& socket)
{
	sf::Packet start;
	start << "start";
	if (socket.send(start) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "\nStart sending failed\n";
	return false;
}
bool sendWords(sf::TcpSocket& socket, std::string role, GameSession& session)
{
	sf::Packet wordPacket;
	if (role == "master")
	{
		wordPacket << "board";
		for (const auto& card : session.gameState.gameBoard)
		{
			wordPacket << card.gameWord << static_cast<int>(card.color);

			std::cout << "\n#\n" << card.gameWord << static_cast<int>(card.color) << card.order << "\n#\n";
		}
	}
	else
	{
		wordPacket << "board";
		for (const auto& card : session.gameState.gameBoard)
		{
			wordPacket << card.gameWord << 4;
		}
	}

	if (socket.send(wordPacket) == sf::Socket::Done);
	{
		return true;
	}
	std::cerr << "\nWordboard sending failed\n";
	return false;

}
bool sendNextTurn(sf::TcpSocket& socket)
{
	sf::Packet resp;
	resp << "next";
	if (socket.send(resp) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "\nNext turn sending failed\n";
	return false;
}
bool sendReveal(sf::TcpSocket& socket, int order, int color)
{
	sf::Packet resp;
	resp << "reveal" << order << color;
	if (socket.send(resp) == sf::Socket::Done)
	{
		std::cout << "\nSent reveal packet: order=" << order << " color=" << color << '\n';
		return true;
	}
	std::cerr << "\nReaviling sending failed\n";
	return false;
}
bool sendFlagUpdate(sf::TcpSocket& socket, int order, int flagInd)
{
	sf::Packet resp;
	resp << "flag" << order << flagInd;
	if (socket.send(resp) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "\nFlag update sending failed\n";
	return false;
}
bool sendAddTime(sf::TcpSocket& socket, int timeVal)
{
	sf::Packet resp;
	resp << "timer_add" << timeVal;
	if (socket.send(resp) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "\nAdd time to timer sending failed\n";
	return false;
}
bool sendUpdateTimer(sf::TcpSocket& socket, float length)
{
	sf::Packet resp;
	resp << "update_timer" << length;
	if (socket.send(resp) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "\nUpdate timer sending failed\n";
	return false;
}
bool sendTimerSetUp(sf::TcpSocket& socket, float val, int team)
{
	sf::Packet resp;
	resp << "set_timer" << team << val;
	if (socket.send(resp) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "\nTime set up sending failed\n";
	return false;
}
bool sendChatMsg(sf::TcpSocket& socket, int team, std::string msg)
{
	sf::Packet resp;
	resp << "chat" << team << msg;
	if (socket.send(resp) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "\nChat msg sending failed\n";
	return false;
}
bool sendWinner(sf::TcpSocket& socket, int team)
{
	sf::Packet resp;
	resp << "winner" << team;
	if (socket.send(resp) == sf::Socket::Done)
	{
		std::cout << "\nThe winner team is: " << team << ' ';
		return true;
	}
	std::cerr << "\nWinner sending failed\n";
	return false;
}
void handlePacket(sf::Packet& packet, Player& sender, GameSession& session, std::vector<Player>& players)
{
	std::cout << "got msg from: " << sender.name << '\n';
	std::string command;
	packet >> command;
	if (command == "params")
	{
		std::string role;
		int team;
		packet >> role >> team;

		sender.setRole(role);
		sender.setTeam(team);

		sf::Packet response;
		response << "role_set" << role << team;
		sender.socket->send(response);
	}
	else if (command == "board" && sender.isHost)
	{
		std::cout << "\nstart msg got from: " << sender.name << "\n";
		session.isStarted = true;

		for (auto& player : players)
		{
			std::cout << "\n**\n";
			sf::Packet wordPacket;

			if (sendWords(*player.socket, player.role, session))
			{
				std::cout << "\nwords sent to: " << player.name << ".\n";
			}
		}
	}
	else if (command == "start" && sender.isHost)
	{
		session.gameState.timer.restart().asSeconds();
		for (auto& player : players)
		{
			if (sendStart(*player.socket))
			{
				sendTimerSetUp(*player.socket, session.gameState.timerVal, 0);
				std::cout << "Start signal sent to " << player.name << '\n';
			}
		}
	}
	else if (command == "prep" && sender.isHost)
	{
		int size = 0;
		packet >> size;
		if (size > 0)
		{
			slots[0].resize(size);
			slots[1].resize(size);
			for (auto& slot : slots[0])
			{
				slot = "free";
				std::cout << slot << " ";
			}
			std::cout << "\n";
			for (auto& slot : slots[1])
			{
				slot = "free";
				std::cout << slot << " ";
			}
			std::cout << "\n";

			std::cout << "lobby was created.\n" << "Each team has " << size << " slots\n";

		}
	}
	else if (command == "slot")
	{
		std::cout << "\nGot request on slot changing\n";
		int team;
		int order;
		packet >> team >> order;

		if (slots[team][order] == "free")
		{
			if (sender.slot == -1)
			{
				sender.setTeam(team);
				sender.setRole(order == 0 ? "master" : "player");
				slots[team][order] = sender.name;
				sender.slot = order;
			}
			else
			{
				slots[sender.team][sender.slot] = "free";
				std::cout << "Slot " << sender.slot << " in team " << sender.team << " now freed. \n";
				sender.setTeam(team);
				sender.setRole(order == 0 ? "master" : "player");
				slots[team][order] = sender.name;
				sender.slot = order;
			}

			sendSlotAprove(1, *sender.socket);
			std::cout << "Slot " << order << " in team " << team << " now taken by " << sender.name << '\n';
			sf::Packet lobbyState = getLobbyState(players);

			for (const auto& player : players)
			{
				std::cout << "socket: " << sender.socket << "\nname: " << sender.name << '\n';
				sendLobbyState(*player.socket, lobbyState);
				std::cout << "Lobby state sent to " << sender.name << "\n\n";
			}
		}
		else
		{
			sendSlotAprove(0, *sender.socket);
			std::cout << "slot is occupied by " << slots[team][order] << "\n";
		}
	}
	else if (command == "free" && sender.slot != (-1))
	{

		slots[sender.team][sender.slot] = "free";
		std::cout << "Slot " << sender.slot << " in team " << sender.team << " now freed. \n";
		sender.setTeam(-1);
		sender.slot = -1;
		sender.setRole("spec");
		sendSlotAprove(1, *sender.socket);

		sf::Packet lobbyState = getLobbyState(players);
		for (const auto& player : players)
		{
			std::cout << "socket: " << sender.socket << "\nname: " << sender.name << '\n';
			sendLobbyState(*player.socket, lobbyState);
			std::cout << "Lobby state sent to " << sender.name << "\n\n";
		}
	}
	else if (command == "card")
	{
		int order = 0;

		packet >> order;

		if (sender.team == session.gameState.curTeam)
		{
			if (sender.role == "player")
			{
				if (session.gameState.curRole == "player")
				{
					int cardCol = static_cast<int>(session.gameState.gameBoard[order].color);
					session.gameState.gameBoard[order].isRevealed = true;

					std::cout << "\n################\nsender team: " << sender.team << "\nclicked card color to int: "
						<< cardCol << "\nplayer role: " << sender.role
						<< "\ncurrent role to play: " << session.gameState.curRole
						<< "\ncurrent team to play: " << session.gameState.curTeam << "\n################\n";

					if (cardCol - sender.team != 1)
					{
						session.gameState.reveal(order);
						session.gameState.nextTurn();
						for (const auto& player : players)
						{
							sendReveal(*player.socket, order, cardCol);
							if (cardCol == 3)
							{
								sendWinner(*player.socket, session.gameState.curTeam);
							}
							else if(cardCol==0)
							{
								sendNextTurn(*player.socket);
							}
							else
							{
								session.gameState.updateScore(session.gameState.curTeam);
							}
						}
						session.gameState.timer.restart();
						std::cout << "\nRevealed word No. " << order << '\n';
						std::cout << "\n Cur turn: " << session.gameState.curTurn << "\n";
						std::cout << "\n Cur role: " << session.gameState.curRole << "\n";
						std::cout << "\n Cur team: " << session.gameState.curTeam << "\n";

					}
					else if (cardCol - sender.team == 1)
					{
						for (const auto& player : players)
						{
							sendAddTime(*player.socket, 15);
							if (sendReveal(*player.socket, order, cardCol))
							{
								std::cout << "\n Sent reveal msg to " << player.name << '\n';
							}
						}
						std::cout << "\nRevealed word No. " << order << '\n';
						session.gameState.reveal(order);
						session.gameState.updateScore(sender.team);
						session.gameState.turnTimeLeft += 15;
						if (session.gameState.existsWinner)
						{
							for (const auto& player : players)
							{
								sendWinner(*player.socket, session.gameState.curTeam);
							}
						}
					}
				}
			}
		}
	}
	else if (command == "flag")
	{

	}
	else if (command == "chat")
	{
		if (sender.team == session.gameState.curTeam)
		{
			if (sender.role == "master")
			{
				std::string msg;
				std::string res;
				while (packet >> msg)
				{
					res += msg + " ";
				}
				if (!res.empty())
				{
					res.pop_back();
				}
				std::cout << "\n parsed msg: " << res << '\n';
				for (const auto& client : players)
				{

					if (sendChatMsg(*client.socket, session.gameState.curTeam, res))
					{
						std::cout << "\n Sent " << sender.name << "'s msg to client " << client.name << '\n';
					}
					if (sendNextTurn(*client.socket))
					{
						std::cout << "\n Sent " << sender.name << " next turn alert.\n";
					}
				}
				session.gameState.nextTurn();
				session.gameState.timer.restart();
				std::cout << "\n Cur turn: " << session.gameState.curTurn << "\n";
				std::cout << "\n Cur role: " << session.gameState.curRole << "\n";
				std::cout << "\n Cur team: " << session.gameState.curTeam << "\n";
			}
		}
	}
}



int main()
{
	sf::Clock broadcastClock;
	const sf::Time BROADCAST_INTERVAL = sf::seconds(1.0f);
	SetConsoleOutputCP(CP_UTF8);
	std::vector<int> numCol = getRandomUniqueNumbersAsc(MAX_WORDS, 24);
	std::vector<WordCard> words = getWordsList(WORDS_PATH, numCol);
	std::sort(words.begin(), words.end(), [](const WordCard& a, const WordCard& b)
		{
			return a.order < b.order;
		});

	GameSession game(words);

	sf::TcpListener listener;
	if (listener.listen(54000) != sf::Socket::Done)
	{
		std::cerr << "Error: Unable to bind listener to port 54000.\n";
		return -1;
	}

	listener.setBlocking(false);

	sf::SocketSelector  selector;
	selector.add(listener);

	std::vector<Player> clients;
	static int nextClientId = 0;

	std::cout << "Server is running. Waiting for clients...\n";
	while (true)
	{
		if (selector.wait())
		{
			if (selector.isReady(listener))
			{
				auto newClient = std::make_unique<sf::TcpSocket>();
				if (listener.accept(*newClient) == sf::Socket::Done)
				{
					sf::Packet helloPack;
					if (newClient->receive(helloPack) == sf::Socket::Done)
					{
						int clientId = nextClientId++;
						std::string command;
						helloPack >> command;
						if (command == "name")
						{
							std::string nickName;
							helloPack >> nickName;
							if (clientId == 0)
							{
								clients.push_back({ std::move(newClient), clientId, 0, -1, "spec", nickName, true });
								addPlayer(clients.back());
								std::cout << "tried to add player: " << clients.back().name << " to list.\n";
							}
							else
							{
								clients.push_back({ std::move(newClient), clientId, 0, -1,  "spec", nickName });
								addPlayer(clients.back());
								std::cout << "tried to add player: " << clients.back().name << " to list.\n";
								sendTeamSize(*clients.back().socket, slots[0].size());
								//sf::Packet lobbyState = getLobbyState(clients);
								//sendLobbyState(*clients.back().socket, lobbyState);
								std::cout << "sent team size to " << clients.back().name << '\n';
							}

							std::cout << "New client: " << nickName << "with id: " << clientId << " connected\n";
						}
						selector.add(*clients.back().socket);
					}
					else
					{
						std::cerr << "Error: Failed to receive packet.\n";
					}
				}
			}
			else
			{
				for (size_t i = 0; i < clients.size(); ++i)
				{
					sf::TcpSocket& client = *clients[i].socket;
					if (selector.isReady(client))
					{
						sf::Packet packet;
						if (client.receive(packet) == sf::Socket::Done)
						{
							clients[i].lastActivityClock.restart();
							handlePacket(packet, clients[i], game, clients);
						}
						else if (clients[i].lastActivityClock.getElapsedTime() > TIMEOUT)
						{
							std::cout << "Client " << clients[i].clientId << " timed out.\n";
							selector.remove(client);
							client.disconnect();
							clients.erase(clients.begin() + i);
							i--;
						}

						if (broadcastClock.getElapsedTime() >= BROADCAST_INTERVAL && game.isStarted)
						{
							float val = game.gameState.timeLeft;

							for (auto& client : clients)
							{
								sendUpdateTimer(*client.socket, val);
							}

							broadcastClock.restart();
						}
					}
				}
			}
		}
	}
	return 0;
}
