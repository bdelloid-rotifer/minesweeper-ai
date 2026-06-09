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
    flaggedCount = 0;
    modelCheckingChanged = false;
    startTime = std::chrono::steady_clock::now();

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
        > model checking stuff
        > frontier = covered tiles next to numbers
        > each numbered tile gives us a tiny equation
        > eg. if tile says 2 and 1 neighbor is already flagged, then the covered neighbors need 1 mine total
        > so...
            1. grab the frontier tiles
            2. try mine/safe combos for them
            3. if combo does not match the numbers, throw it away
            4. count how often each tile is a mine in the combos that survived
        > if mine count for tile is 0, tile is safe, uncover it
        > if mine count for tile is all combos, tile is mine, flag it
        > if nothing is guaranteed, pick tile with lowest p(mine)
        > BUT 2^frontier tiles gets stupid really fast
        > so if there are too many frontier tiles: do not be an idiot
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

    if (!almostOutOfTime() && !frontier.empty() && !constraints.empty()) {

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

    // try model checking before we go back to guessing randomly-ish
    pair<int, int> modelGuess{-1, -1};
    modelCheckingChanged = false;
    if (!almostOutOfTime())
    {
        modelGuess = tryModelChecking();
    }

    if (!safeMoves.empty())
    {
        pair<int, int> move = safeMoves.back();
        safeMoves.pop_back();
        lastX = move.first;
        lastY = move.second;
        --coveredLeft;
        return {UNCOVER, lastX, lastY};
    }

    // if model checking only flagged stuff, think again before guessing
    if (modelCheckingChanged)
    {
        return getAction(number);
    }

    // no guaranteed moves, use the least painful model checking guess
    if (modelGuess.first != -1)
    {
        lastX = modelGuess.first;
        lastY = modelGuess.second;
        tileStates[lastX][lastY].covered = false;
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

bool MyAI::almostOutOfTime() {
    std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - startTime;
    return elapsed.count() > 285.0;
}

std::pair<int, int> MyAI::tryModelChecking() {
    modelCheckingChanged = false;
    if (frontier.empty() || constraints.empty()) return {-1, -1};

    const int maxBlobSize = 80;
    const int maxNodesChecked = 5000000;

    struct modelPiece
    {
        vector<pair<int, int>> tiles;
        vector<long double> worldsByMineCount;
        vector<vector<long double>> mineCountsByMineCount;
    };

    vector<constraint> modelConstraints;
    set<pair<int, int>> modelFrontier;
    set<pair<vector<pair<int, int>>, int>> seenConstraints;

    // clean up the equations in case earlier logic already flagged stuff
    for (const constraint& c: constraints)
    {
        constraint clean;
        clean.x = c.x;
        clean.y = c.y;
        clean.effectiveLabelC = c.effectiveLabelC;

        for (const pair<int, int>& tile: c.neighborVariables)
        {
            if (tileStates[tile.first][tile.second].flagged)
            {
                --clean.effectiveLabelC;
            }
            else if (tileStates[tile.first][tile.second].covered)
            {
                clean.neighborVariables.push_back(tile);
            }
        }

        sort(clean.neighborVariables.begin(), clean.neighborVariables.end());
        if (!clean.neighborVariables.empty() &&
            clean.effectiveLabelC >= 0 &&
            clean.effectiveLabelC <= clean.neighborVariables.size() &&
            seenConstraints.insert({clean.neighborVariables, clean.effectiveLabelC}).second)
        {
            modelConstraints.push_back(clean);
            for (const pair<int, int>& tile: clean.neighborVariables)
            {
                modelFrontier.insert(tile);
            }
        }
    }

    constraints = modelConstraints;
    frontier = modelFrontier;
    if (frontier.empty() || constraints.empty()) return {-1, -1};

    map<pair<int, int>, vector<pair<int, int>>> graph;
    for (const pair<int, int>& tile: frontier)
    {
        graph[tile] = vector<pair<int, int>>();
    }

    // split frontier into blobs so 2^n does not send us to hell
    for (const constraint& c: constraints)
    {
        for (const pair<int, int>& a: c.neighborVariables)
        {
            for (const pair<int, int>& b: c.neighborVariables)
            {
                if (a != b)
                {
                    graph[a].push_back(b);
                }
            }
        }
    }

    map<pair<int, int>, int> frontierDegree;
    for (const constraint& c: constraints)
    {
        for (const pair<int, int>& tile: c.neighborVariables)
        {
            ++frontierDegree[tile];
        }
    }

    pair<int, int> bestGuess{-1, -1};
    double bestRisk = 2.0;

    set<pair<int, int>> visited;
    vector<modelPiece> pieces;
    set<pair<int, int>> tilesWeActuallyChecked;
    bool foundGuaranteedStuff = false;

    for (const pair<int, int>& start: frontier)
    {
        if (visited.find(start) != visited.end()) continue;

        vector<pair<int, int>> component;
        vector<pair<int, int>> queue;
        queue.push_back(start);
        visited.insert(start);

        for (int i = 0; i < queue.size(); ++i)
        {
            pair<int, int> current = queue[i];
            component.push_back(current);

            for (const pair<int, int>& next: graph[current])
            {
                if (visited.find(next) == visited.end())
                {
                    visited.insert(next);
                    queue.push_back(next);
                }
            }
        }

        // too big, do not be an idiot
        if (component.size() > maxBlobSize) continue;

        map<pair<int, int>, int> indexOf;
        for (int i = 0; i < component.size(); ++i)
        {
            indexOf[component[i]] = i;
        }

        vector<vector<int>> constraintTiles;
        vector<int> constraintMines;
        for (const constraint& c: constraints)
        {
            vector<int> localTiles;
            for (const pair<int, int>& tile: c.neighborVariables)
            {
                if (indexOf.find(tile) != indexOf.end())
                {
                    localTiles.push_back(indexOf[tile]);
                }
            }

            if (!localTiles.empty())
            {
                constraintTiles.push_back(localTiles);
                constraintMines.push_back(c.effectiveLabelC);
            }
        }

        vector<vector<int>> tileToConstraints(component.size());
        for (int c = 0; c < constraintTiles.size(); ++c)
        {
            for (int tileIndex: constraintTiles[c])
            {
                tileToConstraints[tileIndex].push_back(c);
            }
        }

        vector<int> assigned(component.size(), -1);
        vector<int> currentMines(constraintTiles.size(), 0);
        vector<int> unassigned(constraintTiles.size(), 0);
        vector<double> mineCounts(component.size(), 0.0);
        vector<long double> worldsByMineCount(component.size() + 1, 0.0);
        vector<vector<long double>> mineCountsByMineCount(component.size(), vector<long double>(component.size() + 1, 0.0));

        for (int c = 0; c < constraintTiles.size(); ++c)
        {
            unassigned[c] = constraintTiles[c].size();
        }

        double legalWorlds = 0.0;
        int minesInCombo = 0;
        int nodesChecked = 0;
        bool capped = false;

        function<void(int)> tryStuff = [&](int index)
        {
            if (capped) return;
            if (++nodesChecked > maxNodesChecked || almostOutOfTime())
            {
                capped = true;
                return;
            }

            if (index == component.size())
            {
                for (int c = 0; c < constraintTiles.size(); ++c)
                {
                    if (currentMines[c] != constraintMines[c]) return;
                }

                ++legalWorlds;
                worldsByMineCount[minesInCombo] += 1.0;
                for (int i = 0; i < assigned.size(); ++i)
                {
                    if (assigned[i] == 1)
                    {
                        ++mineCounts[i];
                        mineCountsByMineCount[i][minesInCombo] += 1.0;
                    }
                }
                return;
            }

            // try mine/safe combos for this blob
            for (int value = 0; value <= 1; ++value)
            {
                assigned[index] = value;
                minesInCombo += value;
                bool legalEnough = true;

                for (int c: tileToConstraints[index])
                {
                    currentMines[c] += value;
                    --unassigned[c];
                }

                for (int c: tileToConstraints[index])
                {
                    if (currentMines[c] > constraintMines[c] ||
                        currentMines[c] + unassigned[c] < constraintMines[c])
                    {
                        legalEnough = false;
                        break;
                    }
                }

                if (legalEnough)
                {
                    tryStuff(index + 1);
                }

                for (int c: tileToConstraints[index])
                {
                    currentMines[c] -= value;
                    ++unassigned[c];
                }

                assigned[index] = -1;
                minesInCombo -= value;
            }
        };

        tryStuff(0);

        // fake board or too much board, throw it away
        if (capped || legalWorlds == 0.0) continue;

        modelPiece piece;
        piece.tiles = component;
        piece.worldsByMineCount = worldsByMineCount;
        piece.mineCountsByMineCount = mineCountsByMineCount;
        pieces.push_back(piece);
        for (const pair<int, int>& tile: component)
        {
            tilesWeActuallyChecked.insert(tile);
        }

        for (int i = 0; i < component.size(); ++i)
        {
            pair<int, int> tile = component[i];
            double risk = mineCounts[i] / legalWorlds;

            // all legal boards agree, so we can trust it
            if (mineCounts[i] == 0.0)
            {
                if (tileStates[tile.first][tile.second].covered && !tileStates[tile.first][tile.second].flagged)
                {
                    tileStates[tile.first][tile.second].covered = false;
                    safeMoves.push_back(tile);
                    modelCheckingChanged = true;
                    foundGuaranteedStuff = true;
                }
            }
            else if (mineCounts[i] == legalWorlds)
            {
                if (tileStates[tile.first][tile.second].covered && !tileStates[tile.first][tile.second].flagged)
                {
                    tileStates[tile.first][tile.second].flagged = true;
                    ++flaggedCount;
                    modelCheckingChanged = true;
                    foundGuaranteedStuff = true;
                }
            }
            else if (!foundGuaranteedStuff)
            {
                bool useThisGuess = false;

                if (bestGuess.first == -1)
                {
                    useThisGuess = true;
                }
                else if (risk < bestRisk)
                {
                    useThisGuess = true;
                }
                else if (risk <= bestRisk)
                {
                    int thisTieBreaker = 0;
                    int bestTieBreaker = 0;

                    if (colDimension == 30 && rowDimension == 16)
                    {
                        thisTieBreaker = frontierDegree[tile];
                        bestTieBreaker = frontierDegree[bestGuess];
                    }

                    if (thisTieBreaker > bestTieBreaker)
                    {
                        useThisGuess = true;
                    }
                }

                if (useThisGuess)
                {
                    bestRisk = risk;
                    bestGuess = tile;
                }
            }
        }
    }

    if (foundGuaranteedStuff) return {-1, -1};

    // use mine count left on the whole board to make the guesses less dumb
    int minesLeft = totalMines - flaggedCount;
    int unknownLeft = 0;
    for (int x = 0; x < colDimension; ++x)
    {
        for (int y = 0; y < rowDimension; ++y)
        {
            if (tileStates[x][y].covered && !tileStates[x][y].flagged)
            {
                ++unknownLeft;
            }
        }
    }

    int outsideTiles = unknownLeft - tilesWeActuallyChecked.size();
    auto combos = [](int n, int k) -> long double
    {
        if (k < 0 || k > n) return 0.0;
        if (k > n - k) k = n - k;

        long double answer = 1.0;
        for (int i = 1; i <= k; ++i)
        {
            answer = answer * (n - k + i) / i;
        }
        return answer;
    };

    if (minesLeft >= 0 && outsideTiles >= 0 && !pieces.empty())
    {
        vector<long double> allWorlds(minesLeft + 1, 0.0);
        allWorlds[0] = 1.0;

        for (const modelPiece& piece: pieces)
        {
            vector<long double> next(minesLeft + 1, 0.0);
            for (int used = 0; used <= minesLeft; ++used)
            {
                if (allWorlds[used] == 0.0) continue;

                for (int mineCount = 0; mineCount < piece.worldsByMineCount.size() && used + mineCount <= minesLeft; ++mineCount)
                {
                    next[used + mineCount] += allWorlds[used] * piece.worldsByMineCount[mineCount];
                }
            }
            allWorlds.swap(next);
        }

        long double totalWorlds = 0.0;
        for (int used = 0; used <= minesLeft; ++used)
        {
            totalWorlds += allWorlds[used] * combos(outsideTiles, minesLeft - used);
        }

        if (totalWorlds > 0.0)
        {
            for (int target = 0; target < pieces.size(); ++target)
            {
                vector<long double> otherWorlds(minesLeft + 1, 0.0);
                otherWorlds[0] = 1.0;

                for (int pieceIndex = 0; pieceIndex < pieces.size(); ++pieceIndex)
                {
                    if (pieceIndex == target) continue;

                    vector<long double> next(minesLeft + 1, 0.0);
                    for (int used = 0; used <= minesLeft; ++used)
                    {
                        if (otherWorlds[used] == 0.0) continue;

                        for (int mineCount = 0; mineCount < pieces[pieceIndex].worldsByMineCount.size() && used + mineCount <= minesLeft; ++mineCount)
                        {
                            next[used + mineCount] += otherWorlds[used] * pieces[pieceIndex].worldsByMineCount[mineCount];
                        }
                    }
                    otherWorlds.swap(next);
                }

                const modelPiece& piece = pieces[target];
                for (int tileIndex = 0; tileIndex < piece.tiles.size(); ++tileIndex)
                {
                    long double mineWorlds = 0.0;
                    for (int mineCount = 0; mineCount < piece.worldsByMineCount.size() && mineCount <= minesLeft; ++mineCount)
                    {
                        if (piece.mineCountsByMineCount[tileIndex][mineCount] == 0.0) continue;

                        long double otherWays = 0.0;
                        for (int usedSomewhereElse = 0; usedSomewhereElse + mineCount <= minesLeft; ++usedSomewhereElse)
                        {
                            otherWays += otherWorlds[usedSomewhereElse] * combos(outsideTiles, minesLeft - mineCount - usedSomewhereElse);
                        }

                        mineWorlds += piece.mineCountsByMineCount[tileIndex][mineCount] * otherWays;
                    }

                    double risk = (double)(mineWorlds / totalWorlds);
                    pair<int, int> tile = piece.tiles[tileIndex];
                    bool useThisGuess = false;

                    if (bestGuess.first == -1)
                    {
                        useThisGuess = true;
                    }
                    else if (risk < bestRisk)
                    {
                        useThisGuess = true;
                    }
                    else if (risk <= bestRisk)
                    {
                        int thisTieBreaker = 0;
                        int bestTieBreaker = 0;

                        if (colDimension == 30 && rowDimension == 16)
                        {
                            thisTieBreaker = frontierDegree[tile];
                            bestTieBreaker = frontierDegree[bestGuess];
                        }

                        if (thisTieBreaker > bestTieBreaker)
                        {
                            useThisGuess = true;
                        }
                    }

                    if (useThisGuess)
                    {
                        bestRisk = risk;
                        bestGuess = tile;
                    }
                }
            }
        }
    }

    return bestGuess;
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
