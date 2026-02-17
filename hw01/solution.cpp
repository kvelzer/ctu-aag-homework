#ifndef __PROGTEST__
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "sample.h"
#endif

struct NodeInfo {
    bool nullable;
    std::set<int> first;
    std::set<int> last;
};

class metodaSousedu {
public:
    std::map<int, uint8_t> symbols;
    std::map<int, std::set<int>> followers;
    int indexCounter = 0;

    NodeInfo analyze(const regexp::RegExp& node) {
        return std::visit(overloaded{
            [&](const std::unique_ptr<regexp::Symbol>& arg) -> NodeInfo {
                int id = ++indexCounter;
                symbols[id] = arg->m_symbol;
                return { false, {id}, {id} };
            },
            [&](const std::unique_ptr<regexp::Alternation>& arg) -> NodeInfo {
                NodeInfo left = analyze(arg->m_left);
                NodeInfo right = analyze(arg->m_right);
                
                NodeInfo res;
                res.nullable = left.nullable || right.nullable;
                
                res.first = left.first;
                res.first.insert(right.first.begin(), right.first.end());
                
                res.last = left.last;
                res.last.insert(right.last.begin(), right.last.end());
                
                return res;
            },
            [&](const std::unique_ptr<regexp::Concatenation>& arg) -> NodeInfo {
                NodeInfo left = analyze(arg->m_left);
                NodeInfo right = analyze(arg->m_right);
                
                NodeInfo res;
                res.nullable = left.nullable && right.nullable;
                
                res.first = left.first;
                if (left.nullable) {
                    res.first.insert(right.first.begin(), right.first.end());
                }
                
                res.last = right.last;
                if (right.nullable) {
                    res.last.insert(left.last.begin(), left.last.end());
                }
                
                for (int l : left.last) {
                    for (int f : right.first) {
                        followers[l].insert(f);
                    }
                }
                
                return res;
            },
            [&](const std::unique_ptr<regexp::Iteration>& arg) -> NodeInfo {
                NodeInfo inner = analyze(arg->m_node);
                
                NodeInfo res;
                res.nullable = true;
                res.first = inner.first;
                res.last = inner.last;
                
                for (int l : inner.last) {
                    for (int f : inner.first) {
                        followers[l].insert(f);
                    }
                }
                
                return res;
            },
            [&](const std::unique_ptr<regexp::Epsilon>& arg) -> NodeInfo {
                return { true, {}, {} };
            },
            [&](const std::unique_ptr<regexp::Empty>& arg) -> NodeInfo {
                return { false, {}, {} };
            }
        }, node);
    }
};

std::set<size_t> wordsMatch(const regexp::RegExp& regexp, const std::vector<Word>& words)
{
    metodaSousedu builder;
    NodeInfo root = builder.analyze(regexp);
    
    std::set<size_t> result;

    for (size_t i = 0; i < words.size(); ++i) {
        const auto& word = words[i];
        
        if (word.empty()) {
            if (root.nullable) {
                result.insert(i);
            }
            continue;
        }

        std::set<int> currentStates;
        uint8_t firstChar = word[0];
        
        for (int id : root.first) {
            if (builder.symbols[id] == firstChar) {
                currentStates.insert(id);
            }
        }

        bool possible = !currentStates.empty();

        for (size_t k = 1; k < word.size(); ++k) {
            if (!possible) break;

            uint8_t nextChar = word[k];
            std::set<int> nextStates;

            for (int state : currentStates) {
                for (int neighbor : builder.followers[state]) {
                    if (builder.symbols[neighbor] == nextChar) {
                        nextStates.insert(neighbor);
                    }
                }
            }
            
            currentStates = nextStates;
            if (currentStates.empty()) {
                possible = false;
            }
        }

        if (possible) {
            bool match = false;
            for (int state : currentStates) {
                if (root.last.count(state)) {
                    match = true;
                    break;
                }
            }
            if (match) {
                result.insert(i);
            }
        }
    }

    return result;
}

#ifndef __PROGTEST__
int main()
{
    // basic test 1
    regexp::RegExp re1 = std::make_unique<regexp::Iteration>(
        std::make_unique<regexp::Concatenation>(
            std::make_unique<regexp::Concatenation>(
                std::make_unique<regexp::Concatenation>(
                    std::make_unique<regexp::Iteration>(
                        std::make_unique<regexp::Alternation>(
                            std::make_unique<regexp::Symbol>('a'),
                            std::make_unique<regexp::Symbol>('b'))),
                    std::make_unique<regexp::Symbol>('a')),
                std::make_unique<regexp::Symbol>('b')),
            std::make_unique<regexp::Iteration>(
                std::make_unique<regexp::Alternation>(
                    std::make_unique<regexp::Symbol>('a'),
                    std::make_unique<regexp::Symbol>('b')))));
    assert(wordsMatch(re1, {Word{}}) == std::set<size_t>{0});
    assert(wordsMatch(re1, {Word{'a', 'b'}}) == std::set<size_t>{0});
    assert(wordsMatch(re1, {Word{'a'}}) == std::set<size_t>{});
    assert(wordsMatch(re1, {Word{'a', 'a', 'a', 'a'}}) == std::set<size_t>{});
    assert(wordsMatch(re1, {Word{'a', 'a', 'a', 'c'}}) == std::set<size_t>{});
    assert(wordsMatch(re1, {Word{'a', 'a', 0x07, 'c'}}) == std::set<size_t>{});
    assert(wordsMatch(re1, {Word{'a', 'a', 'b'}}) == std::set<size_t>{0});
    assert(wordsMatch(re1, {Word{'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'b'}}) == std::set<size_t>{0});
    assert((wordsMatch(re1, {Word{}, Word{'a', 'b'}, Word{'a'}, Word{'a', 'a', 'a', 'a'}, Word{'a', 'a', 'a', 'c'}, Word{'a', 'a', 0x07, 'c'}, Word{'a', 'a', 'b'}, Word{'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'b'}}) == std::set<size_t>{0, 1, 6, 7}));

    // basic test 2
    regexp::RegExp re2 = std::make_unique<regexp::Concatenation>(
        std::make_unique<regexp::Concatenation>(
            std::make_unique<regexp::Iteration>(
                std::make_unique<regexp::Concatenation>(
                    std::make_unique<regexp::Concatenation>(
                        std::make_unique<regexp::Iteration>(
                            std::make_unique<regexp::Alternation>(
                                std::make_unique<regexp::Symbol>('a'),
                                std::make_unique<regexp::Symbol>('b'))),
                        std::make_unique<regexp::Iteration>(
                            std::make_unique<regexp::Alternation>(
                                std::make_unique<regexp::Symbol>('c'),
                                std::make_unique<regexp::Symbol>('d')))),
                    std::make_unique<regexp::Iteration>(
                        std::make_unique<regexp::Alternation>(
                            std::make_unique<regexp::Symbol>('e'),
                            std::make_unique<regexp::Symbol>('f'))))),
            std::make_unique<regexp::Empty>()),
        std::make_unique<regexp::Iteration>(
            std::make_unique<regexp::Alternation>(
                std::make_unique<regexp::Symbol>('a'),
                std::make_unique<regexp::Symbol>('b'))));
    assert(wordsMatch(re2, {Word{}}) == std::set<size_t>{});
    assert(wordsMatch(re2, {Word{'a', 'b'}}) == std::set<size_t>{});
    assert(wordsMatch(re2, {Word{'a', 'b', 'c', 'd'}}) == std::set<size_t>{});
    assert(wordsMatch(re2, {Word{'a', 'b', 'c', 'd', 'e', 'f'}}) == std::set<size_t>{});
    assert(wordsMatch(re2, {Word{'a', 'b', 'c', 'd', 'e', 'f', 'a', 'b'}}) == std::set<size_t>{});
    assert((wordsMatch(re2, {Word{}, Word{'a', 'b'}, Word{'a', 'b', 'c', 'd'}, Word{'a', 'b', 'c', 'd', 'e', 'f'}, Word{'a', 'b', 'c', 'd', 'e', 'f', 'a', 'b'}}) == std::set<size_t>{}));

    // basic test 3
    regexp::RegExp re3 = std::make_unique<regexp::Concatenation>(
        std::make_unique<regexp::Concatenation>(
            std::make_unique<regexp::Concatenation>(
                std::make_unique<regexp::Symbol>('0'),
                std::make_unique<regexp::Symbol>('1')),
            std::make_unique<regexp::Symbol>('1')),
        std::make_unique<regexp::Iteration>(
            std::make_unique<regexp::Alternation>(
                std::make_unique<regexp::Alternation>(
                    std::make_unique<regexp::Concatenation>(
                        std::make_unique<regexp::Concatenation>(
                            std::make_unique<regexp::Symbol>('0'),
                            std::make_unique<regexp::Symbol>('1')),
                        std::make_unique<regexp::Symbol>('1')),
                    std::make_unique<regexp::Concatenation>(
                        std::make_unique<regexp::Concatenation>(
                            std::make_unique<regexp::Symbol>('1'),
                            std::make_unique<regexp::Iteration>(
                                std::make_unique<regexp::Symbol>('0'))),
                        std::make_unique<regexp::Symbol>('1'))),
                std::make_unique<regexp::Symbol>('0'))));
    assert(wordsMatch(re3, {Word{'0', '1'}}) == std::set<size_t>{});
    assert(wordsMatch(re3, {Word{'0', '1', '1'}}) == std::set<size_t>{0});
    assert(wordsMatch(re3, {Word{'0', '1', '1', '0'}}) == std::set<size_t>{0});
    assert(wordsMatch(re3, {Word{'0', '1', '1', '0', '1', '1', '1', '0', '0', '0'}}) == std::set<size_t>{});
    assert(wordsMatch(re3, {Word{'0', '1', '1', '0', '1', '1', '1', '0', '0', '1'}}) == std::set<size_t>{0});
    assert(wordsMatch(re3, {Word{'0', '1', '1', '0', '1', '1', '1', '0', '0', '1', '0'}}) == std::set<size_t>{0});
    assert((wordsMatch(re3, {Word{'0', '1'}, Word{'0', '1', '1'}, Word{'0', '1', '1', '0'}, Word{'0', '1', '1', '0', '1', '1', '1', '0', '0', '0'}, Word{'0', '1', '1', '0', '1', '1', '1', '0', '0', '1'}, Word{'0', '1', '1', '0', '1', '1', '1', '0', '0', '1', '0'}}) == std::set<size_t>{1, 2, 4, 5}));
}
#endif
