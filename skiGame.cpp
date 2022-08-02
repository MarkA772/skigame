#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
#include "SFML/Graphics.hpp"

using namespace std;

const int gameWidth = 50;
const int gameHeight = 20;
const int playerPos[] = { 24, 2 };
const double gameSpeed = 200; // update rate in ms
const int windowWidth = gameWidth * 20;
const int windowHeight = gameHeight * 20;

struct Obstacle {
    int x;
    int y;
    char character;
    Obstacle(int x, int y, char character) :
        x(x), y(y), character(character) {};
};

bool isNeg(Obstacle o) {
    if (o.y >= 0) {
        return false;
    }
    return true;
}

enum class gameStates { start, left, strait, right };

class SFMLDisplay {
public:
    SFMLDisplay() : window(sf::VideoMode(windowWidth, windowHeight), "Ski") {
        createTextures();
    };
    sf::RenderWindow window;
    sf::Texture snailL;
    sf::Texture snailR;
    sf::Texture snailF;
    sf::Texture rock;
    sf::Vector2f spriteScale;

    void createTextures() {
        if (!rock.loadFromFile("rock.png")) {
            cerr << "Unable to load texture.\n";
        }
        if (!snailL.loadFromFile("snailL.png")) {
            cerr << "Unable to load texture.\n";
        }
        if (!snailR.loadFromFile("snailR.png")) {
            cerr << "Unable to load texture.\n";
        }
        if (!snailF.loadFromFile("snailF.png")) {
            cerr << "Unable to load texture.\n";
        }
        spriteScale.x = (float(windowWidth) / gameWidth) / rock.getSize().x;
        spriteScale.y = (float(windowHeight) / gameHeight) / rock.getSize().y;
    }

    void updateSFMLMap(vector<Obstacle> obstacles, gameStates gameState) {
        window.clear(sf::Color::White);

        for (Obstacle o : obstacles) {
            sf::Sprite sprite;
            sprite.setTexture(rock);
            sprite.setScale(spriteScale);
            vector<int> pos = translateCoords(o.x, o.y);
            sprite.setPosition(pos[0], pos[1]);
            window.draw(sprite);
        }
        sf::Sprite player;
        sf::Texture texture;
        if (gameState == gameStates::left) {
            texture = snailL;
        } else if (gameState == gameStates::right) {
            texture = snailR;
        } else if (gameState == gameStates::start || gameState == gameStates::strait) {
            texture = snailF;
        }
        player.setTexture(texture);
        vector<int> pos = translateCoords(playerPos[0], playerPos[1]);
        player.setPosition(pos[0], pos[1]);
        player.setScale(spriteScale);
        window.draw(player);
        
        window.display();
    }

    vector<int> translateCoords(int x, int y) {
        // Should take ascii/terminal/map coords and translate them to coords that
        //  will work on the sfml canvas
        // 0 -> 0, gameWidth -> windowWidth
        int xStep = windowWidth / gameWidth;
        int yStep = windowHeight / gameHeight;
        vector<int> result;
        result.push_back(x * xStep);
        result.push_back(y * yStep);
        return result;
    }
};

class Display {
public:
    Display() : map(gameHeight, vector<char>(gameWidth, ' ')) {};
    vector<vector<char> > map;

    void printMap() {
        string mapString;
        for (unsigned i = 0; i < map.size(); i++) {
            for (unsigned j = 0; j < map[i].size(); j++) {
                mapString += map[i][j];
            }
            mapString += '\n';
        }
        cout << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" << mapString;
    }

    void updateMap(vector<Obstacle> obstacles, gameStates gameState) {
        map = vector<vector<char> >(20, vector<char>(50, ' '));
        for (Obstacle o : obstacles) {
            if (o.x >= 0 && o.x < gameWidth) {
                map[o.y][o.x] = o.character;
            }
        }
        int x = playerPos[0];
        int y = playerPos[1];
        switch (gameState) {
        case gameStates::strait:
        case gameStates::start:
            map[y][x] = '|';
            break;
        case gameStates::left:
            map[y][x] = '/';
            break;
        case gameStates::right:
            map[y][x] = '\\';
            break;
        }
    }
};

class Game {
public:
    Game() : display(), sfmlDisplay(), gameState(gameStates::start), gameEnded(false), obstacles() {};
    Display display;
    SFMLDisplay sfmlDisplay;
    vector<Obstacle> obstacles;
    bool gameEnded;
    chrono::time_point<chrono::high_resolution_clock> lastTime;
    gameStates gameState;

    void runGame() {
        lastTime = chrono::high_resolution_clock::now();
        sf::Event sfevent;
        while (!gameEnded) {
            sfmlDisplay.window.pollEvent(sfevent);
            if (sfevent.type == sf::Event::Closed) {
                break;
            }
            checkInput();
            chrono::time_point<chrono::high_resolution_clock> currentTime = chrono::high_resolution_clock::now();
            auto delta = chrono::duration_cast<chrono::milliseconds>(currentTime - lastTime);
            if (delta.count() < gameSpeed) {
                continue;
            }
            lastTime = currentTime;
            switch (gameState) {
            case gameStates::right:
                goRight();
                break;
            case gameStates::strait:
                goStrait();
                break;
            case gameStates::left:
                goLeft();
                break;
            case gameStates::start:
                rest();
                break;
            }
            killOldObstacles();
            checkEnd();
            display.updateMap(obstacles, gameState);
            sfmlDisplay.updateSFMLMap(obstacles, gameState);
            display.printMap();
        }
        cout << endl << "Game over." << endl;
        cin.get();
    }

    void checkInput() {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            gameState = gameStates::left;
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            gameState = gameStates::strait;
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            gameState = gameStates::right;
        }
    }

    void rest() {

    }

    void goRight() {
        moveObstacles(-1, -1);
    }

    void goStrait() {
        moveObstacles(0, -1);
    }

    void goLeft() {
        moveObstacles(1, -1);
    }

    void moveObstacles(int x, int y) {
        for (Obstacle& o : obstacles) {
            o.x += x;
            o.y += y;
        }
        generateObstacles();
    }

    void generateObstacles() {
        int howMany = (rand() % 3) + 1;
        vector<int> positions;
        for (int i = 0; i < howMany; i++) {
            positions.push_back(rand() % gameWidth);
        }
        for (int i : positions) {
            Obstacle o(i, gameHeight - 1, 'T');
            obstacles.push_back(o);
        }
    }

    void killOldObstacles() {
        obstacles.erase(remove_if(obstacles.begin(), obstacles.end(), isNeg), obstacles.end());
    }

    void checkEnd() {
        for (Obstacle o : obstacles) {
            if (o.x == playerPos[0] && o.y == playerPos[1]) {
                gameEnded = true;
            }
        }
    }
};

int main() {
    srand(time(NULL));
    Game g;
    g.runGame();
}