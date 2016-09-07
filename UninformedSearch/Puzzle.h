
#ifndef Puzzle_h
#define Puzzle_h

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_set>

class PuzzleStrategy {
public:
	struct SearchNode {
		// general node members
		const SearchNode* parent = nullptr;
		const size_t cost = 1;
		const size_t depth = 0;

		SearchNode(const SearchNode* parent, size_t cost)
		: parent(parent), cost(cost),
		depth((parent) ? parent->depth + 1 : 0) {}

		virtual void Trace(size_t i) const {};
	};

	using NodePtr = std::shared_ptr<const SearchNode>;

	// existing is a node == to newnode or nullptr if no such node
	virtual bool TestHeuristics(const SearchNode& newnode, const SearchNode* existing) const
	{
		// override in subclasses to provide heuristics
		return bool(existing) == false;
	}

	void Enqueue(const SearchNode& node) {
		Enqueue(std::make_shared<SearchNode>(node));
	}

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
	std::shared_ptr<T> Next() const
	{
		return std::static_pointer_cast<T>(Next());
	}

	PuzzleStrategy(std::function<bool(NodePtr&, NodePtr&)> comp)
	: frontier(comp) {}

private:
	std::priority_queue<NodePtr, std::vector<NodePtr>, std::function<bool(NodePtr&, NodePtr&)>> frontier;
};

class QueueStrategy : public PuzzleStrategy
{
public:
	QueueStrategy()
	: PuzzleStrategy([](NodePtr& lhs, NodePtr& rhs)-> bool {
		return bool(lhs->cost > rhs->cost);
	}) {}
};

class StackStrategy : public PuzzleStrategy
{
public:
	StackStrategy()
	: PuzzleStrategy([](NodePtr& lhs, NodePtr& rhs) {
	return bool(lhs->depth < rhs->depth);
	}) {}
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
		std::cout << "Expanding search depth to " << depth << std::endl;
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

		char state[N][N] = {};

		PuzzleState() = default;

		PuzzleState(std::initializer_list<decltype(state)> l)
		{
			const auto start = &l.begin()[0][0][0];
			std::copy(start, start + size, begin());
		}

		PuzzleState(const PuzzleState& p)
		{
			std::copy(p.begin(), p.end(), begin());
		}

		PuzzleState& operator=(const PuzzleState& p)
		{
			if (&p != this) {
				std::copy(p.begin(), p.end(), begin());
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

		int Inversions(const PuzzleState& /*goal*/) const
		{
			// FIXME: technically we should examine our goal state
			// this is hardcoded for our particular goal state :(

			// the first tile always will have inversions equal to its value - 1, unless it is 0
			int inversions = std::max(*begin() - 1, 0);
			for (auto i = 1; i < size - 1; ++i)
			{
				const auto cell = nth(i);
				const auto val = *cell;

				// no tile is smaller than 1 (since blank tile is *always* ignored in this calculation
				if (val > 1) {
					size_t matches = std::count_if(cell+1, end(), [&val](const char& v) {
						return v && v < val;
					});
					assert(matches < size - i);
					inversions += matches;
				}
			}
			return inversions;
		}

		using iterator = decltype(&state[0][0]);
		using const_iterator = const char*;

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

		friend std::ostream& operator<<(std::ostream& os, const typename Puzzle<N>::PuzzleState& state)
		{
			for (auto i = 0; i < state.size; ++i)
			{
				if (i && i % state.n == 0) {
					std::cout << std::endl;
				}
				std::cout << static_cast<int>(*state.nth(i));
			}
			std::cout << std::endl;
			return os;
		}
	};

	using CostCalc = std::function<int(const PuzzleState&, const PuzzleState&, int)>;

private:
	PuzzleState state;

	char* blank = nullptr;

	void Swap(char*& ptr)
	{
		std::swap(*ptr, *blank);
		blank = ptr;
		ptr = nullptr;

		//cout << "new state:" << endl << state;
	}

	char* GetMoveDown()
	{
		char* swp = blank + state.n;
		return (swp < state.end()) ? swp : nullptr;
	}

	char* GetMoveUp()
	{
		char* swp = blank - state.n;
		return (swp >= state.begin()) ? swp : nullptr;
	}

	char* GetMoveRight()
	{
		char* swp = blank + 1;
		return (swp < state.end() && (swp - state.begin()) % state.n != 0) ? swp : nullptr;
	}

	char* GetMoveLeft()
	{
		char* swp = blank - 1;
		return (swp >= state.begin() && (state.end() - swp) % state.n != 1) ? swp : nullptr;
	}

	char* GetMove(MOVE m)
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
				throw std::logic_error("Invalid movement command attempted");
		}
	}

public:

	Puzzle(const PuzzleState& initial)
	: state(initial)
	{
		auto it = find_if(state.begin(), state.end(), bind2nd(std::equal_to<char>(), 0));
		assert(it != state.end()); // we must have a blank tile!
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
	bool HasSolution(const PuzzleState& goal) const
	{
		return state.Inversions(goal) % 2 == 0;
	}

	bool Solve(const PuzzleState& goal, PuzzleStrategy& strategy,
			   CostCalc valuator = [](const PuzzleState&, const PuzzleState&, int) {
				   return 1;
			   })
	{
		struct Node : public PuzzleStrategy::SearchNode {
		private:
			static PuzzleState PuzzleStateFromDir(MOVE action, const PuzzleStrategy::SearchNode& parent)
			{
				assert(action); // dont accept NONE as a valid sequence
				// less than ideal, but create a new puzzle with the parent state
				// manipulate it, then destroy it
				const Node& p = static_cast<const Node&>(parent);
				Puzzle puzzle(p.state);
				puzzle.Move(action);
				return puzzle.State();
			}

			Node(const PuzzleState& state, const Node& parent, MOVE action, std::function<int(const PuzzleState&)> calc)
			: state(state), PuzzleStrategy::SearchNode(&parent, calc(state)), action(action) {}

		public:
			const PuzzleState state;
			const MOVE action;

			// constructor for root node
			Node(const PuzzleState& state)
			: PuzzleStrategy::SearchNode(nullptr,0), state(state), action(MOVE::NONE) {}

			// constructor for child nodes
			Node(const Node& parent, MOVE action, std::function<int(const PuzzleState&)> calc)
			: Node(PuzzleStateFromDir(action, parent), parent, action, calc) {}

			bool operator==(const Node& rhs) const
			{
				// we consider equality based on state alone
				return state == rhs.state;
			}

			void Trace(size_t i) const
			{
				if (parent) {
					if (i == 0)
						std::cout << "Truncated trace route:" << std::endl;
					else {
						parent->Trace(--i);

						static const char* symbols[] = {"", "UP","DOWN","LEFT","RIGHT"};
						std::cout << depth << ": " << symbols[action] << std::endl;
						std::cout << state << std::endl;
					}
				} else {
					std::cout << "Path taken to solve:" << std::endl;
				}
			}
		};

		using NodePtr = std::shared_ptr<const Node>;
		// initialize the frontier with the start state
		PuzzleStrategy& frontier = strategy; // alias the strategy for easy to follow terminology
		frontier.Enqueue(std::make_shared<Node>(Node(state)));
		NodePtr current = frontier.Next<const Node>();

		auto hasher=[](const NodePtr& nodeptr){
			return nodeptr->state.hash();
		};

		auto equals=[](const NodePtr& n1, const NodePtr& n2){
			return *n1 == *n2;
		};

		// FIXME: I've no idea what a good size table is
		std::unordered_set<NodePtr, decltype(hasher), decltype(equals)> explored(1000, hasher, equals);
		explored.insert(current);
		size_t expandedCount = 0, createdCount = 1;

		auto ExpandNode=[&](MOVE direction){
			using namespace std::placeholders;
			auto newnode = std::make_shared<const Node>(Node(*current, direction, bind(valuator, _1, goal, current->depth+1)));
			auto foundit = explored.find(newnode);
			auto found = (foundit != explored.end()) ? (*foundit).get() : nullptr;
			if (strategy.TestHeuristics(*newnode, found)) {
				if (found) {
					explored.erase(foundit);
				}
				explored.insert(newnode);
				frontier.Enqueue(newnode);
				++createdCount;
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
		std::cout << "Search complete: " << ((solved) ? "SUCCESS" : "FAILURE") << std::endl;
		std::cout << "Nodes expanded:" << expandedCount << std::endl;
		std::cout << "Nodes created:" << createdCount << std::endl;
		std::cout << "Depth of terminated search:" << current->depth << std::endl;

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
			std::random_device rd;     // only used once to initialise (seed) engine
			std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
			std::uniform_int_distribution<int> uni(UP, RIGHT); // guaranteed unbiased

			Move(static_cast<MOVE>(uni(rng)));
		}
	}

	bool CheckValidMove(MOVE m)
	{
		return GetMove(m);
	}

	bool Move(MOVE m)
	{
		char* move = GetMove(m);
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
	
	friend std::ostream& operator<<(std::ostream& os, const Puzzle<N>& puzzle)
	{
		return os << puzzle.state << std::endl;// << puzzle.goal;
	}
};

#endif /* Puzzle_h */
