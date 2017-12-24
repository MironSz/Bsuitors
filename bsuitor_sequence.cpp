#include<iostream>
#include<fstream>
#include<thread>
#include <vector>
#include <mutex>
#include <set>
#include <list>
#include <algorithm>
#include "blimit2.cpp"
#include<map>

#define DR if(0)
using namespace std;

using kraw = tuple<int, int, int, int>;///koszt, from,to, id
using kraw_lite = tuple<int, int, int>;///koszt,to, id

kraw_lite make_lite(int from, kraw k){
    if(get<1>(k) == from){
        return make_tuple(get<0>(k),get<2>(k),get<3>(k));
    }
    return make_tuple(get<0>(k),get<1>(k),get<3>(k));
}
kraw_lite make_lite(int from, kraw_lite k){
    get<1>(k)=from;
    return k;
}

bool comp(kraw k1, kraw k2) {
    if (get<0>(k1) != get<0>(k2)) {
        return (get<0>(k1) > get<0>(k2));
    }
    if (get<1>(k1) != get<1>(k2)) {
        return (get<1>(k1) < get<1>(k2));
    }
    if (get<2>(k1) != get<2>(k2)) {
        return (get<2>(k1) < get<2>(k2));
    }
    return (get<3>(k1) > get<3>(k2));
}
bool comp2(kraw_lite k1, kraw_lite k2) {
    if (get<0>(k1) != get<0>(k2)) {
        return (get<0>(k1) > get<0>(k2));
    }
    if (get<1>(k1) != get<1>(k2)) {
        return (get<1>(k1) > get<1>(k2));
    }
    return (get<2>(k1) > get<2>(k2));
}

struct cmp_set {
    bool operator()(kraw k1, kraw k2) {
        return comp(k1, k2);
    }

};

int destin(int from, kraw k) {
    if (get<2>(k) == from) {
        return get<1>(k);
    }
    return get<2>(k);
}
int destin( kraw_lite k) {
    return get<1>(k);
}

int price(kraw k) {
    return get<0>(k);
}
int price(kraw_lite k) {
    return get<0>(k);
}

vector <vector<kraw_lite>> N;//pososrtowane krawędzie po koszcie//TODO lite
vector<int> b;

vector <set<kraw_lite>> S;//ci ktorzy mi sie oświadczyli
vector <int> T;//ci którym się oświadczyłem, sama ich liczba
list<int> Q;//Q
set<int> in_Q;
int n, method;
vector<int> it;
map<int, int> old_id_to_new;
map<int, int> new_id_to_old;

void update(int from, kraw_lite &k) {
    DR { cout << "UPDATE, ilosc adorujacych cel: " << S[destin(k)].size() << endl; }
    if (S[destin(k)].size() == b[destin(k)]) {
        auto deleted = *(S[destin(k)].begin());
        int lost_adorator = destin(deleted);
        DR {
            cout << destin(deleted) << " przestaje adorowac " << destin(k) << "  ," << from
                 << " zaczyna\n";
        }
        if (in_Q.find(lost_adorator) == in_Q.end()) {//ten który przestaje adorować
            Q.push_back(lost_adorator);
            in_Q.insert(lost_adorator);
        }
        S[destin(k)].erase(deleted);
        T[lost_adorator]--;
    }
    T[from]++;
    S[destin(k)].insert(make_lite(from,k));
    DR { cout << "Updated\n"; }
}

kraw_lite last(int u) {
    if (S[u].size() < b[u]) {
        return make_tuple(0, INT32_MAX, INT32_MAX);
    }
    return *(S[u].begin());
}

bool check_match(int from, kraw_lite &k) {
    kraw_lite v = last(destin(k));
    DR { cout << "      sprawdzam match " << from << " z " << destin( k) << " last:" << price(v) << endl; }
    if (v < k) {
        return true;
    } else {
        return false;
    }
}

int count_result() {
    int result = 0;
    for (auto k :S) {
        for (auto k2:k) {
            result += price(k2);
        }
    }
    return result / 2;
}

void match_sequence() {
//    for (auto k : N) {
//        sort(k.begin(), k.end());
//    }

    {//To tworzy kolejkę Q
        set<int> already_added;
        for (int i = 0; i < N.size(); i++) {
            for (kraw_lite k:N[i]) {
                if (already_added.find(get<1>(k)) == already_added.end()) {
                    already_added.insert(get<1>(k));
                    Q.push_back(get<1>(k));
                    DR { cout << "DODALEM " << get<1>(k) << endl; }
                }
            }
        }
        in_Q=already_added;
    }

    while (Q.empty() == false) {
        int u = Q.front();
        Q.pop_front();
        DR { cout << "Bede probowal przerobic : " << u << "(" << T[u]<< ")" << endl; }
        while (T[u] < b[u] && it[u] < N[u].size()) {
            DR {
                cout << " Przerabiam " << u << " sprawdzam czy moggo sparowac z " << destin(N[u][it[u]])
                     << " ilosc sasiadow = " << N[u].size() << endl;
            }
            if (check_match(u, N[u][it[u]])) {
                update(u, N[u][it[u]]);
            }
            DR { cout << "indeed updated\n"; }
            it[u]++;
        }
        DR { cout << "zawartosc kolejki : "; }
        DR {
            for (auto a : Q) {
                cout << a << " ";
            }
        }
        DR { cout << endl; }
        if (in_Q.find(u) != in_Q.end()) {
            DR cout <<"Erasing from in_Q\n";
            in_Q.erase(in_Q.find(u));
        }
        DR { cout << endl << endl; }
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
        DR cout <<"("<<get<1>(k)<<","<<get<2>(k)<<")->(;"<<map_old_to_new[get<1>(k)]<<","<<map_old_to_new[get<2>(k)]<<")\n";
        get<1>(k) = map_old_to_new[get<1>(k)];
        get<2>(k) = map_old_to_new[get<2>(k)];
    }

    return new_id;
}

void create_graph(char *file, int b_limit) {
    ifstream input_file(file);
    string bufor;
    vector <kraw> readed;
    vector<int> existing_nodes;
    int a, b1, c, d = 0;
    DR { cout << "Czy otworzylem plik?\n"; }
    if (input_file.is_open()) {
        DR { cout << "TAK!\n"; }
        while (getline(input_file, bufor)) {
            DR { cout << "Wczytalem linie: (" << bufor << ")\n"; }

            if (bufor[0] != '#') {
                sscanf(&(bufor[0]), "%d%d%d", &a, &b1, &c);
                DR { cout << "wczytalem a,b,c " << a << b1 << c << "\n"; }
                readed.push_back(make_tuple(c, a, b1, d++));
            }
        }
    }

    n = skaluj_krawedzie(readed, old_id_to_new, new_id_to_old);
    for (auto k : readed) {
        DR cout << "(" << get<1>(k) << "," << get<2>(k) << ")\n";
    }
    b.resize(n + 1);
    for (int i = 0; i < n; i++) {
        b[i] = bvalue(method, new_id_to_old[i]);
//        DR { cout << "kolejne b: " << bvalue(method, new_id_to_old[i]) << "  " << i << "\n"; }
    }

    DR { cout << "n = " << n << endl; }
    DR cout << "Wczytalem b\n\n\n\n";
    N.resize(n + 1);
    T.resize(n + 1);
    S.resize(n + 1);
    it.resize(n+1);
    for (auto k : readed) {
        DR cout <<"("<<get<1>(k)<<","<<get<2>(k)<<")\n";
        N[get<1>(k)].push_back(make_lite(get<1>(k),k));
        N[get<2>(k)].push_back(make_lite(get<2>(k),k));
    }

    for (auto &k : N) {
        sort(k.begin(), k.end(),comp2);
        for (auto k2 : k) {
            DR { cout << get<0>(k2) << "  "; }
        }
        DR { cout << endl; }
    }
}

void clear_graph() {
    for (auto a:S) {
        a.clear();
    }
    for (auto a:T) {
        a=0;
    }
    for(auto a:it){
        a=0;
    }
    in_Q.clear();
    Q.clear();
}

void generate_b() {
    for (int i = 0; i < n; i++) {
        b[i] = bvalue(method, new_id_to_old[i]);
        DR { cout << "kolejne b: " << bvalue(method, new_id_to_old[i]) << "  " << i << "  " << method << "\n"; }
    }
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