#include "basetype.hpp"
#include <cmath>
#include <vector>

class Heap
{
#define none_pos -1
    std::vector<double> activity;
    std::vector<int> pos_in_heap;
    std::vector<Variable> storage;
    double dec = 0.95, bump = 1.0;
    double factor;
    int siz;
    double MAX_SCALE = std::pow(10, 50);

    void rescale();
    void shift_down(int);
    void shift_up(int);

public:
    Heap(int);
    int top();
    void pop();
    void insert(Variable);
    void decay();
    void bump_var(Variable);
    bool is_empty();
    bool is_in_heap(Variable);
    void debug();
};
