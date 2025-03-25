# How to build

Clone the repo, then open terminal and make a build folder:  
```sh
mkdir build
cd build 
```
Configure cmake and run build:

```sh
cmake .. 
cmake --build . --config Release   
```
# The idea behind the project

This is an implementation of the classic board game Codenames in C++.
I wanted to learn network programming, and I realized that developing a multiplayer game would be a great way to start.
Since I had no prior experience with networking, I decided to use SFML, a familiar graphics library that conveniently includes networking modules.  The server uses `sf::SocketSelector` to manage clients and sf::Packets to handle messages from clients and sending game state updates to them. There is a words collection , and 24 words are chosen randomly and get sent to each client. 

## A gameplay demo:
<img src="demo/coolgp.gif" alt="The minimal gameplay" width="50%">

# Future Plans

The game is already playable, but there are several features I'd like to add:  

- **UI Improvements**:
  - Player nickname display in team panels
  - Word flagging system for team collaboration
  - Turn timer visualization

- **Networking**:
  - Connection recovery
  - Server-side turn timer sync

- **Gameplay**:
  - Custom word packs support
  - Games history with some statistics
  - Some music to the background

P.S. at this moment there's no settings available exept the nickname, I'll add some later