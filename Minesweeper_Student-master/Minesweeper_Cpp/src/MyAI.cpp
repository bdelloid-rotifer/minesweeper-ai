// ======================================================================
// FILE:        MyAI.cpp
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

#include "MyAI.hpp"

MyAI::MyAI ( int _rowDimension, int _colDimension, int _totalMines, int _agentX, int _agentY ) : Agent()
{
    // ======================================================================
    // YOUR CODE BEGINS
    // ======================================================================

    rowDimension = _rowDimension;
    colDimension = _colDimension;
    totalMines = _totalMines;
    agentX = _agentX;
    agentY = _agentY;

    state initialState = {true, false};
    tileStates = vector<vector<state>>(colDimension, vector<state>(rowDimension, initialState));
    tileStates[agentX][agentY].covered = false;
    nextX = 0;
    nextY = 0;
    lastX = agentX;
    lastY = agentY;

    // ======================================================================
    // YOUR CODE ENDS
    // ======================================================================
};

Agent::Action MyAI::getAction( int number )
{
    // ======================================================================
    // YOUR CODE BEGINS
    // ======================================================================

    /*
    C = columns
    R = rows
    M = mines
    Effective label = tile number - number of marked mine neighbors
    successful game: UNCOVER #C*#R-#M times then LEAVE
    getAction(...)
        { 
        are we done?: #CoveredTiles = #Mines -> LEAVE otherwise need to figure out UNCOVER X,Y
        > two types of covered tiles: (1) unmarked, (2) marked
        ...
        use simple rule of thumb logic -> UNCOVER X,Y
        > if effective label is 0, eg the tile says 3 and we already marked 3 neighboring tiles as mines, all the other tiles are safe to UNCOVER
        OR 
        > if effective label is the same as the # unknown neighboring tiles, they can all be marked as mines
        ...
        if this did not give tile to uncover, use better logic -> UNCOVER X,Y
    `   > look at neighboring numbered tiles to see if we can figure out if an uncovered tile is safe,
        > eg. tile A: x + y = 1. tile B: x + y + z = 1. By process of elimination z must = 0. so we can uncover z (it's safe).
        ...
        if this did not give tile to uncover, go to even more sophisticated logic -> UNCOVER X,Y
        > AAAAAAAAAAAAAAAAAAAAAAAAAA
        ...
        finally, if still no tile (this will happen since some partial boards are undecidable), need to guess 
        - exact probability computation may be intractable, use approximation -> UNCOVER X,Y
        > p(mine) = #solutions_with_mine/#solutions. Pick tile with smallest p(mine)
        }
    */

    /* TIMEOUT LOGIC (from slides)
    
    double total_time_elapsed = 0.0
    ...
    Action getAction(...)
    {
        double remaining_time = ...
        if (remaining_time < some_small_number_eg_3)
            make_random_move
        else {
            tS = time_stamp_now
            // do your normal stuff
            tE = time_stamp_now
            dt = tE - tS // time used for this getAction() call
            total_time_elapsed += dt
        }
    }
    
    */


    // if the last tile was 0, its neighbors should be safe
    if (number == 0)
    {
        for (int x = lastX - 1; x <= lastX + 1; ++x)
        {
            for (int y = lastY - 1; y <= lastY + 1; ++y)
            {
                if (x >= 0 && x < colDimension && y >= 0 && y < rowDimension && tileStates[x][y].covered && !tileStates[x][y].flagged)
                {
                    tileStates[x][y].covered = false;
                    safeMoves.push_back({x, y});
                }
            }
        }
    }

    // try safe moves before going back to dumb left-to-right searching
    if (!safeMoves.empty())
    {
        pair<int, int> move = safeMoves.back();
        safeMoves.pop_back();
        lastX = move.first;
        lastY = move.second;
        return {UNCOVER, lastX, lastY};
    }

    while (nextY < rowDimension)
    {
        // check if tile covered and not flagged (for obvious reasons), if it fails, then move cursor forward
        if (tileStates[nextX][nextY].covered && !tileStates[nextX][nextY].flagged)
        {
            int x = nextX;
            int y = nextY;

            tileStates[x][y].covered = false;
            lastX = x;
            lastY = y;

            ++nextX;
            if (nextX == colDimension)
            {
                nextX = 0;
                ++nextY;
            }

            return {UNCOVER, x, y};
        }

        ++nextX;
        // we reached the end of the row, head to beginning of next row
        if (nextX == colDimension)
        {
            nextX = 0;
            ++nextY;
        }
    }

    return {LEAVE, -1, -1};
    // ======================================================================
    // YOUR CODE ENDS
    // ======================================================================

}


// ======================================================================
// YOUR CODE BEGINS
// ======================================================================



// ======================================================================
// YOUR CODE ENDS
// ======================================================================
