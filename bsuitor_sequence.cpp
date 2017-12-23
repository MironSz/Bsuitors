#include<iostream>
#include<fstream>
#include<thread>
#include <vector>
#include <mutex>
#include <set>
#include <list>
#include <algorithm>
#include "blimit.cpp"
#include<map>


#define DR if(0)
using namespace std;

using kraw = tuple<int, int, int, int>;///koszt, from,to, id

bool comp(kraw k1, kraw k2) {
    if (get<0>(k1) != get<0>(k2)) {
        return (get<0>(k1) > get<0>(k2));
    }
    if (get<1>(k1) != get<1>(k2)) {
        return (get<1>(k1) > get<1>(k2));
    }
    if (get<2>(k1) != get<2>(k2)) {
        return (get<2>(k1) > get<2>(k2));
    }
    return (get<3>(k1) > get<3>(k2));
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

int price(kraw k) {
    return get<0>(k);
}

vector <vector<kraw>> N;//pososrtowane krawędzie po koszcie
vector<int> b;

vector <set<kraw>> S;//ci ktorzy mi sie oświadczyli
vector <set<kraw>> T;//ci którym się oświadczyłem
list<int> Q;//Q
set<int> in_Q;
int n, method;

map<int, int> old_id_to_new;
map<int, int> new_id_to_old;

void update(int from, kraw &k) {
    DR cout << "UPDATE, ilosc adorujacych cel: " << S[destin(from, k)].size() << endl;
    if (S[destin(from, k)].size() == b[destin(from, k)]) {
        auto deleted = *(S[destin(from, k)].begin());
        DR cout << destin(destin(from, k), deleted) << " przestaje adorowac " << destin(from, k) << "  ," << from
             << " zaczyna\n";
        if (in_Q.find(destin(destin(from, k), deleted)) == in_Q.end() || true) {
            Q.push_back(destin(destin(from, k), deleted));
            in_Q.insert(destin(destin(from, k), deleted));
        }
        S[destin(from, k)].erase(deleted);
        T[destin(destin(from, k), deleted)].erase(deleted);
    }

    T[from].insert(k);
    S[destin(from, k)].insert(k);
}

kraw last(int u) {
    if (S[u].size() < b[u]) {
        return make_tuple(0, INT32_MAX, INT32_MAX, INT32_MAX);
    }
    return *(S[u].begin());
}

bool check_match(int from, kraw &k) {
    kraw v = last(destin(from, k));
    DR cout << "      sprawdzam match " << from << " z " << destin(from, k) << " last:" << price(v) << endl;
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
    for (auto k : N) {
        sort(k.begin(), k.end());
    }

    {//To tworzy kolejkę Q
        set<int> already_added;
        for (int i = 0; i < N.size(); i++) {
            for (kraw k:N[i]) {
                if (already_added.find(get<2>(k)) == already_added.end()) {
                    already_added.insert(get<2>(k));
                    Q.push_back(get<2>(k));
                    DR cout << "DODALEM " << get<2>(k) << endl;
                }
                if (already_added.find(get<1>(k)) == already_added.end()) {
                    already_added.insert(get<1>(k));
                    Q.push_back(get<1>(k));
                    DR cout << "DODALEM " << get<1>(k) << endl;

                }
            }
        }
    }

    while (Q.empty() == false) {
        int u = Q.front();
        Q.pop_front();
        int i = 0;//TODO
        DR cout << "Bede probowal przerobic : " << u << "(" << T[u].size() << ")" << endl;
        while (T[u].size() < b[u] && i < N[u].size()) {
            DR cout << " Przerabiam " << u << " sprawdzam czy moggo sparowac z " << destin(u, N[u][i])
                 << " ilosc sasiadow = " << N[u].size() << endl;
            if (check_match(u, N[u][i])) {
                update(u, N[u][i]);
            }
            i++;
        }
        DR cout << "zawartosc kolejki : ";
        DR for (auto a : Q) {
            cout << a << " ";
        }
//        in_Q.erase(in_Q.find(u));
        DR cout << endl << endl;
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

    for (auto k : old_id) {
        get<1>(k) = map_old_to_new[get<1>(k)];
        get<2>(k) = map_old_to_new[get<2>(k)];
    }

    return new_id;
}

void create_graph(char *file, int b_limit) {
    ifstream input_file("test1.txt");
    string bufor;
    vector <kraw> readed;
    vector<int> existing_nodes;
    int a, b1, c, d = 0;
    DR cout << "Czy otworzylem plik?\n";
    if (input_file.is_open()) {
        DR cout << "TAK!\n";
        while (getline(input_file, bufor)) {
            DR cout << "Wczytalem linie: (" << bufor << ")\n";
            if (bufor[0] != '#') {
                sscanf(&(bufor[0]), "%d%d%d", &a, &b1, &c);
                DR cout << "wczytalem a,b,c " << a << b1 << c << "\n";
                readed.push_back(make_tuple(c, a, b1, d++));
                DR cout << "d: " << d << endl;
            }
        }
        DR cout << "Wczytalem linie: (" << bufor << ")\n";
    }

    n = skaluj_krawedzie(readed, old_id_to_new, new_id_to_old);
    DR cout << "n = " << n << endl;
    b.resize(n + 1);
    for (int i = 0; i < n; i++) {
        b[i] = bvalue(method, new_id_to_old[i]);
       DR  cout << "kolejne b: " << bvalue(method, new_id_to_old[i]) << "  " << i << "\n";
    }

    N.resize(n + 1);
    T.resize(n + 1);
    S.resize(n + 1);

    for (auto k : readed) {
        N[get<1>(k)].push_back(k);
        N[get<2>(k)].push_back(k);
    }

    for (auto &k : N) {
        sort(k.begin(), k.end(), comp);
        for (auto k2 : k) {
           DR  cout << get<0>(k2) << "  ";
        }
        DR cout << endl;
    }
}

void clear_graph(){
    for(auto a:S){
        a.clear();
    }
    for(auto a:T){
        a.clear();
    }
    in_Q.clear();
}
void generate_b() {
    for (int i = 0; i < n; i++) {
        b[i] = bvalue(method, new_id_to_old[i]);
        DR cout << "kolejne b: " << bvalue(method, new_id_to_old[i]) << "  " << i <<"  "<<method<< "\n";
    }
}

void wypisz_adorowanych() {
    cout << "wierzchoki i adorowani:\n";
    for (int i = 0; i < n; i++) {
        cout << new_id_to_old[i] << " (" << b[i] << "  " << new_id_to_old[i] << ")" << " jest adorowana przez : ";
        for (auto k : S[i]) {
            cout << new_id_to_old[destin(i, k)] << " ";
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
        cout << endl << count_result()<<endl;
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