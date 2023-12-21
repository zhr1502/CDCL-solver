#include <cdcl/heuristic.hpp>
#include <iostream>

using namespace std;

void Heap::debug()
{
    for (int i = 1; i <= siz; i++) cout << activity[i] << " ";
    cout << endl;
    for (int i = 1; i <= siz; i++) cout << pos_in_heap[i] << " ";
    cout << endl;
    for (auto v : storage) cout << v << " ";
    cout << endl << bump << endl;
}

int main()
{
    Heap h(100);
    int n;
    cin >> n;
    for (int i = 1; i <= n; i++)
    {
        int opt;
        cin >> opt;
        if (opt == 1) cout << h.top() << endl;
        if (opt == 2) h.pop();
        if (opt == 3)
        {
            int v;
            cin >> v;
            h.insert(v);
        }
        if (opt == 4)
        {
            h.decay();
        }
        if (opt == 5)
        {
            int v;
            cin >> v;
            h.bump_var(v);
        }
        if (opt == 6)
        {
            h.debug();
        }
    }
}
