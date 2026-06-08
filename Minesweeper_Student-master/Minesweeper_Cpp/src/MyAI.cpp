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

    state initialState = {true, false, -1};
    tileStates = vector<vector<state>>(colDimension, vector<state>(rowDimension, initialState));
    tileStates[agentX][agentY].covered = false;
    nextX = 0;
    nextY = 0;
    lastX = agentX;
    lastY = agentY;

    coveredLeft = (colDimension*rowDimension) -1;

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
        >>> (5/10/2026) - ADDED :D
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
    /*
    5/10/2026
    ok so current process...
    1. store number of tile we last uncovered
    2. go through stack of "safe spaces"
    3. check effective label stuff (all stuff next to it is mines and flag them all or all stuff next to it is already flagged and clear it all)
    4. BWAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    5. left to right spam until we blow up or miraculously survive

    - C. Yang
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

    /*
    24 May 2026 - A. Lay
    DRAFT AI UPDATE:
    - Added a frontier to keep track of cells that have more uncovered neighbors than flagged neighbors
    - Added a probability function to work with the frontier and make an informed guess
    */

    /*
    8 Jun 2026 - A. Lay
    Linear constraint satisfaction algorithm based on matrix representation and linear algebra. Old methods are fallback.
    */


    tileStates[lastX][lastY].number = number;
    frontier.clear();
    constraints.clear();
    // use any safe move we already found first
    if (!safeMoves.empty())
    {
        pair<int, int> move = safeMoves.back();
        safeMoves.pop_back();
        lastX = move.first;
        lastY = move.second;
        --coveredLeft;
        return {UNCOVER, lastX, lastY};
    }

    // use effective labels to find safe tiles and mines
    for (int x = 0; x < colDimension; ++x)
    {
        for (int y = 0; y < rowDimension; ++y)
        {
            if (!tileStates[x][y].covered && tileStates[x][y].number >= 0)
            {
                int flaggedNeighbors = 0;
                vector<pair<int, int>> unknownNeighbors;

                for (int nx = x - 1; nx <= x + 1; ++nx)
                {
                    for (int ny = y - 1; ny <= y + 1; ++ny)
                    {
                        if (nx >= 0 && nx < colDimension && ny >= 0 && ny < rowDimension && !(nx == x && ny == y))
                        {
                            if (tileStates[nx][ny].flagged)
                                ++flaggedNeighbors;
                            else if (tileStates[nx][ny].covered)
                                unknownNeighbors.push_back({nx, ny});
                        }
                    }
                }

                int effectiveLabel = tileStates[x][y].number - flaggedNeighbors;

                if (effectiveLabel == 0)
                {
                    for (pair<int, int> move : unknownNeighbors)
                    {
                        tileStates[move.first][move.second].covered = false;
                        safeMoves.push_back(move);
                    }
                }
                else if (effectiveLabel == unknownNeighbors.size() && !unknownNeighbors.empty())
                {
                    for (pair<int, int> move : unknownNeighbors)
                    {
                        tileStates[move.first][move.second].flagged = true;
                        ++flaggedCount;
                    }
                }
                else if (!unknownNeighbors.empty())
                {
                    constraints.push_back({x, y, effectiveLabel, unknownNeighbors});
                    for (auto un: unknownNeighbors) {
                        frontier.insert(un);
                    }
                }
            }
        }
    }

    // try the linear constraints

    if (!frontier.empty() && !constraints.empty()) {

        std::map<std::pair<int, int>, int> variablesOfIndices;
        std::vector<std::pair<int, int>> indiciesOfVariables;

        int index{0};
        for (auto f: frontier) {
            variablesOfIndices[f] = index++;
            indiciesOfVariables.push_back(f);

        }

        // system of equations. 
        // row is constraint, each column a covered tile with coefficients 1 for bordering a constraint or 0 for no. 
        // augemented columm for effective label
        std::vector<std::vector<double>> matrix_constraints(constraints.size(), std::vector<double>(indiciesOfVariables.size() + 1, 0.0));
        for (int row{}; row < constraints.size(); ++row) {
           for (const auto& c: constraints[row].neighborVariables) {
                int column = variablesOfIndices[c];
                matrix_constraints[row][column] = 1.0;
           }
           matrix_constraints[row][indiciesOfVariables.size()] = (double)constraints[row].effectiveLabelC;
        }

        // row reduction function
        row_reduce(matrix_constraints);

        // get solutions
        for (int row{}; row < matrix_constraints.size(); ++row) {
            double augmented = matrix_constraints[row].back();
            std::vector<int> positives;
            vector<int> negatives;

            double sum {0.0};

            for (int col{}; col < indiciesOfVariables.size(); ++col) {
                if (matrix_constraints[row][col] > 1e-4) {
                    positives.push_back(col);
                    sum += matrix_constraints[row][col];
                }
                else if (matrix_constraints[row][col] < -1e-4) {
                    negatives.push_back(col);
                }
            }

            //must not have negative constraints
            if (negatives.empty() && !positives.empty()) { 
                // sum of positive values in this row = 0, tiles involved in this constraint must all be safe
                if (std::abs(augmented) < 1e-4) {
                    for (int p: positives) {
                        std::pair<int, int> tile = indiciesOfVariables[p];
                        if (tileStates[tile.first][tile.second].covered && !tileStates[tile.first][tile.second].flagged) {
                            tileStates[tile.first][tile.second].covered = false;
                            safeMoves.push_back(tile);
                        }
                    }
                }
                // else, if sum is the same as the augmented value, every tile in this constraint must be a mine
                else if (std::abs(augmented - sum) < 1e-4) {
                    for (int p: positives) {
                        std::pair<int, int> tile = indiciesOfVariables[p];
                        if (tileStates[tile.first][tile.second].covered && !tileStates[tile.first][tile.second].flagged) {
                            tileStates[tile.first][tile.second].flagged = true;
                            ++flaggedCount;
                        }
                    }
                }
            }
        }


    }

    // try new safe moves and local probability logic before going back to dumb left-to-right searching]
    if (!safeMoves.empty())
    {
        pair<int, int> move = safeMoves.back();
        safeMoves.pop_back();
        lastX = move.first;
        lastY = move.second;
        --coveredLeft;
        return {UNCOVER, lastX, lastY};
    }

    // try using probability and frontier logic
    pair<int, int> leastRisky = findLeastRisky();
    if (leastRisky.first != -1) {
      lastX = leastRisky.first;
      lastY = leastRisky.second;
      tileStates[lastX][lastY].covered = false;
      --coveredLeft;
      return{UNCOVER, leastRisky.first, leastRisky.second};
    }
    // else revert back to dumb logic
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
            --coveredLeft;
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

void MyAI::row_reduce(std::vector<std::vector<double>>& matrix) {
    if (matrix.size() == 0) return;
    int n_rows = matrix.size();
    int m_col = matrix[0].size();
    int focus = 0;

    for (int row{}; row < n_rows; ++row) {

        if (focus >= m_col - 1) {
            return;
        }

        int pivotRow = row;

        while (focus < m_col - 1) {
            double max = std::abs(matrix[row][focus]);
            pivotRow = row;
            for (int i{row+1}; i < n_rows; ++i) {
                double temp = std::abs(matrix[i][focus]);
                if (temp > max) {
                    max = temp;
                    pivotRow = i;
                }
            }
            if (max > 1e-4) { 
                break;
            }
            focus++; // next column
        }
        
        if (focus >= m_col - 1) {
            return;
        }


        if (row != pivotRow) {
            std::swap(matrix[row], matrix[pivotRow]);
        }
        double leading_value = matrix[row][focus];
        for (int c{focus}; c < m_col; ++c) {
            matrix[row][c] /= leading_value;
        }
        for (int k{}; k < n_rows; ++k){
            if (k != row) {
                double leading_value_i = matrix[k][focus];
                if (std::abs(leading_value_i) > 1e-4) {
                    for (int j {focus}; j < m_col; ++j) {
                        matrix[k][j] -= leading_value_i * matrix[row][j];
                    }
                }
            }
        }            
        focus++;
        

    }
}

std::pair<int, int> MyAI::findLeastRisky() {

   // safest determined by least density, calculated by 
   // (number at cell - flagged neighbors)/total covered neighbors.

   if (frontier.size() == 0) return{-1, -1};
   
   double board_density = 0.0; 
   if (coveredLeft > 0) {
        board_density = (double) (totalMines - flaggedCount) / (coveredLeft); // used to track density of remaining unflagged mines
   }
   double minimum_risk{10.0};
   std::pair<int, int> safest{-1, -1};

   std::map<std::pair<int,int>, double> riskUncovered;

   for (int c {}; c < colDimension; ++c) {
      for (int r {}; r < rowDimension; ++r) {
         if (tileStates[c][r].covered && !tileStates[c][r].flagged) {
            riskUncovered[{c, r}] = board_density;
         }
      }
   }

   std::set<std::pair<int, int>> visited;
   for (const auto& c: constraints) {
        int coveredNeighbors{static_cast<int>(c.neighborVariables.size())};
        if (coveredNeighbors > 0) {
            double local_risk = (double)(c.effectiveLabelC) / coveredNeighbors;
            for (const auto& tile: c.neighborVariables) {
                if (tileStates[tile.first][tile.second].covered || !tileStates[tile.first][tile.second].flagged) {
                    if (visited.find(tile) == visited.end()) {
                        riskUncovered[tile] = local_risk;
                        visited.insert(tile);
                    }
                    else if (local_risk > riskUncovered[tile]) {
                        riskUncovered[tile] = local_risk;
                    }
                }
            }
        }
   }

   for (const auto& entry: riskUncovered) {
      const pair<int, int>& tile = entry.first;
      double risk = entry.second;
      if (risk < minimum_risk) {
         minimum_risk = risk;
         safest = tile;
      }
   }

   return safest;
}

// ======================================================================
// YOUR CODE ENDS
// ======================================================================
