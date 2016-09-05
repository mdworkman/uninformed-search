#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <stack>
#include <string>
#include <unordered_set>


using namespace std;

class PuzzleStrategy {
public:
	struct SearchNode {
		// general node members
		const size_t cost = 1;
		const size_t depth = 0;

		SearchNode(size_t cost, size_t depth)
		: cost(cost), depth(depth) {}
	};

	using NodePtr = shared_ptr<const SearchNode>;

	// existing is a node == to newnode or nullptr if no such node
	virtual bool TestHeuristics(const SearchNode& newnode, const SearchNode* existing) const
	{
		// override in subclasses to provide heuristics
		return bool(existing) == false;
	}

	virtual void Enqueue(NodePtr) = 0;

	void Enqueue(const SearchNode& node) {
		Enqueue(make_shared<SearchNode>(node));
	}

	virtual void Dequeue() = 0;

	virtual NodePtr Next() const = 0;
	virtual bool Finished() const = 0;
	virtual bool ExpandSearch()
	{
		// by default most searches are complete and dont need expansion
		// override in subclasses that need it
		return false;
	};

	virtual bool IsComplete() const
	{
		// Is the search going to find a solution if one exists?
		// override in subclasses that are not complete
		return true;
	}

	template <class T>
	shared_ptr<T> Next() const
	{
		return static_pointer_cast<T>(Next());
	}
};

class QueueStrategy : public PuzzleStrategy
{
private:
	queue<NodePtr> frontier;

public:

	void Enqueue(NodePtr node)
	{
		frontier.push(node);
	}

	void Dequeue()
	{
		frontier.pop();
	}

	NodePtr Next() const
	{
		return frontier.front();
	}

	bool Finished() const
	{
		return frontier.empty();
	}
};

class StackStrategy : public PuzzleStrategy
{
private:
	stack<NodePtr> frontier;

public:

	void Enqueue(NodePtr node)
	{
		frontier.push(node);
	}

	void Dequeue()
	{
		frontier.pop();
	}

	NodePtr Next() const
	{
		return frontier.top();
	}

	bool Finished() const
	{
		return frontier.empty();
	}
};

using BreadthFirstSearch = QueueStrategy;
using DepthFirstSearch = StackStrategy;

class DepthLimitedSearch : public DepthFirstSearch {
protected:
	size_t depth;

public:
	DepthLimitedSearch(size_t depth)
	: depth(depth) {}

	bool TestHeuristics(const SearchNode& newnode, const SearchNode* existing) const
	{
		return (newnode.depth <= depth && (!existing || existing->depth > newnode.depth));
	}

	bool IsComplete() const
	{
		return false;
	}
};

class IterativeDeepeningSearch : public DepthLimitedSearch {
	size_t step = 10; // how much to increase the depth by

public:
	IterativeDeepeningSearch(size_t depth)
	: DepthLimitedSearch(depth) {}

	bool ExpandSearch()
	{
		// FIXME: we need a max depth or we would loop infinitely on an unsolvable puzzle
		// expand the depth and search again
		depth += step;
		cout << "Expanding search depth to " << depth << endl;
		return true;
	}
};

template<size_t N>
class Puzzle {
public:
	enum MOVE {
		NONE = 0,
		UP,
		DOWN,
		LEFT,
		RIGHT
	};

	struct PuzzleState {
		static constexpr auto n = N;
		static constexpr auto size = N*N;

		int state[N][N] = {};

		PuzzleState() = default;

		PuzzleState(initializer_list<decltype(state)> l)
		{
			const int* start = &l.begin()[0][0][0];
			copy(start, start + size, begin());
		}

		PuzzleState(const PuzzleState& p)
		{
			copy(p.begin(), p.end(), begin());
		}

		PuzzleState& operator=(const PuzzleState& p)
		{
			if (&p != this) {
				copy(p.begin(), p.end(), begin());
			}
			return *this;
		}

		decltype(state[N])& operator[](size_t i)
		{
			return state[i];
		}

		bool operator==(const PuzzleState& rhs) const
		{
			// we dont have to check *all* the elements
			// since each puzzle alwyas has the same numbers we only have to check n - 1
			for (auto i = 0; i < size - 1; ++i)
			{
				if (*nth(i) != *rhs.nth(i)) {
					return false;
				}
			}
			return true;
		}

		size_t hash() const
		{
			static const size_t lg = log2(size); // calculate only once

			size_t h = 0;
			for (auto i = 0; i < size; ++i)
			{
				h |= i << ((*nth(i)) * lg);
			}
			return h;
		}

		using iterator = decltype(&state[0][0]);
		using const_iterator = const int*;

		// providing iterators so we can easily navigate the 2d array as a flat structure
		iterator begin()
		{
			return (iterator)&state;
		}

		const_iterator begin() const
		{
			return (const_iterator)&state;
		}

		iterator end()
		{
			return begin() + size;
		}

		const_iterator end() const
		{
			return begin() + size;
		}

		iterator nth(size_t n)
		{
			return &state[n / N][n % N];
		}

		const_iterator nth(size_t n) const
		{
			return &state[n / N][n % N];
		}

		friend ostream& operator<<(ostream& os, const typename Puzzle<N>::PuzzleState& state)
		{
			for (auto i = 0; i < state.size; ++i)
			{
				if (i && i % state.n == 0) {
					cout << endl;
				}
				cout << *state.nth(i);
			}
			cout << endl;
			return os;
		}
	};

private:
	PuzzleState state;

	int* blank = nullptr;

	void Swap(int*& ptr)
	{
		swap(*ptr, *blank);
		blank = ptr;
		ptr = nullptr;

		//cout << "new state:" << endl << state;
	}

	int* GetMoveDown()
	{
		int* swp = blank + state.n;
		return (swp < state.end()) ? swp : nullptr;
	}

	int* GetMoveUp()
	{
		int* swp = blank - state.n;
		return (swp >= state.begin()) ? swp : nullptr;
	}

	int* GetMoveRight()
	{
		int* swp = blank + 1;
		return (swp < state.end() && (swp - state.begin()) % state.n != 0) ? swp : nullptr;
	}

	int* GetMoveLeft()
	{
		int* swp = blank - 1;
		return (swp >= state.begin() && (state.end() - swp) % state.n != 1) ? swp : nullptr;
	}

	int* GetMove(MOVE m)
	{
		switch (m)
		{
			case UP:
				return GetMoveUp();
			case DOWN:
				return GetMoveDown();
			case LEFT:
				return GetMoveLeft();
			case RIGHT:
				return GetMoveRight();
			default:
				throw logic_error("Invalid movement command attempted");
		}
	}

public:

	Puzzle(const PuzzleState& initial)
	: state(initial)
	{
		auto it = find_if(begin(state), end(state), bind2nd(equal_to<int>(), 0));
		assert(it != end(state)); // we must have a blank tile!
		blank = &(*it);
		assert(*blank == 0);
		// these asserts dont catch if there is more than one 0 so our puzzle may still be invalid
	}

	const PuzzleState& State() const
	{
		return state;
	}

	auto operator[](size_t i) -> decltype(state[i])
	{
		return state[i];
	}

	// we cant actually use this for anything but testing
	bool HasSolution(const PuzzleState& /* goal */) const
	{
		// FIXME: technically we should examine our goal state
		// this is hardcoded for our particular goal state :(

		// the first tile always will have inversions equal to its value - 1, unless it is 0
		int inversions = max(*state.begin() - 1, 0);
		for (auto i = 1; i < state.size - 1; ++i)
		{
			const auto cell = state.nth(i);
			const auto val = *cell;

			// no tile is smaller than 1 (since blank tile is *always* ignored in this calculation
			if (val > 1) {
				size_t matches = count_if(cell+1, state.end(), [&val](const int& v) {
					return v && v < val;
				});
				assert(matches < state.size - i);
				inversions += matches;
			}
		}
		return inversions % 2 == 0;
	}

	bool Solve(const PuzzleState& goal, PuzzleStrategy& strategy)
	{
		struct Node : public PuzzleStrategy::SearchNode {
			PuzzleState state;
			const Node* parent = nullptr;
			const MOVE action;

			// constructor for root node
			Node(const PuzzleState& state)
			: PuzzleStrategy::SearchNode(0,0), state(state), action(MOVE::NONE) {}

			// constructor for child nodes
			Node(const Node& parent, MOVE action)
			: PuzzleStrategy::SearchNode(1, parent.depth + 1), parent(&parent), action(action)
			{
				assert(action); // dont accept NONE as a valid sequence
				// less than ideal, but create a new puzzle with the parent state
				// manipulate it, then destroy it
				Puzzle puzzle(parent.state);
				puzzle.Move(action);
				state = puzzle.State();
			}

			bool operator==(const Node& rhs) const
			{
				// we consider equality based on state alone
				return state == rhs.state;
			}

			void Trace(size_t i) const
			{
				if (parent) {
					if (i == 0)
						cout << "Truncated trace route:" << endl;
					else {
						parent->Trace(--i);

						static const char* symbols[] = {"", "UP","DOWN","LEFT","RIGHT"};
						cout << depth << ": " << symbols[action] << endl;
						cout << state << endl;
					}
				} else {
					cout << "Path taken to solve:" << endl;
				}
			}
		};

		using NodePtr = shared_ptr<const Node>;
		// initialize the frontier with the start state
		PuzzleStrategy& frontier = strategy; // alias the strategy for easy to follow terminology
		frontier.Enqueue(make_shared<Node>(Node(state)));
		NodePtr current = frontier.Next<const Node>();

		auto hasher=[](const NodePtr& nodeptr){
			return nodeptr->state.hash();
		};

		auto equals=[](const NodePtr& n1, const NodePtr& n2){
			return *n1 == *n2;
		};

		// FIXME: I've no idea what a good size table is
		unordered_set<NodePtr, decltype(hasher), decltype(equals)> explored(1000, hasher, equals);
		explored.insert(current);
		size_t expandedCount = 0;

		auto ExpandNode=[&](MOVE direction){
			auto newnode = make_shared<const Node>(Node(*current, direction));
			auto foundit = explored.find(newnode);
			auto found = (foundit != explored.end()) ? (*foundit).get() : nullptr;
			if (strategy.TestHeuristics(*newnode, found)) {
				if (found) {
					explored.erase(foundit);
				}
				explored.insert(newnode);
				frontier.Enqueue(newnode);
			}
		};

		while (!frontier.Finished()) {
			current = frontier.Next<const Node>();
			frontier.Dequeue();

			if (current->state == goal) break;

			// counterclockwise iteration?
			++expandedCount;
			ExpandNode(UP);
			ExpandNode(LEFT);
			ExpandNode(DOWN);
			ExpandNode(RIGHT);
		}

		// update puzzle to current state (even if not solved)
		state = current->state;

		bool solved = IsSolved(goal);
		// FIXME: not sure what "expanded nodes" means
		cout << "Search complete: " << ((solved) ? "SUCCESS" : "FAILURE") << endl;
		cout << "Nodes expanded:" << expandedCount << endl;
		cout << "Depth of terminated search:" << current->depth << endl;

		if (solved) {
			// output the steps
			current->Trace(100);
			return true;
		}
		return false;
	}

	bool IsSolved(const PuzzleState& goal) const
	{
		return state == goal;
	}

	// scramble the puzzle by i moves
	// great for loading a solved puzzle and scrambling for testing
	void Scramble(size_t i)
	{
		while (i--) {
			random_device rd;     // only used once to initialise (seed) engine
			mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
			uniform_int_distribution<int> uni(UP, RIGHT); // guaranteed unbiased

			Move(static_cast<MOVE>(uni(rng)));
		}
	}

	bool CheckValidMove(MOVE m)
	{
		return GetMove(m);
	}

	bool Move(MOVE m)
	{
		int* move = GetMove(m);
		if (move) {
			Swap(move);
			return true;
		}
		return false;
	}

	bool MoveUp()
	{
		return Move(UP);
	}

	bool MoveDown()
	{
		return Move(DOWN);
	}

	bool MoveLeft()
	{
		return Move(LEFT);
	}

	bool MoveRight()
	{
		return Move(RIGHT);
	}

	friend ostream& operator<<(ostream& os, const Puzzle<N>& puzzle)
	{
		return os << puzzle.state << endl;// << puzzle.goal;
	}
};

using Puzzle8 = Puzzle<3>;

auto Puzzle8Search=[](const Puzzle8::PuzzleState& state, size_t i, int& row, int& col) {
	auto beg = begin(state);
	auto loc = find_if(beg, end(state), bind2nd(equal_to<int>(), i));
	int dist = int(loc - beg);
	row = dist / state.n;
	col = dist % state.n;
};

int ManhattanDistance(const Puzzle8::PuzzleState& state, const Puzzle8::PuzzleState& goal)
{
	int distance = 0;
	// blank tile is not included
	for (int i = 1; i < state.size; ++i)
	{
		int goalRow, goalCol;
		Puzzle8Search(goal, i, goalRow, goalCol);

		int stateRow, stateCol;
		Puzzle8Search(state, i, stateRow, stateCol);

		distance += abs(goalRow - stateRow) + abs(goalCol - stateCol);
	}
	assert(distance >= 0);
	return distance;
}

int MisplacedTiles(const Puzzle8::PuzzleState& state, const Puzzle8::PuzzleState& goal)
{
	int count = 0;
	// blank tile is not included
	for (int i = 1; i < state.size; ++i)
	{
		int goalRow, goalCol;
		Puzzle8Search(goal, i, goalRow, goalCol);

		int stateRow, stateCol;
		Puzzle8Search(state, i, stateRow, stateCol);

		count += bool(goalRow != stateRow || goalCol != stateCol);
	}
	assert(count >= 0);
	return count;
}

void AnalyzePuzzle(const Puzzle8& puzzle, const Puzzle8::PuzzleState& goal)
{
	bool hasSolution = puzzle.HasSolution(goal);
	cout << "Puzzle has solution?: " << hasSolution << endl;

	vector<tuple<shared_ptr<PuzzleStrategy>,string>> strategies {{
		make_tuple( make_shared<BreadthFirstSearch>(BreadthFirstSearch()), "BreadthFirstSearch"),
		make_tuple( make_shared<DepthFirstSearch>(DepthFirstSearch()), "DepthFirstSearch"),
		// 31 moves is the maximum number needed to solve an 8puzzle so we limit depth to be that
		make_tuple( make_shared<DepthLimitedSearch>(DepthLimitedSearch(31)), "DepthLimitedSearch"),
		make_tuple( make_shared<IterativeDeepeningSearch>(IterativeDeepeningSearch(10)), "IterativeDeepeningSearch")
	}};

	for (auto& package : strategies) {
		shared_ptr<PuzzleStrategy> strategy;
		string message;

		tie(strategy, message) = package;

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
			solved = puzzleCopy->Solve(goal, *strategy);
		} while (!solved && strategy->ExpandSearch());

		cout << "Finished: stopping timer." << endl;
		chrono::steady_clock::time_point end = chrono::steady_clock::now();
		cout << "Time taken: " << chrono::duration_cast<chrono::milliseconds>(end - begin).count() << "ms" << endl;

		assert(solved == hasSolution);
	}
}

void Tests(const Puzzle8::PuzzleState& goal)
{
	cout << "Running tests" << endl;
	cout << goal << endl;
	Puzzle8 testPuzzle(goal);
	testPuzzle.Scramble(100);
	assert(testPuzzle.HasSolution(goal));

	AnalyzePuzzle(testPuzzle, goal);
}

int main()
{/*
	string file_name = "";
	cout << "Enter a file name to read a puzzle from." << endl;
	if (!(cin >> file_name))
	{
		cout << "\nThe file name was invalid." << endl;
		return 1;
	}

	ifstream in(file_name);
	if (!in)
	{
		cout << "The file was not found or could not be opened." << endl;
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
        cin.get();
        return 1;
    }
*/
	Puzzle8::PuzzleState state {{
		{ 0, 1, 3 },
		{ 8, 2, 6 },
		{ 4, 5, 7 }
	}};

	Puzzle8::PuzzleState goal {{
		{ 1, 2, 3 },
		{ 4, 5, 6 },
		{ 7, 8, 0 }
	}};

	Tests(goal);

	Puzzle8 puzzle(state);
	AnalyzePuzzle(puzzle, goal);

	return 0;
}
