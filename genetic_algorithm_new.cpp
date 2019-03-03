// largely based on https://www.youtube.com/watch?v=BOZfhUcNiqk

#include <iostream>
#include <vector>
#include <random>
#include <math.h>
#include <ctime>
#include <chrono>
#include <ncurses.h>
#include <cstring>
#include <string>
#include <fstream>
#include <unistd.h>
#include <algorithm>
using namespace std;

const char BLANK_CHAR = ' ';
const char V_EDGE_CHAR = '|';
const char H_EDGE_CHAR = '-';
const char DOT_CHAR = 'O';
const char DEAD_CHAR = '#';
const char BEST_CHAR = '$';
const char WIN_CHAR = '*';
const char OBSTACLE_CHAR = 'X';
const int MAP_WIDTH = 30;
const int MAP_HEIGHT = 30;
const int NUM_DOTS = 600;
const int LIFESPAN = 3 * (MAP_WIDTH+MAP_HEIGHT);
const int NUM_GENERATIONS = 1000;  // number of child generations to make
const double ENTROPY = 0.01;  // chance of child making random move instead of following path

struct Location {
    int x;
    int y;
    Location(int xx,int yy) : x(xx),y(yy) {}
};

struct Obstacle {
    int x;  // x-distance from center
    int y;  // y-distance from center
    int w;  // width
    int h;  // height
    Obstacle(int xx,int yy,int ww,int hh) : x(xx),y(yy),w(ww),h(hh) {}
};

const Location START = Location(0,0);
const Location GOAL = Location(MAP_WIDTH-1, MAP_HEIGHT-1);
const vector<Obstacle> OBST_DATA = {  // holds coords of upper left of obstacle, then width and height
    //Obstacle(5,-5,18,4),
    //Obstacle(12,5,6,2)
    //Obstacle(0,0,20,4)
    //Obstacle(5,0,20,4)
    //Obstacle(4,13,16,4),
    Obstacle(22,2,8,16),
    Obstacle(12,20,15,2),
    Obstacle(0,2,12,6)
};
vector<Location> OBSTACLES;

void makeObstacles() {  // change obstacle data to list of all coords with obstacle
    for (int i=0; i<OBST_DATA.size(); i++) {
        Obstacle o = OBST_DATA.at(i);
        for (int y=o.y; y<o.y+o.h; y++)
            for (int x=o.x; x<o.x+o.w; x++)
                OBSTACLES.push_back(Location(x,y));
    }
}

class Dot {
public:
    Dot(Location l) : xy(l), al(true), mc(0) {}
    Dot(int x,int y) : xy(Location(x,y)), al(true), mc(0) {}
    Dot(Location l,vector<int> pp) : xy(l), p(pp), al(true), mc(0) {}
    Dot(int x,int y,vector<int> pp) : xy(Location(x,y)), p(pp), al(true), mc(0) {}
    void kill() { al = false; }
    bool isAlive() { return al; }
    bool hasWon() { return xy.x==GOAL.x && xy.y==GOAL.y; }
    void moveRand() {  // move one space in random direction; die if hit wall or obstacle
        if (isAlive() && mc<LIFESPAN && !hasWon()) {
            int r = rand()%4;
            switch(r) {
                case 0: moveUp(); if (!inBounds() || inObstacle()) { moveDown(); kill(); } break;
                case 1: moveDown(); if (!inBounds() || inObstacle()) { moveUp(); kill(); } break;
                case 2: moveLeft(); if (!inBounds() || inObstacle()) { moveRight(); kill(); } break;
                case 3: moveRight(); if (!inBounds() || inObstacle()) { moveLeft(); kill(); } break;
                default: break;
            }
            p.push_back(r);
            mc++;
        }
    }
    void moveUp() { xy.y--; }
    void moveDown() { xy.y++; }
    void moveLeft() { xy.x--; }
    void moveRight() { xy.x++; }
    void movePath(double entropy) {  // take next step along path; has entropy chance of making random move instead
        if (isAlive() && mc<LIFESPAN && !hasWon()) {
            int dir;
            if (mc >= p.size()) {  // if finished path, move randomly
                dir = rand()%4;
                p.push_back(dir);
            }
            else {  // not finished path
                int r = rand()%100;  // decide whether to move along path or move randomly
                if (r < entropy*100) {  // random move
                    dir = rand()%4;
                    p.at(mc) = dir;  // update path
                }
                else {  // follow path
                    dir = p.at(mc);
                }
            }
            switch (dir) {
                case 0: moveUp(); if (!inBounds() || inObstacle()) { moveDown(); kill(); } break;
                case 1: moveDown(); if (!inBounds() || inObstacle()) { moveUp(); kill(); } break;
                case 2: moveLeft(); if (!inBounds() || inObstacle()) { moveRight(); kill(); } break;
                case 3: moveRight(); if (!inBounds() || inObstacle()) { moveLeft(); kill(); } break;
                default: break;
            }
            mc++;
        }
    }
    bool inBounds() {  // return true if Dot is in bounds
        if (xy.x<0 || xy.x>=MAP_WIDTH || xy.y<0 || xy.y>=MAP_HEIGHT)
            return false;
        return true;
    }
    bool inObstacle() {
        for (int i=0; i<OBSTACLES.size(); i++)
            if (xy.x==OBSTACLES.at(i).x && xy.y==OBSTACLES.at(i).y)
                return true;
        return false;
    }
    Location loc() { return xy; }
    int moveCount() { return mc; }  // how many moves Dot has taken (kill when > lifespan)
    double score() {  // based on distance from goal if hasn't reached it and move count if it has
        int sc = 0;
        if (hasWon()) {
            sc = 500 + ( pow((LIFESPAN-mc)/(double)LIFESPAN,2) ) * 1000;  // score is high since winning Dots are always better than non-winning Dots
        }
        else
            sc = ( pow(MAP_WIDTH-(GOAL.x-xy.x),2) + pow(MAP_HEIGHT-(GOAL.y-xy.y),2) ) *100/(pow(MAP_WIDTH,2)+pow(MAP_HEIGHT,2));  // score 0-100 based on distance
        return sc;
    }
    int pathSize() { return p.size(); }
    vector<int> path() { return p; }
private:
    Location xy;
    bool al;
    int mc;
    vector<int> p;  // list of Dot's moves
};

class Set {  // holds list of Dots
public:
    Set(vector<Dot>& vv) : v(vv) {}
    int size() { return v.size(); }
    Dot dotAt(int i) { return v.at(i); }
    void moveRandAll() {  // move all dots one space in random directions
        for (int i=0; i<v.size(); i++)
            v.at(i).moveRand();
    }
    void movePathAll() {  // move all dots one step along their respective paths (or possibly mutate)
        for (int i=0; i<v.size(); i++)
            v.at(i).movePath(ENTROPY);
    }
    int bestDot() {  // return index of dot with highest score
        double highScore = 0;
        int highDot = 0;
        double threshold = 1.0;  // fractional difference between dots for them to "tie"
        // originally had threshold set to 1.05, but now set to 1 to ignore threshold so that farthest dot must win
        for (int i=0; i<v.size(); i++)
            if (v.at(i).score()/v.at(highDot).score() >= 1/threshold && v.at(i).score()/v.at(highDot).score() <= threshold) {  // similar scores break tie with move count
                if (v.at(i).moveCount() < v.at(highDot).moveCount()) {
                    highScore = v.at(i).score();
                    highDot = i;
                }
            }
            else if (v.at(i).score() > highScore) {
                highScore = v.at(i).score();
                highDot = i;
            }
                
        return highDot;
    }
    void setVec(vector<Dot>& vv) { v = vv; }
private:
    vector<Dot> v;
};

class Map {  // holds character vectors to print
public:
    Map(Set& ss) : s(ss) {
        vector<char> a(MAP_WIDTH,BLANK_CHAR);
        vector<vector<char>> b(MAP_HEIGHT,a);
        v = b;
    }
    void makeMap() {  // make Map from input Set
        // clear map
        for (int i=0; i<v.size(); i++)
            for (int j=0; j<v.at(0).size(); j++) {
                bool isObstacle = false;
                for (int k=0; k<OBSTACLES.size(); k++)
                    if (j==OBSTACLES.at(k).x && i==OBSTACLES.at(k).y)
                        isObstacle = true;
                if (isObstacle)
                    v.at(i).at(j) = OBSTACLE_CHAR;
                else
                    v.at(i).at(j) = BLANK_CHAR;
            }
        // add living dots
        for (int i=0; i<s.size(); i++) {
            Location l = s.dotAt(i).loc();
            if (s.dotAt(i).isAlive())
                v.at(l.y).at(l.x) = DOT_CHAR;
            else
                v.at(l.y).at(l.x) = DEAD_CHAR;
        }
        // label best scoring dot
        v.at(s.dotAt(s.bestDot()).loc().y).at(s.dotAt(s.bestDot()).loc().x) = BEST_CHAR;
        // label winning dots
        for (int i=0; i<s.size(); i++) {
            Location l = s.dotAt(i).loc();
            if (s.dotAt(i).hasWon())
                v.at(l.y).at(l.x) = WIN_CHAR;
        }
    }
    char getCharAt(int x,int y) { return v.at(y).at(x); }
    void setSet(Set& ss) { s = ss; }
private:
    Set s;
    vector<vector<char>> v;
};

/*const char* charToCString(char c) {
    string s = "";
    s += c;
    return s.c_str();
}*/

class Environment {
public:
    Environment(Set& ss,Map& mm) : s(ss),m(mm) {
        int i = 60+log10(NUM_GENERATIONS);
        // window must be wide enough to fit longest possible line (might be "Initial cycle" line (61) or "Evolution cycle #X" (60 + width of number) or map width
        string s = "resize -s "+to_string(MAP_HEIGHT+11)+" "+to_string(max(MAP_WIDTH+7,max(61,i))+1);
        system(s.c_str());
        initscr();
        cbreak();  // accept keys when pressed without needing ENTER
        noecho();  // don't display key presses
    }
    ~Environment() {
        endwin();
    }
    void draw() {  // draw map on terminal
        int col = 5;  // x-coord of upper-left corner edge
        int row = 9;  // y-coord of upper-left corner edge
        // top border
        for (int i=0; i<MAP_WIDTH+2; i++) {
            mvprintw(row,col+i,"%c",H_EDGE_CHAR);
            clrtoeol();
        }
        row++;
        // map rows
        for (int i=0; i<MAP_HEIGHT; i++) {  // for each row
            mvprintw(row,col,"%c",V_EDGE_CHAR);
            for (int j=0; j<MAP_WIDTH; j++)  // for each column
                mvprintw(row,col+j+1,"%c",m.getCharAt(j,i));
            mvprintw(row,col+MAP_WIDTH+1,"%c",V_EDGE_CHAR);
            clrtoeol();
            row++;
        }
        // bottom border
        for (int i=0; i<MAP_WIDTH+2; i++) {
            mvprintw(row,col+i,"%c",H_EDGE_CHAR);
            clrtoeol();
        }
    }
    bool dotsLeft() {  // how many dots still alive
        for (int i=0; i<s.size(); i++)
            if (s.dotAt(i).isAlive())
                return true;
        return false;
    }
    void stepRand() {  // advance dots randomly
        s.moveRandAll();
        m.setSet(s);
        m.makeMap();
    }
    void stepPath() {  // advance dots along parent's path, with random mutations
        s.movePathAll();
        m.setSet(s);
        m.makeMap();
    }
    bool done() {  // check if generation is done moving
        // if all Dots dead, done
        if (!dotsLeft()) return true;
        // if any living Dots still have moves left and haven't won, not done
        for (int i=0; i<s.size(); i++)
            if (s.dotAt(i).isAlive() && s.dotAt(i).moveCount()<LIFESPAN && !s.dotAt(i).hasWon())
                return false;
        // all living Dots have no moves left
        return true;
    }
    void setSet(Set& ss) { s=ss; }
    void setMap(Map& mm) { m=mm; }
    Dot dotAt(int i) { return s.dotAt(i); }
    int bestDot() { return s.bestDot(); }
    bool hasWon() {
        for (int i=0; i<s.size(); i++)
            if (s.dotAt(i).hasWon())
                return true;
        return false;
    }
private:
    Set s;
    Map m;
};

int displayPrompt() {  // options menu
    int dispOption = 0;
    mvprintw(0,0,"Display options: 0 to PLAY ALL cycles");
    clrtoeol();
    mvprintw(1,0,"                 1 to PLAY NEXT cycle");
    clrtoeol();
    mvprintw(2,0,"                 2 to STEP through next cycle");
    clrtoeol();
    mvprintw(3,0,"                 3 to SKIP to next cycle");
    clrtoeol();
    mvprintw(4,0,"                 4 to SKIP ALL cycles and show each result");
    clrtoeol();
    mvprintw(5,0,"                 5 to SHOW FINAL result only: ");
    clrtoeol();
    dispOption = getch() - '0';
    if (dispOption < 0 || dispOption > 5)
        return displayPrompt();
    for (int i=0; i<=5; i++) {
        wmove(stdscr,i,0);
        clrtoeol();
    }
    return dispOption;
}

int main() {
    ofstream debugFile;
    debugFile.open("debugfile.txt");
    srand(chrono::system_clock::now().time_since_epoch().count());  // set random seed
    makeObstacles();
    bool runAgain = true;
    while (runAgain) {  // for each simulation
        
        // initialize map
        
        vector<Dot> v;
        for (int i=0; i<NUM_DOTS; ++i)
            v.push_back(START);
        Set s(v);
        Map m(s);
        m.makeMap();
        Environment e(s,m);
        
        // initial random cycle
        
        mvprintw(7,0,"/////////////////////// Initial cycle ///////////////////////");
        clrtoeol();
        e.draw();
        int dispOption = 1;
        
        // children generations
        
        for (int i=0; i<NUM_GENERATIONS; i++) {  // for each generation
            
            // play generation
            
            if (dispOption >= 1 && dispOption <= 3)
                dispOption = displayPrompt();
            if (dispOption == 0 || dispOption == 4)
                mvprintw(6,0,"Press SPACE to stop auto-run (at end of cycle)");
            mvprintw(7,0,"//////////////////// Evolution cycle #%d ////////////////////",i+1);
            clrtoeol();
            if (dispOption <= 2)
                e.draw();
            while (!e.done()) {
                e.stepPath();
                if (dispOption <= 2) {
                    if (dispOption <= 1)
                        usleep(7500);
                    e.draw();
                    if (dispOption == 2) {
                        mvprintw(6,0,"Press SPACE to step (or hold to step faster)");
                        clrtoeol();
                    }
                    refresh();
                    if (dispOption == 2)
                        while (getch() != ' ');
                }
                refresh();
            }
            if (dispOption <= 4)
                e.draw();
            if (!(dispOption == 0 || dispOption == 4)) {
                wmove(stdscr,6,0);
                clrtoeol();
            }
            int winners = 0;
            refresh();
            
            // make children
            
            vector<Dot> vv;
            vector<Dot> oldDots;
            for (int i=0; i<NUM_DOTS; i++)
                oldDots.push_back(e.dotAt(i));
            int scoreSum = 0;
            for (Dot d : oldDots)
                scoreSum += pow(d.score(),2);
            for (int i=0; i<NUM_DOTS; i++) {
                int valueChosen = rand() % scoreSum;
                int dotChosen = 0;
                int cumulScoreSum = 0;
                for (Dot d : oldDots) {
                    cumulScoreSum += pow(d.score(),2);
                    if (cumulScoreSum < valueChosen)
                        dotChosen++;
                    else
                        break;
                }
                Dot copy = Dot(START,oldDots.at(dotChosen).path());
                vv.push_back(copy);
            }
            mvprintw(8,0,"Fastest: %s",(e.hasWon() ? to_string(e.dotAt(e.bestDot()).moveCount()).c_str() : "N/A"));
            clrtoeol();
            s.setVec(vv);
            e.setSet(s);
            e.setMap(m);
            m.makeMap();
            if (dispOption == 0 || dispOption == 4) {
                nodelay(stdscr, true);
                char c = getch();
                if (c == ' ') {
                    dispOption = 1;
                    wmove(stdscr,6,0);
                    clrtoeol();
                }
                nodelay(stdscr,false);
            }
        }
        if (dispOption == 5)
            e.draw();
        refresh();
        mvprintw(6,0,"Run another simulation? (Y/N): ");
        clrtoeol();
        char c = getch();
        if (!(c == 'Y' || c == 'y'))
            runAgain = false;
        wmove(stdscr,6,0);
        clrtoeol();
    }
    debugFile.close();
}