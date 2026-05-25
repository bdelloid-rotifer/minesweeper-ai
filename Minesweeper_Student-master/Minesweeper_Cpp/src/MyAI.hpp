// ======================================================================
// FILE:        MyAI.hpp
//
// AUTHOR:      Jian Li
//
// DESCRIPTION: This file contains your agent class, which you will
//              implement. You are responsible for implementing the
//              'getAction' function and any helper methods you feel you
//              need.
//
// NOTES:       - If you are having trouble understanding how the shell
//                works, look at the other parts of the code, as well as
//                the documentation.
//
//              - You are only allowed to make changes to this portion of
//                the code. Any changes to other portions of the code will
//                be lost when the tournament runs your code.
// ======================================================================

#ifndef MINE_SWEEPER_CPP_SHELL_MYAI_HPP
#define MINE_SWEEPER_CPP_SHELL_MYAI_HPP

#include "Agent.hpp"
#include <iostream> // temporary use
#include <vector>
#include <map>
#include <set>
#include <algorithm>

using namespace std;

class MyAI : public Agent
{
public:
    MyAI ( int _rowDimension, int _colDimension, int _totalMines, int _agentX, int _agentY );

    Action getAction ( int number ) override;


    // ======================================================================
    // YOUR CODE BEGINS
    // ======================================================================\

    std::pair<int, int> findLeastRisky();

private:
    // stores if the tile is covered, flagged, and what number it has
    struct state
    {
        bool covered;
        bool flagged;
        int number;
    };

    // tells us the state of all tiles on the board
    vector<vector<state>> tileStates;
    // flagged count
    int flaggedCount;
    // tells us where to look next
    int nextX;
    int nextY;
    // remembers the last tile we tried to uncover
    int lastX;
    int lastY;
    // stores tiles we think are safe to uncover next
    vector<pair<int, int>> safeMoves;
    // number of remaining covered tiles
    int coveredLeft;

    // stores coordinates of tiles that have more covered neighbors than flagged neighbors
    std::vector<std::pair<int, int>> frontier;
    // ======================================================================
    // YOUR CODE ENDS
    // ======================================================================
};

#endif //MINE_SWEEPER_CPP_SHELL_MYAI_HPP
