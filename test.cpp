#include <stdio.h>
#include <vector>
#include <map>

using namespace std;

struct structura {
    int n;
    vector<int> v;
};

void func2(int &x) {
    x++;
}

// void func(map<int, struct structura *> &m) {
//     auto x = m.find(6);
//     if (x != m.end()) 
//         m[6].n++;
//     structuramea->n++;
//     structuramea->v.push_back(50);
// }

int 

int main() {
    // map<int, struct structura *> m;
    // struct structura structuramea;
    // structuramea.n = 0;
    // structuramea.v.push_back(20);
    // m.insert({6, &structuramea});
    // func(m);
    // auto x = m.find(6);
    // struct structura st = *(x->second);
    // printf("%d\n", st.n);
    // for (int i : st.v) {
    //     printf("%d\n", i);
    // }
    int x = 1;
    func2(x);
    printf("%d\n", x);
    return 0;
}