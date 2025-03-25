# How to build

Clone the repo, then open terminal and make a build folder:  
`mkdir build`  
Move to it:  
`cd build`  
Run cmake:  
`cmake ..`  
Run cmake build:  
`cmake --build . --config Release`   

# The idea behind the project

This is a classic boardgame - codenames - implementation in c++. I was going to learn some network programing and I have come to conclusion that making some multiplayer game is a perfect way to start. Since I knew nothing on networking, I decided to use a famillar graphic library sfml which fortunatly has a networking modules too.  

### A minimal gameplay shown here:
<img src="demo/coolgp.gif" alt="The minimal gameplay" width="50%">

# Future

Well it's already kinda playable, but there are some features I'd like to add in the future. The first thing is to show players nickname in the panels of teams , also I have made a timer on the server side, but still did not use it.