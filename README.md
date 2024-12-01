## Chess Game in C++
This is a simple chess game developed in C++ that provides an intuitive drag-and-drop interface and supports both Player vs. Player (PvP) and Player vs. Engine (PvE) modes.

### How to Run
Ensure you have the SFML library installed on your system.

Navigate to the code folder.

Compile and run the main.cpp file. 
### Features
Gameplay: Drag-and-Drop

Chess Rules:
- Pawn Promotion: Promotes pawns upon reaching the 8th rank.
- Castling: Allows switching the king and rook under the standard rules.
- Other basic chess rules implemented.

Game Modes:
- PvP Mode: Play with another player locally.
- PvE Mode: Play with an bot.
### Code Logic
- Framework: Built using the SFML (Simple and Fast Multimedia Library) for graphics and event handling.
- AI Integration: Utilizes the Stockfish chess engine for bot gameplay in PvE mode.
