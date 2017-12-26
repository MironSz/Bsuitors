#include<iostream>
#include<fstream>
#include<thread>
#include <vector>
#include <mutex>
#include <set>
#include <list>
#include <algorithm>
#include "blimit2.cpp"
#include <cassert>
#include<map>

#define DR if(0)
using namespace std;

using kraw = tuple<int, int, int, int>;///koszt, from,to, id
using kraw_lite = tuple<int, int, int>;///koszt,to, id

kraw_lite make_lite(int from, kraw k) {
    if (get<1>(k) == from) {
        return make_tuple(get<0>(k), get<2>(k), get<3>(k));
    }
    return make_tuple(get<0>(k), get<1>(k), get<3>(k));
}

kraw_lite make_lite(int from, kraw_lite k) {
    get<1>(k) = from;
    return k;
}

int destin(int from, kraw k) {
    if (get<2>(k) == from) {
        return get<1>(k);
    }
    return get<2>(k);
}

int destin(kraw_lite k) {
    return get<1>(k);
}

int price(kraw k) {
    return get<0>(k);
}

int price(kraw_lite k) {
    return get<0>(k);
}

map<int, int> old_id_to_new;
map<int, int> new_id_to_old;

bool comp(kraw_lite k1, kraw_lite k2) {
    if (get<0>(k1) != get<0>(k2)) {
        return (get<0>(k1) > get<0>(k2));
    }
    if (get<1>(k1) != get<1>(k2)) {
        return new_id_to_old[get<1>(k1)] > new_id_to_old[get<1>(k2)];
    }
    return false;
}

struct cmp_set {
    bool operator()(const kraw_lite k1, const kraw_lite k2) const {
        return comp(k1, k2);
    }
};

vector <vector<kraw_lite>> N;//pososrtowane krawędzie po koszcie//TODO lite

vector<int> b;
vector <set<kraw_lite, cmp_set>> S;//ci ktorzy mi sie oświadczyli
vector<int> T;//ci którym się oświadczyłem, sama ich liczba
list<int> Q;//Q
set<int> in_Q;
int n, method;
vector<int> it;

void update(int from, kraw_lite &k) {
    assert(from != destin(k));
    if (S[destin(k)].size() == b[destin(k)]) {
        auto deleted = *(--S[destin(k)].end());
        int lost_adorator = destin(deleted);
        if (in_Q.find(lost_adorator) == in_Q.end()) {//ten który przestaje adorować
            in_Q.insert(lost_adorator);
        }
        S[destin(k)].erase(deleted);
        Q.push_back(lost_adorator);
        if (get<2>(k) > -1) {
            T[lost_adorator]--;
        }
    }
    T[from]++;
    S[destin(k)].insert(make_lite(from, k));
}

kraw_lite last(int u) {
    if (S[u].size() < b[u]) {
        return make_tuple(-1, -1, -1);
    }
    return *(--S[u].end());
}

bool check_match(int from, kraw_lite &k) {
    if (b[destin(k)] == 0) {
        return false;
    }
    kraw_lite v = last(destin(k));
    return comp(make_lite(from, k), v);
}

int count_result() {
    int result = 0;
    for (auto k :S) {
        for (auto k2:k) {
            result += price(k2);
        }
    }
    assert(result % 2 == 0);
    return result / 2;
}

void match_sequence() {
    {//To tworzy kolejkę Q
        set<int> already_added;
        for (int i = 0; i < N.size(); i++) {
            for (kraw_lite k:N[i]) {
                if (already_added.find(get<1>(k)) == already_added.end() && b[get<1>(k)]) {
                    already_added.insert(get<1>(k));
                    Q.push_back(get<1>(k));
                }
            }
        }
        in_Q = already_added;
    }

    while (Q.empty() == false) {
        int u = Q.front();
        Q.pop_front();
        while (T[u] < b[u] && it[u] < N[u].size()) {
            if (check_match(u, N[u][it[u]])) {
                update(u, N[u][it[u]]);
            }
            it[u]++;
        }
        if (in_Q.find(u) != in_Q.end()) {
            in_Q.erase(in_Q.find(u));
        }
    }
}

int skaluj_krawedzie(vector <kraw> &old_id, map<int, int> &map_old_to_new, map<int, int> &map_new_to_old) {
    int prev = -1;
    int new_id = 0;
    vector<int> existing_nodes;

    for (auto id : old_id) {
        existing_nodes.push_back(get<1>(id));
        existing_nodes.push_back(get<2>(id));
    }

    sort(existing_nodes.begin(), existing_nodes.end());//Pararel

    for (int id : existing_nodes) {
        if (id != prev) {
            map_old_to_new.insert(make_pair(id, new_id));
            map_new_to_old.insert(make_pair(new_id, id));
            new_id++;
            prev = id;
        }
    }

    for (auto &k : old_id) {
        get<1>(k) = map_old_to_new[get<1>(k)];
        get<2>(k) = map_old_to_new[get<2>(k)];
    }

    return new_id;
}

void generate_b() {
    for (int i = 0; i < n; i++) {
        b[i] = bvalue(method, new_id_to_old[i]);
    }
}

void create_graph(char *file, int b_limit) {
    ifstream input_file(file);
    string bufor;
    vector <kraw> readed;
    vector<int> existing_nodes;
    int a, b1, c, d = 0;
    if (input_file.is_open()) {
        while (getline(input_file, bufor)) {

            if (bufor[0] != '#') {
                sscanf(&(bufor[0]), "%d%d%d", &a, &b1, &c);
                readed.push_back(make_tuple(c, a, b1, d++));
            }
        }
    }

    n = skaluj_krawedzie(readed, old_id_to_new, new_id_to_old);
    b.resize(n);

    N.resize(n);
    T.resize(n);
    S.resize(n);
    it.resize(n);
    for (auto k : readed) {
        if (price(k)) {
            N[get<1>(k)].push_back(make_lite(get<1>(k), k));
            if (get<1>(k) != get<2>(k)) {
                N[get<2>(k)].push_back(make_lite(get<2>(k), k));
            }
        }
    }

    for (auto &k : N) {
        sort(k.begin(), k.end(), comp);
    }
}

void clear_graph() {
    for (auto &a:S) {
        a.clear();
    }
    for (auto &a:T) {
        a = 0;
    }
    for (auto &a:it) {
        a = 0;
    }
    for (auto &a:b) {
        a = 0;
    }
    in_Q.clear();
    Q.clear();
}

void wypisz_adorowanych() {
    cout << "wierzchoki i adorowani:\n";
    for (int i = 0; i < n; i++) {
        cout << new_id_to_old[i] << " (" << b[i] << "  " << new_id_to_old[i] << ")" << " jest adorowana przez : ";
        for (auto k : S[i]) {
            cout << new_id_to_old[destin(k)] << " ";
        }
        cout << "\n";
    }
}

void sprawdz_poprawnosc() {
    int missing = 0;
    for (int i = 0; i < n; i++) {
        for (auto k : S[i]) {
            missing += (S[destin(k)].find(make_lite(i, k)) == S[destin(k)].end());
            if ((S[destin(k)].find(make_lite(i, k)) == S[destin(k)].end())) {
                cout << "  from " << i << " to " << destin(k) << " price " << price(k) << " (" <<
                     b[i] << "," << b[destin(k)] << ") ->(" << T[i] << "," << T[destin(k)] << endl;
            }
        }
    }
    cout << "Missing: " << missing << endl;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "usage: " << argv[0] << " thread-count inputfile b-limit" << std::endl;
        return 1;
    }

    int thread_count = std::stoi(argv[1]);
    int b_limit = std::stoi(argv[3]);
    std::string input_filename{argv[2]};
    method = 0;
    create_graph(&(input_filename[0]), b_limit);
    for (method = 0; method <= b_limit; method++) {
        generate_b();
        match_sequence();
        cout << endl << count_result() << endl;
        sprawdz_poprawnosc();
        clear_graph();
    }
//    wypisz_adorowanych();
}

/*
 * FLAGSHIPY!!!!
 * Promisionary notes
 * Czołgi
 * Rasowe technologie
 * JEszcze sie zastanowimy na ktore strategie gramy
 * Nie gramy na senackich przedstawicieli
 * Usable planets
 * Preliminary objective(te pocątkowe secrety)
 * Secret i preliminary nie liczą się do limitu claimów na turę
 * Gramy na nagły koniec?
 * Zaczynamy z trzema wyłożonymi I stage objective
 */