#include "cdcl/heuristic.hpp"
#include <algorithm>
#include "cdcl/basetype.hpp"

Heap::Heap(int size) : siz(size)
{
    activity.resize(siz + 1);
    storage.push_back(none_pos);
    pos_in_heap.push_back(none_pos);
    for (int i = 1; i <= siz; i++)
        storage.push_back(i), pos_in_heap.push_back(i);
    factor = 1 / dec;
}

bool Heap::is_empty() { return storage.size() == 1; }

int Heap::top()
{
    if (is_empty()) return none_pos;
    return storage[1];
}

void Heap::pop()
{
    if (is_empty()) return;
    if (storage.size() == 2)
    {
        pos_in_heap.at(storage[1]) = none_pos;
        storage.pop_back();
        return;
    }
    pos_in_heap.at(storage[1]) = none_pos;
    storage[1] = storage.back();
    storage.pop_back();
    pos_in_heap.at(storage[1]) = 1;
    shift_down(1);
}

void Heap::insert(Variable v)
{
    if (is_in_heap(v)) return;
    storage.push_back(v);
    pos_in_heap.at(storage.back()) = storage.size() - 1;
    shift_up(storage.size() - 1);
}

void Heap::decay()
{
    bump *= factor;
    if (bump >= MAX_SCALE) rescale();
}

void Heap::bump_var(Variable v)
{
    auto pos = pos_in_heap.at(v);
    activity.at(v) += bump;
    if (activity.at(v) >= MAX_SCALE) rescale();
    if (pos != none_pos) shift_up(pos);
}

void Heap::rescale()
{
    for (int i = 1; i <= siz; i++) activity[i] /= MAX_SCALE;

    bump /= MAX_SCALE;
    return;
}

bool Heap::is_in_heap(Variable v) { return pos_in_heap[v] != none_pos; }

void Heap::shift_up(int pos)
{
    int fa = pos / 2;
    while (fa > 0 && activity[storage[fa]] < activity[storage[pos]])
    {
        pos_in_heap[storage[fa]] = pos;
        pos_in_heap[storage[pos]] = fa;
        std::swap(storage[fa], storage[pos]);
        pos = fa;
        fa = pos / 2;
    }
}

void Heap::shift_down(int pos)
{
    int lchd = pos * 2, rchd = pos * 2 + 1;
    int current_siz = storage.size() - 1;
    while (lchd <= current_siz &&
               activity[storage[lchd]] > activity[storage[pos]] ||
           rchd <= current_siz &&
               activity[storage[rchd]] > activity[storage[pos]])
    {
        int max_child = lchd;
        if (rchd <= current_siz &&
            activity[storage[rchd]] >= activity[storage[lchd]])
            max_child = rchd;

        pos_in_heap[storage[max_child]] = pos;
        pos_in_heap[storage[pos]] = max_child;

        std::swap(storage[max_child], storage[pos]);
        pos = max_child;
        lchd = pos * 2;
        rchd = pos * 2 + 1;
    }
}
