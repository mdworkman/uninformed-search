
#include <chrono>
#include <fstream>
#include <queue>
#include <stack>
#include <string>
#include <unordered_set>
#include <memory>
#include <tuple>
#include <functional>

#include "Puzzle.h"

#define TEST_ITERATIONS 0

using namespace std;

using Puzzle8 = Puzzle<3>;

bool Puzzle8Search(const Puzzle8::PuzzleState& state, char i, int& row, int& col) {
	auto beg = begin(state);
	auto loc = find_if(beg, end(state), bind2nd(equal_to<char>(), i));
	if (loc != state.end()) {
		int dist = int(loc - beg);
		row = dist / state.n;
		col = dist % state.n;
		return true;
	}
	return false;
};

int ManhattanDistance(const Puzzle8::PuzzleState& state, const Puzzle8::PuzzleState& goal, int cumulativeCost)
{
	int distance = 0;
	// blank tile is not included
	for (int i = 1; i < state.size; ++i)
	{
		int goalRow, goalCol;
		bool found = Puzzle8Search(goal, i, goalRow, goalCol);
		assert(found);

		int stateRow, stateCol;
		found = Puzzle8Search(state, i, stateRow, stateCol);
		assert(found);

		distance += abs(goalRow - stateRow) + abs(goalCol - stateCol);
	}
	assert(distance >= 0);
	return distance + cumulativeCost;
}

int ManhattanDistanceInversions(const Puzzle8::PuzzleState& state, const Puzzle8::PuzzleState& goal, int cumulativeCost)
{
	return ManhattanDistance(state, goal, cumulativeCost) + state.Inversions(goal)/2;
}

int ManhattanDistanceGreedy(const Puzzle8::PuzzleState& state, const Puzzle8::PuzzleState& goal, int cumulativeCost)
{
	// greedy doesnt care about the cost to arrive at this point
	return ManhattanDistance(state, goal, 0);
}

int MisplacedTiles(const Puzzle8::PuzzleState& state, const Puzzle8::PuzzleState& goal, int cumulativeCost)
{
	int count = 0;
	// blank tile is not included
	for (int i = 1; i < state.size; ++i)
	{
		int goalRow, goalCol;
		bool found = Puzzle8Search(goal, i, goalRow, goalCol);
		assert(found);

		int stateRow, stateCol;
		found = Puzzle8Search(state, i, stateRow, stateCol);
		assert(found);

		count += bool(goalRow != stateRow || goalCol != stateCol);
	}
	assert(count >= 0 && count < state.size);
	return count + cumulativeCost;
}

void AnalyzePuzzle(const Puzzle8& puzzle, const Puzzle8::PuzzleState& goal)
{
	bool hasSolution = puzzle.HasSolution(goal);
	if (!hasSolution)
	{
		cout << "Puzzle has no solution." << endl;
		cin.ignore();
		cin.get();
		return;
	}
	cout << "Puzzle has a solution." << endl;
	Puzzle8::CostCalc defaultValue;

	vector<tuple<shared_ptr<PuzzleStrategy>,string,Puzzle8::CostCalc>> strategies {{
		make_tuple( make_shared<BreadthFirstSearch>(BreadthFirstSearch()), "BreadthFirstSearch", defaultValue),
		make_tuple( make_shared<DepthFirstSearch>(DepthFirstSearch()), "DepthFirstSearch", defaultValue),
		// 31 moves is the maximum number needed to solve an 8puzzle so we limit depth to be that
		make_tuple( make_shared<DepthLimitedSearch>(DepthLimitedSearch(31)), "DepthLimitedSearch", defaultValue),
		make_tuple( make_shared<IterativeDeepeningSearch>(IterativeDeepeningSearch(1)), "IterativeDeepeningSearch", defaultValue),
		make_tuple( make_shared<BiDirectionalSearch>(BiDirectionalSearch()), "BiDirectionalSearch", defaultValue),
		make_tuple( make_shared<QueueStrategy>(QueueStrategy()), "ManhattanDistance", ManhattanDistance),
		make_tuple( make_shared<QueueStrategy>(QueueStrategy()), "ManhattanDistanceInversions", ManhattanDistanceInversions),
		make_tuple( make_shared<QueueStrategy>(QueueStrategy()), "ManhattanDistanceGreedy", ManhattanDistanceGreedy),
		make_tuple( make_shared<QueueStrategy>(QueueStrategy()), "MisplacedTiles", MisplacedTiles)
	}};

	for (auto& package : strategies) {
		shared_ptr<PuzzleStrategy> strategy;
		string message;
		Puzzle8::CostCalc valuator;

		tie(strategy, message, valuator) = package;

		shared_ptr<Puzzle8> puzzleCopy;

		cout << "Attempting to solve puzzle:" << endl << puzzle << endl;
		cout << "Attempting to solve with " << message << endl;
		cout << "Beginning timer" << endl;
		chrono::steady_clock::time_point begin = chrono::steady_clock::now();

		bool solved = false;
		do {
			// copy the puzzle so we can attempt to solve it multiple times
			// and use multiple different methods
			puzzleCopy = make_shared<Puzzle8>(puzzle);
			if (valuator) {
				solved = puzzleCopy->Solve(goal, *strategy, valuator);
			} else {
				solved = puzzleCopy->Solve(goal, *strategy);
			}
		} while (!solved && strategy->ExpandSearch());

		cout << "Finished: stopping timer." << endl;
		chrono::steady_clock::time_point end = chrono::steady_clock::now();
		cout << "Time taken: " << chrono::duration_cast<chrono::milliseconds>(end - begin).count() << "ms" << endl;

		assert(solved == hasSolution);
	}
}

void Tests(const Puzzle8::PuzzleState& goal)
{
	Puzzle8 testPuzzle(goal);
	testPuzzle.Scramble(100);
	assert(testPuzzle.HasSolution(goal));

	AnalyzePuzzle(testPuzzle, goal);
}

int main()
{
	Puzzle8::PuzzleState goal {{
		{ 1, 2, 3 },
		{ 4, 5, 6 },
		{ 7, 8, 0 }
	}};
#if TEST_ITERATIONS
	cout << "Running tests with goal:" << endl << goal << endl;

	int i = TEST_ITERATIONS;
	while (i--) {
		Tests(goal);
	}
#else
	string file_name = "";
	cout << "Enter a file name to read a puzzle from." << endl;
	if (!(cin >> file_name))
	{
		cout << "\nThe file name was invalid." << endl;
		cin.ignore();
		cin.get();
		return 1;
	}

	ifstream in(file_name);
	if (!in)
	{
		cout << "The file was not found or could not be opened." << endl;
		cin.ignore();
		cin.get();
		return 1;
	}

	Puzzle8::PuzzleState state;
	auto size = state.n;
    // Use this int with a "bitmask"
    int flags = 0;
    
	// The location of the zero (i.e. empty) tile.
    int zero[2] = {-1, -1};
	int count = 0;
	while (in && count <= 8)
	{
		char temp = '\0';
		if (in >> temp)
		{
			if (temp == '_')
			{
				state[count / size][count % size] = 0;
				zero[0] = count / size;
				zero[1] = count++ % size;
			}
			else
			{
				state[count / size][count % size] = temp - '0';
                // Use 2^(value - 1) as a bitmask of sorts to make sure all values have been entered and
                // they are all valid (e.g. 1-8).
                flags += pow(2, ( (temp - '0') - 1) );
				++count;
			}
		}
	};
    // Check that the puzzle has all needed values
    if( !(flags == 255 && zero[0] >= 0) )
    {
        cout << "The inputted puzzle was not valid. Puzzle was:" << endl;
        cout << state << endl;
		cin.ignore();
        cin.get();
        return 1;
    }

	Puzzle8 puzzle(state);
	AnalyzePuzzle(puzzle, goal);
#endif

	cin.ignore();
	cin.get();
	return 0;
}
