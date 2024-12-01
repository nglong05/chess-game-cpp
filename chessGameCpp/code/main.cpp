#include <SFML/Graphics.hpp>
#include <windows.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <cmath>

using namespace sf;
using namespace std;

int squareSize = 56; // pixel ô cờ
Vector2f offset(28, 28);
bool isBotTurn = false;
vector<bool> pawnPromoted(32, false);
Sprite f[32]; 

string position = "";

// Stockfish process handles
STARTUPINFOA sti = { 0 };
PROCESS_INFORMATION pi = { 0 };
HANDLE pipin_w, pipin_r, pipout_w, pipout_r;

// chuyển vị trí quân cờ từ dạng vector2f sang kí hiệu cờ vua đại số (algebraic notation)
string toChessNote(Vector2f p)
{
    string s = "";
    s += char(p.x / squareSize + 'a');              // Column
    s += char(8 - p.y / squareSize + '0');            // Row
    return s;
}

// chuyển từ algebraic notation sang vị trí quân cờ dạng vector2f
Vector2f toCoord(char a, char b)
{
    int x = int(a) - 'a';                 // Column
    int y = 8 - int(b) + '0';                 // Row
    return Vector2f(x * squareSize, y * squareSize);
}

void moveCastling(string str)
{
    Vector2f oldPos = toCoord(str[0], str[1]); 
    Vector2f newPos = toCoord(str[2], str[3]); 

    for (int i = 0; i < 32; i++)
        if (f[i].getPosition() == newPos)
            f[i].setPosition(-100, -100);

    for (int i = 0; i < 32; i++)
        if (f[i].getPosition() == oldPos)
            f[i].setPosition(newPos);
}

IntRect whitePawnRect(squareSize * 5, squareSize * 1, squareSize, squareSize);
IntRect blackPawnRect(squareSize * 5, squareSize * 0, squareSize, squareSize); 
IntRect whiteKingRect(squareSize * 4, squareSize * 1, squareSize, squareSize); 
IntRect blackKingRect(squareSize * 4, squareSize * 0, squareSize, squareSize); 

bool isWhitePawn(Sprite& piece) {
    IntRect textureRect = piece.getTextureRect();
    return (textureRect == whitePawnRect);
}

bool isBlackPawn(Sprite& piece) {
    IntRect textureRect = piece.getTextureRect();
    return (textureRect == blackPawnRect);
}

bool isWhiteKing(Sprite& piece) {
    IntRect textureRect = piece.getTextureRect();
    return (textureRect == whiteKingRect);
}

bool isBlackKing(Sprite& piece) {
    IntRect textureRect = piece.getTextureRect();
    return (textureRect == blackKingRect);
}

bool displayWon(int a) {
    RenderWindow window(VideoMode(300, 100), "Result");
    window.clear(Color::White);

    // Load the image as a texture
    Texture texture;
    if (a) {
        texture.loadFromFile("code/images/bwon.png");  
    } 
    else  {
        texture.loadFromFile("code/images/wwon.png");
    }

    // Create a sprite to display the texture
    Sprite sprite;
    sprite.setTexture(texture);

    // Set the size and position of the sprite to center it
    sprite.setScale(300.0f / texture.getSize().x, 100.0f / texture.getSize().y); // Scale to 300x100
    sprite.setPosition(
        (window.getSize().x - sprite.getGlobalBounds().width) / 2.0f,
        (window.getSize().y - sprite.getGlobalBounds().height) / 2.0f
    );

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
                return true; // Return to menu when window closes
            }
        }

        window.clear(Color::Black);
        window.draw(sprite);
        window.display();
    }

    return false;
}

void promotePawn(int i, bool isWhite) {
    RenderWindow promoteWindow(VideoMode(230, 100), "Promote Pawn");
    Texture promoteTexture;
    promoteTexture.loadFromFile("code/images/figures.png");

    // Cắt ảnh cho 4 quân cờ có thể được phong
    Sprite rook(promoteTexture, IntRect(0, isWhite ? 0 : squareSize, squareSize, squareSize));
    Sprite knight(promoteTexture, IntRect(squareSize, isWhite ? 0 : squareSize, squareSize, squareSize));
    Sprite bishop(promoteTexture, IntRect(squareSize * 2, isWhite ? 0 : squareSize, squareSize, squareSize));
    Sprite queen(promoteTexture, IntRect(squareSize * 3, isWhite ? 0 : squareSize, squareSize, squareSize));

    // Đặt vị trí các quân cờ trong cửa sổ
    rook.setPosition(10, 20);
    knight.setPosition(60, 20);
    bishop.setPosition(110, 20);
    queen.setPosition(160, 20);

    while (promoteWindow.isOpen()) {
        Event event;
        while (promoteWindow.pollEvent(event)) {
            if (event.type == Event::Closed)
                promoteWindow.close();
            else if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                Vector2f mousePos = promoteWindow.mapPixelToCoords(Mouse::getPosition(promoteWindow));

                // Kiểm tra lựa chọn quân cờ
                if (rook.getGlobalBounds().contains(mousePos)) {
                    f[i].setTextureRect(IntRect(0, isWhite ? 0 : squareSize, squareSize, squareSize));
                    promoteWindow.close();
                }
                else if (knight.getGlobalBounds().contains(mousePos)) {
                    f[i].setTextureRect(IntRect(squareSize, isWhite ? 0 : squareSize, squareSize, squareSize));
                    promoteWindow.close();
                }
                else if (bishop.getGlobalBounds().contains(mousePos)) {
                    f[i].setTextureRect(IntRect(squareSize * 2, isWhite ? 0 : squareSize, squareSize, squareSize));
                    promoteWindow.close();
                }
                else if (queen.getGlobalBounds().contains(mousePos)) {
                    f[i].setTextureRect(IntRect(squareSize * 3, isWhite ? 0 : squareSize, squareSize, squareSize));
                    promoteWindow.close();
                }
            }
        }

        promoteWindow.clear(Color::White);
        promoteWindow.draw(rook);
        promoteWindow.draw(knight);
        promoteWindow.draw(bishop);
        promoteWindow.draw(queen);
        promoteWindow.display();
    }
}

void move(string str)
{
    Vector2f oldPos = toCoord(str[0], str[1]); // gán giá trị cũ của quân cờ trước khi bị di chuyển vào oldPos
    Vector2f newPos = toCoord(str[2], str[3]); // gán giá trị mới của quân cờ sau khi bị di chuyển vào newPos

    // xóa hình ảnh của quân cờ bị ăn bằng cách đưa nó ra vị trí -100 -100 (ẩn quân cờ khỏi window)
    for (int i = 0; i < 32; i++)
        if (f[i].getPosition() == newPos) {
            f[i].setPosition(-100, -100);
            
            if (isWhiteKing(f[i])) {
                displayWon(1);
            }

            else if (isBlackKing(f[i])) {
                displayWon(0);
            }
        }

    // dịch chuyển vị trí của quân cờ
    for (int i = 0; i < 32; i++) {
        if (f[i].getPosition() == oldPos) {
            f[i].setPosition(newPos);
        }
    }

    for (int i = 0; i < 32; i++) {
        int row = static_cast<int>(round(f[i].getPosition().y / squareSize));
        if (isWhitePawn(f[i]) && row == 0 && !pawnPromoted[i]) {
            promotePawn(i, false);
            pawnPromoted[i] = true; 
        }
        else if (isBlackPawn(f[i]) && row == 7 && !pawnPromoted[i]) {
            promotePawn(i, true);
            pawnPromoted[i] = true; 
        }
    }

    // Handle castling
    if (str == "e1g1" && position.find("e1") == -1) moveCastling("h1f1");
    if (str == "e8g8" && position.find("e8") == -1) moveCastling("h8f8");
    if (str == "e1c1" && position.find("e1") == -1) moveCastling("a1d1");
    if (str == "e8c8" && position.find("e8") == -1) moveCastling("a8d8");

}

int board[8][8] =
{ -1,-2,-3,-4,-5,-3,-2,-1,
 -6,-6,-6,-6,-6,-6,-6,-6,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  6, 6, 6, 6, 6, 6, 6, 6,
  1, 2, 3, 4, 5, 3, 2, 1 };
 

void loadPosition()
{
    int k = 0;               //stores the Sprite objects for each chess piece.
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
            int n = board[i][j];
            if (!n) continue;
            // nếu board = 0, nghĩa là vị trí trống không có quân cờ
            int x = abs(n) - 1;
            // gán loại quân cờ cho từng vị trí (ví dụ ở vị trí 1 là quân xe, 2 là quân mã)
            int y = n > 0 ? 1 : 0;
            // gán quân cờ trắng đen, nếu mang giá trị dương thì màu trắng, và ngược lại
            f[k].setTextureRect(IntRect(squareSize * x, squareSize * y, squareSize, squareSize));
            // hàm setTextureRect chọn từng phần hình ảnh trong t1 (hình ảnh các quân cờ)
            // hàm IntRect sau đó tạo ra một hình vuông, lấy phần ảnh quân cờ thích hợp dựa trên x và y cho từng quân cờ
            f[k].setPosition(squareSize * j, squareSize * i);
            // đặt hình ảnh quân cờ vào vị trí đúng trên bàn cờ
            k++;
        }

    // Apply moves in position string
    for (int i = 0; i < position.length(); i += 5)
        move(position.substr(i, 4));
}

void ConnectToEngine(const char* path) {
    SECURITY_ATTRIBUTES sats = { sizeof(sats), NULL, TRUE };
    CreatePipe(&pipout_r, &pipout_w, &sats, 0);
    CreatePipe(&pipin_r, &pipin_w, &sats, 0);

    sti.cb = sizeof(sti);
    sti.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    sti.wShowWindow = SW_HIDE;
    sti.hStdInput = pipin_r;
    sti.hStdOutput = pipout_w;
    sti.hStdError = pipout_w;

    CreateProcessA(path, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &sti, &pi);
}

string getNextMove(const string& position) {
    string command = "position startpos moves " + position + "\ngo\n";
    DWORD writ, read, available;
    BYTE buffer[2048];
    string output;

    WriteFile(pipin_w, command.c_str(), command.length(), &writ, NULL);

    while (true) {
        Sleep(100);  //delay to allow Stockfish to process

        PeekNamedPipe(pipout_r, NULL, 0, NULL, &available, NULL);
        if (available > 0) {
            ZeroMemory(buffer, sizeof(buffer));
            ReadFile(pipout_r, buffer, sizeof(buffer) - 1, &read, NULL);
            buffer[read] = 0;
            output += (char*)buffer;

            size_t n = output.find("bestmove");
            if (n != string::npos) {
                return output.substr(n + 9, 4);
            }
        }
    }
    return "error";
}

void CloseConnection() {
    WriteFile(pipin_w, "quit\n", 5, NULL, NULL);
    CloseHandle(pipin_w);
    CloseHandle(pipin_r);
    CloseHandle(pipout_w);
    CloseHandle(pipout_r);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

enum MenuOptions {PvE, PvP, Quit, None};
MenuOptions showMenu(RenderWindow& window) {
    Texture backgroundTexture, pveTexture, pvpTexture, quitTexture;
    backgroundTexture.loadFromFile("code/images/menu.png");
    pveTexture.loadFromFile("code/images/PvE.png");
    pvpTexture.loadFromFile("code/images/PvP.png");
    quitTexture.loadFromFile("code/images/quit.png");
    Sprite background(backgroundTexture);
    Sprite pveButton(pveTexture), pvpButton(pvpTexture), quitButton(quitTexture);

    // Position each button sprite on the screen
    pveButton.setPosition(100.f, 70.f);
    pvpButton.setPosition(100.f, 200.f);
    quitButton.setPosition(100.f, 330.f);

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) window.close();
            if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
                if (pveButton.getGlobalBounds().contains(mousePos)) return PvE;
                if (pvpButton.getGlobalBounds().contains(mousePos)) return PvP;
                if (quitButton.getGlobalBounds().contains(mousePos)) return Quit;
            }
        }
        window.clear();
        window.draw(background);
        window.draw(pveButton);
        window.draw(pvpButton);
        window.draw(quitButton);
        window.display();
    }
    return None;
}

int main()
{
    RenderWindow window(VideoMode(504, 504), "Chess Menu");
    MenuOptions choice = showMenu(window);

    if (choice != Quit) {
        RenderWindow window(VideoMode(504, 504), "Chess");
        if (choice == PvE) ConnectToEngine("code/stockfish.exe");

        Texture t1, t2;
        t1.loadFromFile("code/images/figures.png");
        t2.loadFromFile("code/images/board.png");

        for (int i = 0; i < 32; i++) f[i].setTexture(t1);
        Sprite sBoard(t2);

        loadPosition();

        bool isMove = false;
        float dx = 0, dy = 0;
        Vector2f oldPos, newPos;
        string str;
        int n = 0;

        while (window.isOpen()) {
            Vector2i pos = Mouse::getPosition(window) - Vector2i(offset);
            Event e;

            while (window.pollEvent(e)) {
                // tắt cửa sổ
                if (e.type == Event::Closed) window.close();
                // backspace để đi lại 1 bước
                if (e.type == Event::KeyPressed && e.key.code == Keyboard::BackSpace) {
                    if (position.length() > 5) 
                        position.erase(position.length() - 6, 5);
                    loadPosition(); 
                }
                // nhấp chuột
                if (e.type == Event::MouseButtonPressed && e.mouseButton.button == Mouse::Left)
                    for (int i = 0; i < 32; i++)
                        if (f[i].getGlobalBounds().contains(pos.x, pos.y)) {
                            isMove = true;
                            n = i;
                            // n mang giá trị của quân cờ được chọn trong 32 quân cờ
                            dx = pos.x - f[i].getPosition().x;
                            dy = pos.y - f[i].getPosition().y;
                            // vị trí của hỉnh ảnh quân cờ, sao cho khi drag quân cờ 
                            // thì hình ảnh quân cờ "dính" vào con trỏ chuột một cách hợp lí
                            oldPos = f[i].getPosition();
                            // lưu lại vị trí cũ của quân cờ cho các hành động sau
                        }
                // nhả chuột
                if (e.type == Event::MouseButtonReleased && e.mouseButton.button == Mouse::Left) {
                    isMove = false;    
                    Vector2f p = f[n].getPosition() + Vector2f(squareSize / 2, squareSize / 2);
                    // vector2f của vị trí quân cờ 
                    // cần phải cộng thêm để lấy vị trí trung tâm của quân cờ, 
                    // do hàm getPosition trả về tọa độ góc trên bên trái của ảnh quân cờ
                    newPos = Vector2f(squareSize * int(p.x / squareSize), squareSize * int(p.y / squareSize));
                    // tính toán vị trí sao cho khi quân cờ được đặt xuống, thì vị trí mới của quân cờ nằm trên 
                    // các ô cờ (có thể hiểu là làm tròn vị trí mới của quân cờ để quân cờ snap vào đúng ô cờ)
                    str = toChessNote(oldPos) + toChessNote(newPos);
                    // ví dụ, position có thể là "e2e4 d7d5 "
                    move(str);
                    if (oldPos != newPos) position += str + " ";
                    f[n].setPosition(newPos);
                }
            }
            // Stockfish move
            if (choice == PvE && Keyboard::isKeyPressed(Keyboard::Space)) {
                str = getNextMove(position);
                oldPos = toCoord(str[0], str[1]);
                newPos = toCoord(str[2], str[3]);

                // find piece that need to move
                for (int i = 0; i < 32; i++) {
                    if (f[i].getPosition() == oldPos) {
                        n = i;
                    }
                }
                // animation
                for (int k = 0; k < 400; k++) {
                    Vector2f p = newPos - oldPos;
                    f[n].move(p.x / 400, p.y / 400);

                    window.clear();           
                    window.draw(sBoard);      

                    // Offset and draw each piece
                    for (int i = 0; i < 32; i++) f[i].move(offset);
                    for (int i = 0; i < 32; i++) window.draw(f[i]);
                    for (int i = 0; i < 32; i++) f[i].move(-offset);

                    window.display();          // Display the current frame
                }

                // Finalize the piece's position after animation completes
                move(str);                     
                position += str + " ";         
                f[n].setPosition(newPos);      
            }

            // load hình ảnh
            if (isMove) f[n].setPosition(pos.x - dx, pos.y - dy);
            window.clear();
            window.draw(sBoard);
            for (int i = 0; i < 32; i++) {
                f[i].move(offset);
            }
            for (int i = 0; i < 32; i++) {
                window.draw(f[i]); 
            }
            window.draw(f[n]);
            for (int i = 0; i < 32; i++) {
                f[i].move(-offset);
            }
            window.display();
        }
        CloseConnection();
        return 0;
    }

    else {
        window.close();
        return 0;
    }
}