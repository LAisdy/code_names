#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <iostream>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <windows.h>
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <queue>
#include <string>
#include <sstream>
#include <mutex>
#include <codecvt>
#include <locale>

//u8 converter:
std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;


//enums:
enum state
{
	MENU = 0,
	LOBBYPREP = 1,
	JOININGPREP = 2,
	HOSTING = 3,
	JOINING = 4,
	LOBBY = 5,
	PLAYING = 6,
	SETTINGS = 7
};

enum Color
{
	WHITE = 0,
	BLUE = 1,
	RED = 2,
	BLACK = 3,
	NONE = 4
};

//used_colors:

sf::Color SANDSTONE(196, 187, 145);
sf::Color DRIFTEDWOOD(156, 148, 115);
sf::Color PALE_LEMON(239, 228, 176);
sf::Color DARK_UMBER(36, 20, 6);
sf::Color COPPER_BROWN(112, 62, 18);
sf::Color WALNUT(69, 38, 11);
sf::Color SCARLETT_RED(237, 28, 36);
sf::Color CRIMSON(95, 10, 14);
sf::Color FOREST_GREEN(34, 117, 76);
sf::Color PINE_GREEN(13, 71, 30);
sf::Color BLACKCOL(0, 0, 0);
sf::Color WHITECOL(255, 255, 255);
sf::Color DARK_GRAY(57, 57, 57);
sf::Color DEF_RED(153, 33, 23);
sf::Color DEEP_RED(117, 25, 18);
sf::Color DEF_BLUE(29, 97, 153);
sf::Color DEEP_BLUE(23, 78, 122);
sf::Color BRIGHT_RED(255, 47, 13);

//vars:

sf::Font font;

sf::Vector2f  TEAM_PANEL_DIMS = { 200.f,530.f };
sf::Vector2f CHAT_PANEL_DIMS = { 190.f, 85.f };

std::string nickName;

std::queue<sf::Packet> messageQueue;

std::atomic<bool> isConnected = false;
std::atomic<state> currentState = MENU;
std::atomic<bool> threadStarted = false;
std::atomic<bool> isListening{ false };

std::mutex queueMutex;

std::thread listeningThread;

std::unordered_map<std::string, int> players;

void centerText(sf::Text& text, sf::RectangleShape& rect)
{
	sf::FloatRect textBounds = text.getLocalBounds();
	sf::FloatRect panelBounds = rect.getGlobalBounds();
	text.setPosition(
		panelBounds.left + (panelBounds.width - textBounds.width) / 2.f - textBounds.left,
		panelBounds.top + (panelBounds.height - textBounds.height) / 2.f - textBounds.top
	);
}


//sending msgs:

//server side processing done
bool sendCardClick(int order, sf::TcpSocket& socket)
{
	sf::Packet clickedCard;
	clickedCard << "card" << order;
	if (socket.send(clickedCard) == sf::Socket::Done)
	{
		return true;
	}
	return false;
}


bool sendFlagClick(int cardOrder, int flagOrder, sf::TcpSocket& socket)
{
	sf::Packet clickedCard;
	clickedCard << "flag" << cardOrder << flagOrder;
	if (socket.send(clickedCard) == sf::Socket::Done)
	{
		return true;
	}
	return false;
}

//server side processing done
bool sendWelcomeMsg(std::string& name, sf::TcpSocket& socket)
{
	sf::Packet nickName;
	nickName << "name" << name;
	if (socket.send(nickName) == sf::Socket::Done)
	{
		return true;
	}
	return false;
}

//server side processing done
bool sendPlayerParams(int team, std::string& role, sf::TcpSocket& socket)
{
	sf::Packet paramMsg;
	paramMsg << "params" << team << role;
	if (socket.send(paramMsg) == sf::Socket::Done)
	{
		return true;
	}
	return false;
}

bool sendTurnEnd(sf::TcpSocket& socket)
{
	sf::Packet packet;
	packet << "end";
	if (socket.send(packet) == sf::Socket::Done)
	{
		return true;
	}
	return false;
}

//server side processing done
bool sendChosenSlot(int team, int order, sf::TcpSocket& socket)
{
	sf::Packet request;
	request << "slot" << team << order;
	if (socket.send(request) == sf::Socket::Done)
	{
		return true;
	}
	return false;
}

//server side processing done
bool sendLobbyParams(int size, sf::TcpSocket& socket)
{
	sf::Packet request;
	request << "prep" << size;
	if (socket.send(request) == sf::Socket::Done)
	{
		return true;
	}
	return false;
}

//server side processing done
bool sendFreeSlot(sf::TcpSocket& socket)
{
	sf::Packet request;
	request << "free";
	if (socket.send(request) == sf::Socket::Done)
	{
		return true;
	}
	return false;
}

//server side processing done
bool sendStartGame(sf::TcpSocket& socket)
{
	sf::Packet request;
	request << "start";
	if (socket.send(request) == sf::Socket::Done)
	{
		return true;
	}
	return false;
}

//server side processing done
bool sendPreStartGame(sf::TcpSocket& socket)
{
	sf::Packet request;
	request << "board";
	if (socket.send(request) == sf::Socket::Done)
	{
		return true;
	}
	return false;
}

//server side proccessing done 
bool sendChatMsg(sf::TcpSocket& socket, sf::Text& msg)
{
	sf::Packet chatMsg;
	std::basic_string<sf::Uint8> utf8Msg = msg.getString().toUtf8();
	std::string curMsg(utf8Msg.begin(), utf8Msg.end());
	chatMsg << "chat" << curMsg;
	if (socket.send(chatMsg) == sf::Socket::Done)
	{
		return true;
	}
	std::cerr << "Failed sending chat msg\n";
	return false;
}

//structs:

struct FlagPanel
{
	FlagPanel()
	{
		redFlag.setSize({ 10, 10.f });
		greenFlag.setSize({ 10.f, 10.f });


		redFlag.setFillColor(redOffColor);
		greenFlag.setFillColor(greenOffColor);
	}

	sf::RectangleShape redFlag;
	sf::RectangleShape greenFlag;

	sf::Color redOffColor = CRIMSON;
	sf::Color redOnColor = SCARLETT_RED;

	sf::Color greenOnColor = FOREST_GREEN;
	sf::Color greenOffColor = PINE_GREEN;

	bool redOn = false;
	bool greenOn = false;

	bool isHoveringRed(sf::RenderWindow& window) const
	{
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
		return redFlag.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
	}

	bool isHoveringGreen(sf::RenderWindow& window) const
	{
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
		return greenFlag.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
	}


	bool isClickedRed(sf::Event& event, sf::RenderWindow& window) const
	{
		return (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
			redFlag.getGlobalBounds().contains(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y))));

	}

	bool isClickedGreen(sf::Event& event, sf::RenderWindow& window) const
	{
		return (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
			greenFlag.getGlobalBounds().contains(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y))));

	}

	void updateRedFlag()
	{
		redOn = !redOn;
		if (redOn)
		{
			redFlag.setFillColor(redOnColor);
		}
		else
		{
			redFlag.setFillColor(redOffColor);
		}
	}

	void updateGreenFlag()
	{
		greenOn = !greenOn;
		if (greenOn)
		{
			greenFlag.setFillColor(greenOnColor);
		}
		else
		{
			greenFlag.setFillColor(greenOffColor);
		}
	}

	void draw(sf::RenderWindow& window)
	{
		window.draw(redFlag);
		window.draw(greenFlag);
	}
};

struct Timer
{
	sf::RectangleShape timerShape;
	sf::Color startCol = sf::Color::Green;
	sf::Color endCol = sf::Color::Red;

	float timeSet = 0.f;
	float timerWidth = 190.f;
	float timeLeft = timeSet;
	bool isStopped = false;

	Timer(float val)
	{
		timeSet = val;
		timeLeft = val;
	}

	sf::Color interpolateColor(const sf::Color& start, const sf::Color& end, float factor)
	{
		sf::Uint8 red = static_cast<sf::Uint8>(start.r + factor * (end.r - start.r));
		sf::Uint8 green = static_cast<sf::Uint8>(start.g + factor * (end.g - start.g));
		sf::Uint8 blue = static_cast<sf::Uint8>(start.b + factor * (end.b - start.b));
		return sf::Color(red, green, blue);
	}

	void update(float leftTime)
	{
		float deltaTime = timeLeft - leftTime;
		timeLeft = leftTime;
		if (timeLeft < 0.f)
		{
			timeLeft = 0.f;
		}
		float curWidth = timerShape.getSize().x;
		float step = curWidth - timerWidth * deltaTime / timeSet;
		if (step < 0.f)
		{
			step = 0.f;
		}
		timerShape.setSize(sf::Vector2f(step, timerShape.getSize().y));

		float factor = timeLeft / timeSet;
		timerShape.setFillColor(interpolateColor(endCol, startCol, factor));
	}

	void draw(sf::RenderWindow& window)
	{
		if (!isStopped)
		{
			window.draw(timerShape);
		}
	}
};

struct WordCard
{
	bool isRevealed = false;
	int order = 0;

	std::vector<std::string> redFlaggers;
	std::vector<std::string> greenFlaggers;

	FlagPanel flags;

	sf::RectangleShape wordCard;

	sf::Text word;
	WordCard()
	{

	}

	WordCard(FlagPanel flg)
		:flags(flg)
	{

	}
	WordCard& operator=(const WordCard& other) {
		if (this == &other) return *this;

		order = other.order;
		redFlaggers = other.redFlaggers;
		greenFlaggers = other.greenFlaggers;
		flags = other.flags;
		wordCard = other.wordCard;
		word = other.word;
		isRevealed = other.isRevealed;

		return *this;
	}

	void setUp(float startX, float startY, float stepX, float stepY, sf::Font& font, const std::string& wrd, sf::Color col = SANDSTONE)
	{

		wordCard.setPosition(startX + stepX, startY + stepY);
		wordCard.setFillColor(col);

		flags.redFlag.setFillColor(DEF_RED);
		flags.redFlag.setPosition(wordCard.getPosition().x + wordCard.getSize().x - 5 - flags.redFlag.getSize().x,
			wordCard.getPosition().y + wordCard.getSize().y - 5 - flags.redFlag.getSize().y);
		col.toInteger() == 255 ? flags.redFlag.setOutlineColor(WHITECOL) : flags.redFlag.setOutlineColor(BLACKCOL);
		flags.redFlag.setOutlineThickness(2.f);

		flags.greenFlag.setFillColor(FOREST_GREEN);
		flags.greenFlag.setPosition(wordCard.getPosition().x + wordCard.getSize().x - (5 + flags.greenFlag.getSize().x) * 2,
			wordCard.getPosition().y + wordCard.getSize().y - (5 + flags.greenFlag.getSize().y));
		col.toInteger() == 255 ? flags.greenFlag.setOutlineColor(WHITECOL) : flags.greenFlag.setOutlineColor(BLACKCOL);
		flags.greenFlag.setOutlineThickness(2.f);


		word.setFont(font);
		word.setString(converter.from_bytes(wrd));
		word.setCharacterSize(20);
		col.toInteger() == 255 ? word.setFillColor(WHITECOL) : word.setFillColor(DARK_UMBER);

		sf::FloatRect textBounds = word.getLocalBounds();
		sf::FloatRect labelBounds = wordCard.getGlobalBounds();

		while (textBounds.width > labelBounds.width && word.getCharacterSize() > 1)
		{
			word.setCharacterSize(word.getCharacterSize() - 2);
			textBounds = word.getLocalBounds();
		}
		centerText(word, wordCard);
	}

	void draw(sf::RenderWindow& window)
	{
		window.draw(wordCard);
		window.draw(flags.greenFlag);
		window.draw(flags.redFlag);
		window.draw(word);
	}

	void setDims(sf::Vector2f flDims, sf::Vector2f lnDims)
	{
		wordCard.setSize(lnDims);
		flags.redFlag.setSize(flDims);
		flags.greenFlag.setSize(flDims);
	}

	bool isHovering(sf::RenderWindow& window)
	{
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
		bool hoverCard = wordCard.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
		bool hoverRedFlag = flags.redFlag.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
		bool hoverGreenFlag = flags.greenFlag.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
		return (hoverCard && !hoverRedFlag && !hoverGreenFlag);
	}

	bool isClicked(sf::Event& event, sf::RenderWindow& window)
	{
		bool isInCard = event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
			wordCard.getGlobalBounds().contains(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));
		bool isInRedFlag = event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
			flags.redFlag.getGlobalBounds().contains(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));
		bool isInGreenFlag = sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
			flags.greenFlag.getGlobalBounds().contains(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

		return (isInCard && !isInRedFlag && !isInGreenFlag);
	}

	void updateCardColor(sf::Color& color)
	{
		wordCard.setFillColor(color);
	}

	void revealWord(int col)
	{
		isRevealed = true;
		if (col == 3)
		{
			updateCardColor(BLACKCOL);
			word.setFillColor(WHITECOL);
		}
		else if (col == 2)
		{
			updateCardColor(DEF_RED);
		}
		else if (col == 1)
		{
			updateCardColor(DEF_BLUE);
		}
		else if (col == 0)
		{
			updateCardColor(WHITECOL);
		}
	}
};

struct InputBox
{
	sf::RectangleShape label;
	sf::RectangleShape inputBox;
	sf::Text text;
	sf::Text labelText;

	sf::Color labelColor = COPPER_BROWN;
	sf::Color inputBoxColor = SANDSTONE;
	sf::Color boxTextColor = DARK_UMBER;
	bool isWriting = false;

	void setInpBoxParams(float x, float y, float labelWidth, float inputWidth, float height, sf::Font& font)
	{
		//label:
		label.setSize({ labelWidth, height });
		label.setPosition(x, y);
		label.setFillColor(labelColor);
		labelText.setFillColor(boxTextColor);
		labelText.setFont(font);

		centerText(labelText, label);

		//input box:
		inputBox.setSize({ inputWidth, height });
		inputBox.setPosition(x + labelWidth, y);
		inputBox.setFillColor(inputBoxColor);
		text.setFillColor(boxTextColor);
		text.setFont(font);

		centerText(text, inputBox);
	}

	bool isHovering(sf::RenderWindow& window)
	{
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
		return inputBox.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
	}
	bool isClicked(sf::Event& event, sf::RenderWindow& window)
	{
		return (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
			inputBox.getGlobalBounds().contains(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y))));
	}

	void updateBoxColor(sf::Color& color)
	{
		inputBox.setFillColor(color);
	}
	void handleInput(sf::Event& event)
	{
		if (isWriting && event.type == sf::Event::TextEntered)
		{
			if (event.text.unicode < 128)
			{
				if (event.text.unicode == '\b')
				{
					if (!text.getString().isEmpty())
					{
						std::string content = text.getString();
						content.pop_back();
						text.setString(content);
					}
				}
				else if (event.text.unicode == '\r' || event.text.unicode == 27)
				{
					isWriting = false;
				}
				else
				{
					std::string content = text.getString();
					content += static_cast<char>(event.text.unicode);
					text.setString(content);
				}
				centerText(text, inputBox);
			}
		}
	}
	void draw(sf::RenderWindow& window)
	{
		window.draw(label);
		window.draw(labelText);
		window.draw(inputBox);
		window.draw(text);
	}
};

struct MenuBtn
{

	sf::RectangleShape shape;
	sf::Text text;
	int order;
	bool isHovering(sf::RenderWindow& window)
	{
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
		return shape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
	}
	bool isClicked(sf::Event& event, sf::RenderWindow& window)
	{
		return (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
			shape.getGlobalBounds().contains(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y))));

	}
};

struct PlayerSlot
{
	sf::RectangleShape shape;
	sf::Text text;
	std::string playerName;
	std::string role;
	bool isOccupied = false;
	PlayerSlot() = default;
	PlayerSlot(sf::Color slotCol, sf::Vector2f dims, sf::Color txtCol, sf::Vector2f orig, sf::Font& font)
	{
		shape.setOrigin(orig);
		shape.setSize(dims);
		shape.setFillColor(slotCol);

		text.setCharacterSize(20);
		text.setFillColor(txtCol);
		text.setFont(font);

		updateTextPos();
	}

	bool isClicked(sf::Event& event, sf::RenderWindow& window)
	{
		return (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
			shape.getGlobalBounds().contains(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y))));
	}

	void updateTextPos()
	{
		if (text.getString().isEmpty())
		{
			text.setString(" ");
		}
		centerText(text, shape);
	}

	void draw(sf::RenderWindow& window)
	{
		window.draw(shape);
		window.draw(text);
	}

};

struct LobbyPanel
{
	int teamSize = 0;

	PlayerSlot masterSlot;

	sf::Text teamName;

	std::vector<PlayerSlot> playerSlots;
	LobbyPanel() = default;

	LobbyPanel(int size, const PlayerSlot& master, const std::vector<PlayerSlot>& slots)
		: teamSize(size), masterSlot(master), playerSlots(slots)
	{
	}

	void draw(sf::RenderWindow& window)
	{
		window.draw(teamName);
		masterSlot.draw(window);

		for (auto& slot : playerSlots)
		{
			slot.draw(window);
		}
	}
	void updateTitlePos()
	{
		auto x = masterSlot.shape.getPosition().x;
		auto y = masterSlot.shape.getPosition().y - 84;
		teamName.setOrigin(0.f, 0.f);
		teamName.setPosition(x, y);
	}
};

struct BoardPanel
{
	std::vector<std::vector<WordCard>> board;
	sf::RectangleShape boardShape;

	BoardPanel(std::vector<std::vector<WordCard>>& boardOfWords)
		:board(boardOfWords)
	{
		board.resize(6, std::vector<WordCard>(4));

		boardShape.setOrigin(.0f, .0f);
		boardShape.setSize({ 440,440 });
		boardShape.setFillColor(PALE_LEMON);
		boardShape.setPosition(TEAM_PANEL_DIMS.x + 10.f, 5.f);
	}

	void draw(sf::RenderWindow& window)
	{
		window.draw(boardShape);
		for (auto& card : board)
		{
			for (WordCard& elem : card)
			{
				elem.draw(window);
			}
		}
	}
};

struct TeamPanel
{
	sf::TcpSocket& mySocket;
	PlayerSlot masterSlot;
	std::vector<PlayerSlot> playerSlots;
	Timer timer;
	sf::RectangleShape teamShape;
	sf::RectangleShape chatShape;

	sf::RectangleShape hintButton;
	sf::RectangleShape inputShape;
	std::vector<sf::Text> chatMessages{};

	std::string hintWord;
	sf::Text curMsg{};
	int hintCount = 0;
	int hintInd = 0;

	bool isActive = false;
	bool isWriting = false;
	std::vector<sf::Text> chatMsgs{};

	TeamPanel(sf::TcpSocket& socket, PlayerSlot& master, std::vector<PlayerSlot>& players, float timeSet = 0)
		:mySocket(socket), masterSlot(master), playerSlots(players), timer(timeSet)
	{
	}

	void setUpPanel(sf::Vector2f pos, sf::Vector2f orig, sf::Vector2f dims, sf::Color& col)
	{
		teamShape.setPosition(pos);
		teamShape.setOrigin(orig);
		teamShape.setSize(dims);
		teamShape.setFillColor(col);
	}

	void seUpChat(sf::Vector2f orig, sf::Vector2f dims, sf::Color& col)
	{
		chatShape.setOrigin(orig);
		chatShape.setSize(dims);
		chatShape.setPosition(teamShape.getPosition().x + 5.f, teamShape.getPosition().y + teamShape.getSize().y - chatShape.getSize().y - 5.f);
		chatShape.setFillColor(col);
		curMsg.setFont(font);
		curMsg.setFillColor(WALNUT);
	}

	void setUpHintButton(sf::Color& col)
	{
		hintButton.setSize({ 20.f,20.f });
		hintButton.setPosition(chatShape.getPosition().x + chatShape.getSize().x - 25, chatShape.getPosition().y + chatShape.getSize().y - 25);
		//std::cout << "\nhintPos: " << hintButton.getPosition().x << " " << hintButton.getPosition().y << std::endl;
		hintButton.setOrigin(0.f, 0.f);
		hintButton.setFillColor(col);
	}

	void setUpInputBox(sf::Color& col)
	{
		inputShape.setPosition(chatShape.getPosition().x + 5, hintButton.getPosition().y);
		inputShape.setOrigin(0.f, 0.f);
		inputShape.setSize({ chatShape.getSize().x - 15 - hintButton.getSize().x, hintButton.getSize().y });
		inputShape.setFillColor(col);
	}

	void setHint(std::string& word, int count)
	{
		hintWord = word;
		hintCount = count;
	}

	void setUpTimer()
	{
		timer.timerShape.setSize({ 190.f, 10.f });
		timer.timerShape.setPosition({ chatShape.getPosition().x, chatShape.getPosition().y - 20 });
		timer.timerShape.setFillColor(sf::Color::Green);
	}

	void decreaseTimer(float deltaTime)
	{
		timer.update(deltaTime);
	}

	void increaseTimer(float deltaTime)
	{
		float delta = -1 * deltaTime;
		timer.update(deltaTime);
	}

	sf::Packet getHint()
	{
		sf::Packet hint;
		hint << "hint" << hintWord << hintCount;
		return hint;
	}

	bool isHoveringChat(sf::RenderWindow& window)
	{
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
		return chatShape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
	}

	bool isHoveringHint(sf::RenderWindow& window)
	{
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
		return hintButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
	}

	bool isClickedHint(sf::Event& event, sf::RenderWindow& window)
	{
		return (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
			hintButton.getGlobalBounds().contains(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y))));
	}

	bool isHoveringInput(sf::RenderWindow& window)
	{
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
		return inputShape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
	}

	bool isClickedInput(sf::Event& event, sf::RenderWindow& window)
	{
		return (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
			inputShape.getGlobalBounds().contains(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y))));
	}

	void handleUtf8Input(sf::Event& event)
	{
		if (isWriting && event.type == sf::Event::TextEntered)
		{
			size_t x = curMsg.getString().getSize();
			if (event.text.unicode == '\b')
			{
				if (!curMsg.getString().isEmpty())
				{
					sf::String content = curMsg.getString();
					content.erase(content.getSize() - 1, 1);
					curMsg.setString(content);
				}
			}
			else if (event.text.unicode == 27)
			{
				isWriting = false;
			}
			else if (event.text.unicode == '\r')
			{
				isWriting = false;
				curMsg.setPosition(inputShape.getPosition().x + 5, inputShape.getPosition().y + 5);
				if (sendChatMsg(mySocket, curMsg))
				{
					curMsg.setString("");
					isWriting = false;
				}
			}
			else
			{
				if (x < 20)
				{
					sf::String content = curMsg.getString();
					content += event.text.unicode;
					curMsg.setString(content);
				}
			}

			if (x <= 14)
			{
				curMsg.setCharacterSize(18);
			}
			else if (x > 14 && x <= 15)
			{

				curMsg.setCharacterSize(16);
			}
			else if (x >= 16 && x < 19)
			{
				curMsg.setCharacterSize(14);
			}
			else if (x >= 19)
			{
				curMsg.setCharacterSize(12);
			}

			//std::string temp= curMsg.getString();
			//std::cout << temp << '\n';

			centerText(curMsg, inputShape);
		}
	}

	void addMsgToChat(sf::String msg)
	{
		sf::Text sfMsg;
		sfMsg.setFont(font);
		sfMsg.setFillColor(WALNUT);
		sfMsg.setString(msg);
		chatMsgs.push_back(sfMsg);
	}

	void draw(sf::RenderWindow& window)
	{
		window.draw(teamShape);
		window.draw(chatShape);
		window.draw(hintButton);
		window.draw(inputShape);
		if (!chatMsgs.empty())
		{
			for (size_t i = 0; i < std::min(chatMsgs.size() - hintInd, size_t(3)); ++i)
			{
				chatMsgs[i + hintInd].setCharacterSize(16);
				chatMsgs[i + hintInd].setFillColor(WALNUT);
				chatMsgs[i + hintInd].setFont(font);
				chatMsgs[i + hintInd].setPosition({ chatShape.getPosition().x + 5, chatShape.getPosition().y + 3 + 18 * i });
				window.draw(chatMsgs[i + hintInd]);
			}
		}
		window.draw(curMsg);
	}
};

struct EndGamePanel
{
	sf::RectangleShape backShape;
	sf::RectangleShape panelShape;
	sf::Text endText;

	sf::Color backColor = { 0,0,0,128 };
	sf::Color panelColor = FOREST_GREEN;

	sf::Font font;

	bool isActive = false;
	int result = 0b00;

	EndGamePanel(sf::Font& _font)
		:font(_font)
	{
		backShape.setSize({ 860,540 });
		backShape.setFillColor(backColor);
		backShape.setPosition(0.f, 0.f);
		panelShape.setSize({ 600, 450 });
		panelShape.setFillColor(panelColor);
		panelShape.setPosition(130, 45);
	}

	void callGameEnd(int wonTeam, int myTeam)
	{
		isActive = true;
		result = wonTeam;
		if (wonTeam == myTeam)
		{
			sf::String msg = "You Won";
			setText(msg);
		}
		else
		{
			sf::String msg = "You Lose";
			setText(msg);
		}
	}

	void setText(sf::String& _text)
	{
		endText.setString(_text);
		endText.setFont(font);
		endText.setFillColor(DEF_RED);
		endText.setCharacterSize(36);
		centerText(endText, panelShape);
	}

	void draw(sf::RenderWindow& window)
	{
		if (isActive)
		{
			window.draw(backShape);
			window.draw(panelShape);
			window.draw(endText);
		}
	}

	void setPanelCol(sf::Color& col)
	{
		panelShape.setFillColor(col);
	}
};

struct GameMngr
{
	BoardPanel& boardPanel;
	TeamPanel& leftPanel;
	TeamPanel& rightPanel;
	EndGamePanel& endPanel;
	int curTurn = 0b00;
	int myTeam = 0;
	bool isReady = false;

	void nextTurn()
	{
		curTurn = (curTurn + 0b01) & 0b11;

		if (curTurn & 0b10)
		{
			leftPanel.isActive = false;
			leftPanel.timer.isStopped = true;
			rightPanel.isActive = true;
			rightPanel.timer.isStopped = false;
		}
		else
		{
			leftPanel.isActive = true;
			leftPanel.timer.isStopped = false;
			rightPanel.isActive = false;
			rightPanel.timer.isStopped = true;
		}

	//std::cout << "\n######################################################\n";
	//std::cout << "\n Current turn: " << curTurn << '\n';
	//std::cout << "\n Left Panel active: " << leftPanel.isActive << '\n';
	//std::cout << "\n Right Panel active: " << rightPanel.isActive << '\n';
	//std::cout << "\n######################################################\n";

	}

	void draw(sf::RenderWindow& window)
	{
		leftPanel.draw(window);
		boardPanel.draw(window);
		rightPanel.draw(window);
		endPanel.draw(window);
	}
};

//words vec for game session:
std::vector<std::vector<WordCard>> boardOfWords(6, std::vector<WordCard>(4));

//funcs:

sf::Color servPackToColor(int col)
{
	switch (col)
	{
	case 0:
		return WHITECOL;
	case 1:
		return DEF_BLUE;
	case 2:
		return DEF_RED;
	case 3:
		return BLACKCOL;
	default:
		return SANDSTONE;
	}
}

std::pair<int, int> getWordOrder(int num)
{
	if (num < 0 || num > 23)
	{
		throw std::out_of_range("Number must be in the range 0 to 23.");
	}

	int n = num / 4;
	int c = num % 4;

	return { n, c };
}

void processResponse(sf::Packet& packet, LobbyPanel& left, LobbyPanel& right, GameMngr& gameScreen, sf::TcpSocket& socket)
{
	std::string command;
	packet >> command;

	if (command == "slot")
	{
		int res;
		packet >> res;
		if (res == 0)
		{
			std::cout << "Slot is occupied.\n";
		}
		else
		{
			std::cout << "You took this slot.\n";
		}
	}
	else if (command == "update_lobby")
	{
		//name-team-slot
		std::cout << "\n\nupdating lobby\n\n";
		int t = 0;
		int s = 0;
		std::string nick;
		while (packet >> nick >> t >> s)
		{
			std::cout << "\n*\n" << " nick: " << nick << "\n team: " << t << "\n slot: " << s << "\n*\n";
			if (t < 0 || t > 1)
			{
				std::cerr << "Invalid team index t : " << t << '\n';
				continue;
			}
			if ((t == 0 && (s < 0 || s > left.playerSlots.size())) ||
				(t == 1 && (s < 0 || s > right.playerSlots.size())))
			{
				std::cerr << "Invalid slot index s : " << s << '\n';
				continue;
			}

			auto& targetSlot = (t == 0)
				? (s == 0 ? left.masterSlot : left.playerSlots[s - 1])
				: (s == 0 ? right.masterSlot : right.playerSlots[s - 1]);

			if (nick == "free")
			{
				targetSlot.text.setString(" ");
				targetSlot.isOccupied = false;
				targetSlot.playerName.clear();
			}
			else
			{
				targetSlot.text.setString(nick);
				targetSlot.isOccupied = true;
				targetSlot.playerName = nick;
				players[nick] = t;
			}
			targetSlot.updateTextPos();
		}

	}
	else if (command == "size")
	{
		int size = 0;
		packet >> size;

		left.teamSize = size;
		right.teamSize = size;
	}
	else if (command == "board")
	{
		std::string word;
		int color = 0;
		int order = 0;
		FlagPanel flgPane;
		WordCard curWord(flgPane);


		int ord = 0;
		for (int j = 0; j < 6; ++j)
		{
			float startX = TEAM_PANEL_DIMS.x + 15.f;
			float startY = 10.f;
			for (int i = 0; i < 4; ++i)
			{
				packet >> word >> color;
				auto col = servPackToColor(color);
				curWord.setDims({ 10.f,10.f }, { 103.75f,67.5f });
				curWord.setUp(startX, startY, (curWord.wordCard.getSize().x + 5) * i, (curWord.wordCard.getSize().y + 5) * j, font, word, col);
				curWord.order = ord++;
				gameScreen.boardPanel.board[j][i] = (curWord);
				//std::cout << "\n***\nword: " << word << " color: " << color << " order: " << curWord.order << "\n***\n";
			}
		}

		if (sendStartGame(socket))
		{
			std::cout << "Board Ready.\n";
		}
	}
	else if (command == "start")
	{
		std::cout << "\nstarting\n";
		currentState = PLAYING;
		gameScreen.leftPanel.isActive = true;
	}
	else if (command == "set_timer")
	{
		int team = 0;
		float tVal = 0.f;
		packet >> team >> tVal;

		if (team > 1 || team < 0)
		{
			std::cerr << "Invalid team index t : " << team << '\n';
		}

		if (team == 0)
		{
			gameScreen.leftPanel.timer.timeSet = tVal;
			gameScreen.leftPanel.setUpTimer();
		}
		else
		{
			gameScreen.rightPanel.timer.timeSet = tVal;
			gameScreen.rightPanel.setUpTimer();
		}
	}
	else if (command == "update_timer")
	{
		float leftTime = 0.f;
		packet >> leftTime;
		(gameScreen.curTurn & 0b1) ? gameScreen.leftPanel.timer.update(leftTime) : gameScreen.rightPanel.timer.update(leftTime);
	}
	else if (command == "next")
	{
		gameScreen.nextTurn();
	}
	else if (command == "reveal")
	{
		int order = 0;
		int col = 0;
		packet >> order >> col;
		std::pair<int, int> inds = getWordOrder(order);
		gameScreen.boardPanel.board[inds.first][inds.second].revealWord(col);
		std::cout << "\nRevealed word no.: [" << inds.first << ' ' << inds.second << "] with color: " << col << '\n';
	}
	else if (command == "flag")
	{

	}
	else if (command == "chat")
	{
		int team = 0;
		std::string res;
		packet >> team >> res;

		if (res.empty())
		{
			std::cerr << "Error: received empty chat message\n";
			return;
		}

		sf::String sfRes = sf::String::fromUtf8(res.begin(), res.end());

		if (team == 0)
		{
			gameScreen.leftPanel.addMsgToChat(sfRes);
			if (gameScreen.leftPanel.chatMsgs.size() > 3)
			{
				gameScreen.leftPanel.hintInd++;
			}
			//std::cout << "\nadded msg: " << res << " to blue chatMessages\n";

		}
		else if (team == 1)
		{
			gameScreen.rightPanel.addMsgToChat(sfRes);
			if (gameScreen.rightPanel.chatMsgs.size() > 3)
			{
				gameScreen.rightPanel.hintInd++;
			}
			//std::cout << "\nadded msg: " << res << " to red chatMessages\n";
		}
	}
	else if (command == "winner")
	{
		int wonTeam = 0;
		packet >> wonTeam;
		std::cout << "\nThe winning team is: " << wonTeam << "\nMy team is: " << gameScreen.myTeam << '\n';
		sf::String finishMsg;
		gameScreen.endPanel.isActive = true;
		if (gameScreen.myTeam == wonTeam)
		{
			finishMsg = "You Won!\nCongrats!";
			gameScreen.endPanel.setPanelCol(FOREST_GREEN);
		}
		else
		{
			finishMsg = "Game over!\n Noob...";
			gameScreen.endPanel.setPanelCol(DARK_GRAY);
		}
		gameScreen.endPanel.setText(finishMsg);
	}

}

sf::RectangleShape makeNewButtonElement(sf::Vector2f& dims, sf::Color& fillCol, sf::Vector2u& winSize, int order)
{
	sf::RectangleShape button(dims);
	button.setFillColor(fillCol);
	button.setPosition((winSize.x - button.getSize().x) / 2, (winSize.y + 10) - (5 - order) * (10 + dims.y));
	return button;
}

sf::Text makeNewButtonTextElement(std::string text, sf::Font& font, int size, sf::Color fillCol, sf::RectangleShape& button)
{
	sf::Text buttonText(text, font, size);
	buttonText.setFillColor(fillCol);
	sf::FloatRect textBounds = buttonText.getLocalBounds();
	sf::FloatRect buttonBounds = button.getGlobalBounds();
	buttonText.setPosition(
		buttonBounds.left + (buttonBounds.width - textBounds.width) / 2.f - textBounds.left,
		buttonBounds.top + (buttonBounds.height - textBounds.height) / 2.f - textBounds.top
	);
	return buttonText;
}

MenuBtn makeBtnElem(sf::Vector2f& buttonSize, sf::Color& buttonColor, sf::Color& textColor,
	sf::Font& font, sf::Vector2u& windowSize, std::string& text, int index)
{
	sf::RectangleShape button = makeNewButtonElement(buttonSize, buttonColor, windowSize, index);
	sf::Text buttonText = makeNewButtonTextElement(text, font, 24, textColor, button);
	return { button, buttonText, index };
}

void initializeLobbyPanel(sf::Font& font, LobbyPanel& lobbyPanelLeft, LobbyPanel& lobbyPanelRight, sf::RenderWindow& window)
{
	int teamSize = lobbyPanelLeft.teamSize;
	if (teamSize < 2)
	{
		std::cerr << "team can't have less then 2 players";
		currentState = MENU;
	}
	sf::Vector2f slotDims{ 200.f,50.f };
	PlayerSlot masterSlotLeft(DEEP_BLUE, slotDims, DARK_UMBER, { 0.f,0.f }, font);
	int winWidth = window.getSize().x;
	int winHeight = window.getSize().y;
	float offsetLeft = 100;
	float offsetRight = winWidth - offsetLeft - slotDims.x;
	masterSlotLeft.shape.setPosition(offsetLeft, 100.f);
	PlayerSlot masterSlotRight(DEEP_RED, slotDims, DARK_UMBER, { 0.f,0.f }, font);
	masterSlotRight.shape.setPosition(offsetRight, 100.f);

	std::vector<PlayerSlot> team1Slots;
	std::vector<PlayerSlot> team2Slots;

	float startX = 100.f, startY = 200.f;
	float slotSpacing = 60.f;

	for (int i = 0; i < teamSize - 1; ++i)
	{
		// Team 1
		PlayerSlot slot1(DEF_BLUE, { 200.f, 50.f }, DARK_UMBER, { 0.f, 0.f }, font);
		slot1.shape.setPosition(startX, startY + i * slotSpacing);
		team1Slots.push_back(slot1);

		// Team 2
		PlayerSlot slot2(DEF_RED, { 200.f, 50.f }, DARK_UMBER, { 0.f, 0.f }, font);
		slot2.shape.setPosition(offsetRight, startY + i * slotSpacing);
		team2Slots.push_back(slot2);
	}


	lobbyPanelLeft = LobbyPanel(teamSize, masterSlotLeft, team1Slots);
	lobbyPanelLeft.teamName = { "Monkeys",font,24 };
	lobbyPanelLeft.teamName.setFillColor(DARK_UMBER);
	lobbyPanelLeft.updateTitlePos();

	lobbyPanelRight = LobbyPanel(teamSize, masterSlotRight, team2Slots);
	lobbyPanelRight.teamName = { "Horses", font, 24 };
	lobbyPanelRight.teamName.setFillColor(DARK_UMBER);
	lobbyPanelRight.updateTitlePos();

}


//states processing:

void menuHandling(std::vector<MenuBtn>& menuButtons, sf::Event event, InputBox& nick, sf::RenderWindow& window, sf::TcpSocket& socket,
	sf::IpAddress& serverIp, unsigned short serverPort)
{
	for (auto& elem : menuButtons)
	{
		if (elem.isHovering(window))
		{
			if (elem.isClicked(event, window))
			{
				if (elem.text.getString() == std::string("CREATE") && !isConnected)
				{
					currentState = LOBBYPREP;
				}
				else if (elem.text.getString() == std::string("JOIN"))
				{
					if (!isConnected)
					{
						currentState = JOININGPREP;
					}
				}
				else if (elem.text.getString() == std::string("SETTINGS"))
				{
					currentState = SETTINGS;
				}
				else if (elem.text.getString() == std::string("EXIT"))
				{
					window.close();
				}
			}
			elem.shape.setOutlineThickness(2.f);
			elem.shape.setOutlineColor(DARK_UMBER);
			elem.text.setFillColor(SANDSTONE);
			elem.text.setOutlineThickness(2.f);
			elem.text.setOutlineColor(DARK_UMBER);
		}
		else
		{
			elem.shape.setOutlineThickness(0.f);
			elem.text.setOutlineThickness(0.f);
			elem.text.setFillColor(DARK_UMBER);
		}
	}
}

void lobbyPrepHandling(InputBox& input, sf::Event event, InputBox& nick, sf::RenderWindow& window, LobbyPanel& lobbyPanelLeft, LobbyPanel& lobbyPanelRight)
{
	if (input.isClicked(event, window))
	{
		input.updateBoxColor(DRIFTEDWOOD);
		input.isWriting = true;
	}

	input.handleInput(event);

	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
	{
		if (input.isWriting)
		{
			input.updateBoxColor(input.inputBoxColor);
			input.isWriting = false;
		}
		else
		{
			currentState = MENU;
		}
	}

	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
	{
		std::string size = input.text.getString();
		if (!size.empty() && std::all_of(size.begin(), size.end(), ::isdigit))
		{
			lobbyPanelLeft.teamSize = stoi(size);
			lobbyPanelRight.teamSize = stoi(size);
			currentState = HOSTING;
		}
	}
}

void hostingHandling(sf::Event event, InputBox& nick, sf::RenderWindow& window, sf::TcpSocket& socket, sf::IpAddress& serverIp,
	unsigned short serverPort, LobbyPanel& lobbyLeft, LobbyPanel& lobbyRight, sf::Font& font)
{
	if (socket.connect(serverIp, serverPort) == sf::Socket::Done && !isConnected)
	{
		std::cout << "Connected to server" << std::endl;
		isConnected = true;
		isListening = true;
		if (!threadStarted)
		{
			listeningThread = std::thread([&]()
				{

					std::cout << "Thread is working!" << '\n';
					while (isListening)
					{
						sf::Packet line;
						auto status = socket.receive(line);
						if (status == sf::Socket::Done)
						{
							{
								std::lock_guard<std::mutex> lock(queueMutex);
								messageQueue.push(line);
								std::cout << "\n pushed new packet\n";
							}
						}
						else if (status == sf::Socket::Disconnected || currentState == MENU)
						{
							std::cout << "Disconnected from server." << '\n';
							isListening.store(false);
						}
						else if (status == sf::Socket::NotReady)
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(50));
						}
						else
						{
							std::cerr << "Error while receiving packet." << '\n';
						}
					}
				});
			listeningThread.detach();
			threadStarted = true;
		}
		std::cout << "Listening to server now\n";
		std::string name = nick.text.getString();

		if (sendWelcomeMsg(name, socket))
		{
			std::cout << "Name sent to server" << '\n';
			initializeLobbyPanel(font, lobbyLeft, lobbyRight, window);
			if (sendLobbyParams(lobbyLeft.teamSize, socket))
			{
				currentState = LOBBY;
			}
		}
	}
	else
	{
		std::cerr << "Failed to connect to server" << std::endl;
		sf::sleep(sf::milliseconds(5000));
		currentState = MENU;
	}
}

void joiningPrepHandling(InputBox& input, sf::Event event, sf::RenderWindow& window, sf::IpAddress& adress)
{
	if (input.isClicked(event, window))
	{
		input.updateBoxColor(DRIFTEDWOOD);
		input.isWriting = true;
	}

	input.handleInput(event);

	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
	{
		if (input.isWriting)
		{
			input.updateBoxColor(input.inputBoxColor);
			input.isWriting = false;
		}
		else
		{
			currentState = MENU;
		}
	}

	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
	{
		std::string ipInp = input.text.getString();
		if (!ipInp.empty())
		{
			adress = ipInp;
			currentState = JOINING;
		}
	}
}

void joiningHandling(sf::Event event, InputBox& nick, sf::RenderWindow& window, sf::TcpSocket& socket, sf::IpAddress& serverIp,
	unsigned short serverPort, LobbyPanel& lobbyLeft, LobbyPanel& lobbyRight, sf::Font& font, GameMngr& gameScreen)
{
	if (socket.connect(serverIp, serverPort) == sf::Socket::Done && !isConnected)
	{
		std::cout << "Connected to server" << std::endl;
		isConnected = true;
		isListening = true;
		if (!threadStarted)
		{
			listeningThread = std::thread([&]()
				{
					sf::Packet line;
					std::cout << "Thread is working!" << '\n';
					while (isListening)
					{
						auto status = socket.receive(line);
						if (status == sf::Socket::Done)
						{
							{
								std::lock_guard<std::mutex> lock(queueMutex);
								messageQueue.push(line);
							}
						}
						else if (status == sf::Socket::Disconnected || currentState == MENU)
						{
							std::cout << "Disconnected from server." << '\n';
							isListening.store(false);
						}
						else if (status == sf::Socket::NotReady)
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(50));
						}
						else
						{
							std::cerr << "Error while receiving packet." << '\n';
						}
					}
				});
			listeningThread.detach();
			threadStarted = true;
		}
		std::cout << "Listening to server now\n";
		std::string name = nick.text.getString();

		if (sendWelcomeMsg(name, socket))
		{
			std::cout << "Name sent to server" << '\n';
			sf::Packet pack;
			sf::sleep(sf::milliseconds(20));
			if (!messageQueue.empty())
			{
				pack = messageQueue.back();
				messageQueue.pop();
			}
			processResponse(pack, lobbyLeft, lobbyRight, gameScreen, socket);
			initializeLobbyPanel(font, lobbyLeft, lobbyRight, window);
			currentState = LOBBY;
		}
	}
	else
	{
		std::cerr << "Failed to connect to server" << std::endl;
		currentState = MENU;
	}
}

void lobbyHandling(LobbyPanel& lobbyLeft, LobbyPanel& lobbyRight, GameMngr& gameScreen, sf::Event event, sf::RenderWindow& window, sf::TcpSocket& socket)
{

	if (lobbyLeft.masterSlot.isClicked(event, window) && !lobbyLeft.masterSlot.isOccupied)
	{
		if (sendChosenSlot(0, 0, socket))
		{
			gameScreen.myTeam = 0;
			std::cout << "u r left team master now" << '\n';
		}
	}

	if (lobbyRight.masterSlot.isClicked(event, window) && !lobbyRight.masterSlot.isOccupied)
	{

		if (sendChosenSlot(1, 0, socket))
		{
			gameScreen.myTeam = 1;
			std::cout << "u r right team master now" << '\n';
		}
	}

	for (int i = 0; i <= lobbyLeft.teamSize - 2; ++i)
	{
		if (lobbyLeft.playerSlots[i].isClicked(event, window) && !lobbyLeft.playerSlots[i].isOccupied)
		{
			if (sendChosenSlot(0, i + 1, socket))
			{
				gameScreen.myTeam = 0;
				std::cout << "slot " << i + 1 << " of left was chosen.\n" << "Waiting server to confirm...\n";
			}
		}

		if (lobbyRight.playerSlots[i].isClicked(event, window) && !lobbyRight.playerSlots[i].isOccupied)
		{

			if (sendChosenSlot(1, i + 1, socket))
			{
				gameScreen.myTeam = 1;
				std::cout << "slot " << i + 1 << " of right was chosen.\n" << "Waiting server to confirm...\n";
			}
		}
	}



	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F)
	{
		if (sendFreeSlot(socket))
		{
			std::cout << "Your current slot was freed.\n";
		}
	}

	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::S)
	{
		if (sendPreStartGame(socket))
		{
			std::cout << "Waiting server to send board...\n";
		}
	}

	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::L)
	{
		if (sendPreStartGame(socket))
		{
			currentState = PLAYING;
		}
	}

}

void playingHandling(sf::TcpSocket& socket, sf::RenderWindow& window, sf::Event event, GameMngr& gameScreen)
{
	for (auto& elem : gameScreen.boardPanel.board)
	{
		for (auto& card : elem)
		{
			if (card.flags.isHoveringRed(window))
			{
				card.flags.redFlag.setOutlineColor({ 229,255,0 });
			}
			else
			{
				card.wordCard.getFillColor().toInteger() == 255 ? card.flags.redFlag.setOutlineColor(WHITECOL)
					: card.flags.redFlag.setOutlineColor(BLACKCOL);
			}

			if (card.flags.isHoveringGreen(window))
			{
				card.flags.greenFlag.setOutlineColor({ 229,255,0 });
			}
			else
			{
				card.wordCard.getFillColor().toInteger() == 255 ? card.flags.greenFlag.setOutlineColor(WHITECOL)
					: card.flags.greenFlag.setOutlineColor(BLACKCOL);
			}

			if (card.isHovering(window))
			{
				card.wordCard.getFillColor().toInteger() == 255 ? card.wordCard.setOutlineColor(BRIGHT_RED) : card.wordCard.setOutlineColor(BLACKCOL);
				card.wordCard.setOutlineThickness(2.5f);
			}
			else
			{
				card.wordCard.setOutlineThickness(0.f);
			}

			//clicking processing:

			if (card.isClicked(event, window))
			{
				//send card click to server:
				if (!sendCardClick(card.order, socket))
				{
					std::cerr << "Failed to send card click to server\n";
				}
			}

			if (card.flags.isClickedGreen(event, window))
			{
				//send green flag click to server:
				if (!sendFlagClick(card.order, 0, socket))
				{
					std::cerr << "Failed to send green flag click to server\n";
				}
			}
			if (card.flags.isClickedRed(event, window))
			{
				//send red flag click to server:
				if (!sendFlagClick(card.order, 0, socket))
				{
					std::cerr << "Failed to send red flag click to server\n";
				}
			}

		}
	}

	//left (0) team proccessing:
	if (gameScreen.leftPanel.isActive)
	{
		gameScreen.leftPanel.teamShape.setOutlineThickness(2.0f);
		gameScreen.leftPanel.teamShape.setOutlineColor({ 229,255,0 });
	}
	else
	{
		gameScreen.leftPanel.teamShape.setOutlineThickness(0.0f);
	}

	if (gameScreen.myTeam == 0 && gameScreen.leftPanel.isClickedInput(event, window) && gameScreen.leftPanel.isActive)
	{
		std::cout << "writing in blue chat...\n";
		gameScreen.leftPanel.isWriting = true;
	}
	else if (gameScreen.leftPanel.isClickedInput(event, window))
	{

		std::cout << "team: " << gameScreen.myTeam << "\n team panel active: " << gameScreen.leftPanel.isActive << "\n";
	}

	if (gameScreen.leftPanel.isActive && gameScreen.leftPanel.isWriting)
	{
		gameScreen.leftPanel.handleUtf8Input(event);
		if (gameScreen.leftPanel.isClickedHint(event, window))
		{
			gameScreen.leftPanel.isWriting = false;
			gameScreen.leftPanel.curMsg.setPosition(gameScreen.leftPanel.inputShape.getPosition().x + 5,
				gameScreen.leftPanel.inputShape.getPosition().y + 5);
			if (sendChatMsg(gameScreen.leftPanel.mySocket, gameScreen.leftPanel.curMsg))
			{
				gameScreen.leftPanel.chatMsgs.push_back(gameScreen.leftPanel.curMsg);
				if (gameScreen.leftPanel.chatMsgs.size() > 3)
				{
					gameScreen.leftPanel.hintInd++;
				}
				gameScreen.leftPanel.curMsg.setString("");
				gameScreen.leftPanel.isWriting = false;
			}
		}
	}

	if (gameScreen.leftPanel.isHoveringChat(window))
	{
		gameScreen.leftPanel.chatShape.setOutlineThickness(2.f);
		gameScreen.leftPanel.chatShape.setOutlineColor(BLACKCOL);
		if (event.type == sf::Event::MouseWheelScrolled)
		{
			if (!gameScreen.leftPanel.chatMsgs.empty())
			{
				int maxHintInd = std::max(0, static_cast<int>(gameScreen.leftPanel.chatMsgs.size()) - 3);

				if (event.mouseWheelScroll.delta > 0 && gameScreen.leftPanel.hintInd > 0)
				{
					gameScreen.leftPanel.hintInd--;
				}
				else if (event.mouseWheelScroll.delta < 0 && gameScreen.leftPanel.hintInd < maxHintInd)
				{
					gameScreen.leftPanel.hintInd++;
				}

				gameScreen.leftPanel.hintInd = std::clamp(gameScreen.leftPanel.hintInd, 0, maxHintInd);
			}
		}
	}
	else
	{
		gameScreen.leftPanel.chatShape.setOutlineThickness(0.f);
	}

	if (gameScreen.leftPanel.isHoveringInput(window))
	{
		gameScreen.leftPanel.inputShape.setOutlineThickness(2.f);
		gameScreen.leftPanel.inputShape.setOutlineColor(BLACKCOL);
	}
	else
	{
		gameScreen.leftPanel.inputShape.setOutlineThickness(0.f);
	}

	if (gameScreen.leftPanel.isHoveringHint(window))
	{
		gameScreen.leftPanel.hintButton.setOutlineThickness(2.f);
		gameScreen.leftPanel.hintButton.setOutlineColor(BLACKCOL);
	}
	else
	{
		gameScreen.leftPanel.hintButton.setOutlineThickness(0.f);
	}

	//right (1) team processing

	if (gameScreen.rightPanel.isActive)
	{
		gameScreen.rightPanel.teamShape.setOutlineThickness(2.0f);
		gameScreen.rightPanel.teamShape.setOutlineColor({ 229,255,0 });
	}
	else
	{
		gameScreen.rightPanel.teamShape.setOutlineThickness(0.0f);
	}

	if (gameScreen.myTeam == 1 && gameScreen.rightPanel.isClickedInput(event, window) && gameScreen.rightPanel.isActive)
	{
		std::cout << "writing in red chat...\n";
		gameScreen.rightPanel.isWriting = true;
	}
	else if (gameScreen.rightPanel.isClickedInput(event, window))
	{

		std::cout << "team: " << gameScreen.myTeam << "\n team panel active: " << gameScreen.rightPanel.isActive << "\n";
	}

	if (gameScreen.rightPanel.isActive && gameScreen.rightPanel.isWriting)
	{
		gameScreen.rightPanel.handleUtf8Input(event);
		if (gameScreen.rightPanel.isClickedHint(event, window))
		{
			gameScreen.rightPanel.isWriting = false;
			gameScreen.rightPanel.curMsg.setPosition(gameScreen.rightPanel.inputShape.getPosition().x + 5,
				gameScreen.rightPanel.inputShape.getPosition().y + 5);
			if (sendChatMsg(gameScreen.rightPanel.mySocket, gameScreen.rightPanel.curMsg))
			{
				gameScreen.rightPanel.chatMsgs.push_back(gameScreen.rightPanel.curMsg);
				if (gameScreen.rightPanel.chatMsgs.size() > 3)
				{
					gameScreen.rightPanel.hintInd++;
				}
				gameScreen.rightPanel.curMsg.setString("");
				gameScreen.rightPanel.isWriting = false;
			}
		}
	}

	if (gameScreen.rightPanel.isHoveringChat(window))
	{
		gameScreen.rightPanel.chatShape.setOutlineThickness(2.f);
		gameScreen.rightPanel.chatShape.setOutlineColor(BLACKCOL);
		if (event.type == sf::Event::MouseWheelScrolled)
		{
			if (!gameScreen.rightPanel.chatMsgs.empty())
			{
				int maxHintInd = std::max(0, static_cast<int>(gameScreen.rightPanel.chatMsgs.size()) - 3);

				if (event.mouseWheelScroll.delta > 0 && gameScreen.rightPanel.hintInd > 0)
				{
					gameScreen.rightPanel.hintInd--;
				}
				else if (event.mouseWheelScroll.delta < 0 && gameScreen.rightPanel.hintInd < maxHintInd)
				{
					gameScreen.rightPanel.hintInd++;
				}

				gameScreen.rightPanel.hintInd = std::clamp(gameScreen.rightPanel.hintInd, 0, maxHintInd);
			}
		}
	}
	else
	{
		gameScreen.rightPanel.chatShape.setOutlineThickness(0.f);
	}

	if (gameScreen.rightPanel.isHoveringInput(window))
	{
		gameScreen.rightPanel.inputShape.setOutlineThickness(2.f);
		gameScreen.rightPanel.inputShape.setOutlineColor(BLACKCOL);
	}
	else
	{
		gameScreen.rightPanel.inputShape.setOutlineThickness(0.f);
	}

	if (gameScreen.rightPanel.isHoveringHint(window))
	{
		gameScreen.rightPanel.hintButton.setOutlineThickness(2.f);
		gameScreen.rightPanel.hintButton.setOutlineColor(BLACKCOL);
	}
	else
	{
		gameScreen.rightPanel.hintButton.setOutlineThickness(0.f);
	}

}

void settingsHandling(InputBox& inpBox, sf::Event event, sf::RenderWindow& window)
{
	if (inpBox.isClicked(event, window))
	{
		inpBox.updateBoxColor(DRIFTEDWOOD);
		inpBox.isWriting = true;
	}

	inpBox.handleInput(event);

	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
	{
		if (inpBox.isWriting)
		{
			inpBox.updateBoxColor(inpBox.inputBoxColor);
			inpBox.isWriting = false;
		}
		else
		{
			currentState = MENU;
		}
	}

	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
	{
		if (inpBox.isWriting)
		{
			inpBox.updateBoxColor(inpBox.inputBoxColor);
			inpBox.isWriting = false;
		}
	}
}

void handleServerMessages(sf::TcpSocket& socket, LobbyPanel& lobbyLeft, LobbyPanel& lobbyRight, GameMngr& gameScreen)
{
	std::lock_guard<std::mutex> lock(queueMutex);
	while (!messageQueue.empty())
	{
		sf::Packet packet = std::move(messageQueue.front());
		messageQueue.pop();
		processResponse(packet, lobbyLeft, lobbyRight, gameScreen, socket);
	}
}

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	setlocale(LC_ALL, "Russian");
	sf::RenderWindow window(sf::VideoMode(860, 540), "CodeNames");
	window.setKeyRepeatEnabled(false);

	window.setFramerateLimit(144);

	if (!font.loadFromFile("cyr.OTF"))
	{
		std::cerr << "Failed to load font\n";
		return -1;
	}
	font.setSmooth(false);

	//networking staff:

	sf::TcpSocket socket;

	sf::IpAddress serverIp = "127.0.0.1";
	unsigned short serverPort = 54000;

	//menu buttons:

	std::vector<MenuBtn> menuButtons;
	std::vector<std::string> menuBtnNames{ "CREATE", "JOIN", "SETTINGS", "EXIT" };

	sf::Vector2u windowSize = window.getSize();

	sf::Vector2f buttonSize(350.f, 80.f);

	for (int i = 0; i < menuBtnNames.size(); ++i)
	{
		menuButtons.push_back(makeBtnElem(buttonSize, SANDSTONE, DARK_UMBER, font, windowSize, menuBtnNames[i], i));
	}

	//settings page:

	InputBox inpBox;
	inpBox.labelText = sf::Text("NAME", font, 24);
	inpBox.text = sf::Text("Player", font, 24);
	inpBox.setInpBoxParams(100, 200, 200, 400, 80, font);

	//joiningPrep page:

	InputBox joiningPrepInp;
	joiningPrepInp.labelText = sf::Text("IP : ", font, 24);
	joiningPrepInp.text = sf::Text("", font, 24);
	joiningPrepInp.setInpBoxParams(100, 200, 200, 400, 80, font);

	//lobbyPrep page:

	InputBox lobbyPrepInp;
	lobbyPrepInp.labelText = sf::Text("Team Size: ", font, 24);
	lobbyPrepInp.text = sf::Text("2", font, 24);
	lobbyPrepInp.setInpBoxParams(100, 200, 200, 400, 80, font);

	//lobby:
	LobbyPanel lobbyPanelLeft;
	LobbyPanel lobbyPanelRight;

	BoardPanel boardPanel(boardOfWords);
	TeamPanel blueTeam{ socket, lobbyPanelLeft.masterSlot, lobbyPanelLeft.playerSlots };
	TeamPanel redTeam{ socket, lobbyPanelRight.masterSlot, lobbyPanelRight.playerSlots };
	EndGamePanel finishScreen(font);
	GameMngr game{ boardPanel, blueTeam, redTeam, finishScreen };

	//play page:

	blueTeam.setUpPanel({ 5.f,5.f }, { .0f,.0f }, { 200.f,530.f }, DEF_BLUE);

	blueTeam.seUpChat({ .0f,.0f }, { 190.f, 85.f }, DEEP_BLUE);
	blueTeam.setUpHintButton(DEF_BLUE);
	blueTeam.setUpInputBox(DEF_BLUE);

	boardPanel.boardShape.setOrigin(.0f, .0f);
	boardPanel.boardShape.setSize({ 440,440 });
	boardPanel.boardShape.setFillColor(PALE_LEMON);
	boardPanel.boardShape.setPosition(blueTeam.teamShape.getPosition().x + blueTeam.teamShape.getSize().x + 5.f, 5.f);

	redTeam.teamShape.setOrigin(.0f, .0f);
	redTeam.teamShape.setSize({ 200,530 });
	redTeam.teamShape.setFillColor(DEF_RED);
	redTeam.teamShape.setPosition(window.getSize().x - 5.f - redTeam.teamShape.getSize().x, 5.f);

	redTeam.seUpChat({ .0f,.0f }, { 190.f, 85.f }, DEEP_RED);
	redTeam.setUpHintButton(DEF_RED);
	redTeam.setUpInputBox(DEF_RED);


	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				isListening.store(false);
				if (listeningThread.joinable())
				{
					listeningThread.join();
				}
				window.close();
			}

			//states processing:

			if (currentState.load() == MENU)
			{
				menuHandling(menuButtons, event, inpBox, window, socket, serverIp, serverPort);
			}

			if (currentState.load() == HOSTING)
			{
				hostingHandling(event, inpBox, window, socket, serverIp, serverPort, lobbyPanelLeft, lobbyPanelRight, font);
			}

			if (currentState.load() == JOININGPREP)
			{
				joiningPrepHandling(joiningPrepInp, event, window, serverIp);
			}
			if (currentState.load() == JOINING)
			{
				joiningHandling(event, inpBox, window, socket, serverIp, serverPort, lobbyPanelLeft, lobbyPanelRight, font, game);
			}

			if (currentState.load() == LOBBYPREP)
			{
				lobbyPrepHandling(lobbyPrepInp, event, inpBox, window, lobbyPanelLeft, lobbyPanelRight);
			}

			if (currentState.load() == LOBBY)
			{
				lobbyHandling(lobbyPanelLeft, lobbyPanelRight, game, event, window, socket);

			}

			if (currentState.load() == SETTINGS)
			{
				settingsHandling(inpBox, event, window);
			}

			if (currentState.load() == PLAYING)
			{
				playingHandling(socket, window, event, game);
			}

		}

		handleServerMessages(socket, lobbyPanelLeft, lobbyPanelRight, game);

		currentState.load() == PLAYING ? window.clear(DARK_GRAY) : window.clear(PALE_LEMON);

		if (currentState.load() == MENU)
		{
			for (const auto& elem : menuButtons)
			{
				window.draw(elem.shape);
				window.draw(elem.text);
			}
		}
		if (currentState.load() == JOININGPREP)
		{
			joiningPrepInp.draw(window);
		}
		if (currentState.load() == LOBBYPREP)
		{
			lobbyPrepInp.draw(window);
		}

		if (currentState.load() == HOSTING)
		{
			sf::Text statusText(isConnected ? "Connected to Server" : "Connecting...", font, 20);
			statusText.setFillColor(isConnected ? sf::Color::Green : sf::Color::Red);
			statusText.setPosition(10.f, 10.f);
			window.draw(statusText);
		}

		if (currentState.load() == SETTINGS)
		{
			inpBox.draw(window);
		}

		if (currentState.load() == LOBBY)
		{
			lobbyPanelLeft.draw(window);
			lobbyPanelRight.draw(window);
		}

		if (currentState.load() == PLAYING)
		{
			game.draw(window);
		}
		window.display();
	}

	isListening = false;
	if (listeningThread.joinable())
	{
		listeningThread.join();
	}

	socket.disconnect();
	return 0;
}
