#define EXPOSE_EXCEPTION

#include <sstream>

#include "unit_test.h"
#include "render_queue.h"

using namespace std;
using namespace shrtool;
using namespace shrtool::unit_test;

vector<pair<int, int>> parse_edges(const string& g)
{
    istringstream is(g);
    vector<pair<int, int>> res;
    string tmp;

    is >> tmp >> tmp >> tmp;
    if(tmp != "{")
        throw parse_error("Failed to parse graph: " + tmp);

    while(true) {
        tmp.clear();
        int n1, n2;

        is >> n1;
        if(is.fail()) {
            is.clear();
            is >> tmp;
            if(tmp != "}")
                throw parse_error("Failed to parse graph: " + tmp);
            else break;
        }
        is >> tmp;
        is >> n2;

        if(tmp != "->")
            throw parse_error("Failed to parse graph: " + tmp);

        res.push_back(make_pair(n1, n2));
    }

    return res;
}

TEST_CASE(test_topology_sort) {
    auto g = parse_edges(R"EOF(
    digraph G {
        1 -> 0
        0 -> 5
        2 -> 3
        4 -> 5
        2 -> 4
        3 -> 5
        5 -> 6
     }
     )EOF");

    void_render_task n[7];
    for(auto& p : g)
        n[p.second].rely_on(n[p.first]);

    queue_render_task q;
    for(auto& e : n) q.tasks.push_back(&e);

    q.sort();

    map<const render_task*, size_t> idx;
    for(auto e : q)
        idx[e] = idx.size();

    for(auto& p : g)
        assert_true(idx[n + p.first] < idx[n + p.second]);
}

TEST_CASE(test_topology_sort_failure) {
    auto g = parse_edges(R"EOF(
    digraph G {
        6 -> 0
        0 -> 1
        1 -> 5
        2 -> 3
        4 -> 5
        2 -> 4
        3 -> 5
        5 -> 6
     }
     )EOF");

    void_render_task n[7];
    for(auto& p : g)
        n[p.second].rely_on(n[p.first]);

    queue_render_task q;
    for(auto& e : n) q.tasks.push_back(&e);

    assert_except(q.sort(), resolve_error);
}

int main(int argc, char* argv[])
{
    test_main(argc, argv);
}

