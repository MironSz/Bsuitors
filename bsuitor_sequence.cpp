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

using namespace std;

using kraw = tuple<int, int, int>;///koszt, from,to

struct cmp_set {
    bool operator()(kraw k1, kraw k2) {
        return true;
    }

};

int destin(int from, kraw k) {
    if (get<0>(k) == from) {
        return get<1>(k);
    }
    return get<0>(k);
}

int price(kraw k) {
    return get<2>(k);
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
    if (S[destin(from, k)].size() == b[destin(from, k)]) {
        auto deleted = *(--S[destin(from, k)].end());
        if (in_Q.find(destin(destin(from, k), deleted)) == in_Q.end()) {
            Q.push_back(destin(destin(from, k), deleted));
            in_Q.insert(destin(destin(from, k), deleted));
        }
        S[destin(from, k)].erase(deleted);
        T[destin(destin(from, k), deleted)].erase(k);
    }

    T[from].insert(k);
    S[destin(from, k)].insert(k);
}

kraw last(int u) {
    if (S[u].size() < b[u]) {
        return make_tuple(0, INT32_MAX, INT32_MAX);
    }
    return *(--S[u].end());
}

bool check_match(int from, kraw &k) {
    kraw v = last(destin(from, k));
    if (v < k) {
        return true;
    } else {
        return false;
    }
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
                    cout<< "DODALEM " <<
                }
                if (already_added.find(get<1>(k)) == already_added.end()) {
                    already_added.insert(get<1>(k));
                    Q.push_back(get<1>(k));
                }
            }
        }
    }

    while (Q.empty() == false) {
        int u = Q.front();
        Q.pop_front();
        int i = 0;
        while (T[u].size() < b[u] && i < N[u].size()-1) {
            cout<<" u =" <<u<<endl;
            if (check_match(u, N[u][i])) {
                update(u, N[u][i]);
            }
            i++;
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

    for (auto k : old_id) {
        get<1>(k) = map_old_to_new[get<1>(k)];
        get<2>(k) = map_old_to_new[get<2>(k)];
    }

    return new_id;
}

void create_graph(char *file) {
    ifstream input_file("test1.txt");
    string bufor;
    vector <tuple<int, int, int>> readed;
    vector<int> existing_nodes;
    int a, b1, c;
    cout << "Czy otworzylem plik?\n";
    if (input_file.is_open()) {
        cout << "TAK!\n";
        while (getline(input_file, bufor)) {
            cout << "Wczytalem linie: (" << bufor << ")\n";
            if (bufor[0] != '#') {
                sscanf(&(bufor[0]), "%d%d%d", &a, &b1, &c);
                cout << "wczytalem a,b,c " << a << b1 << c << "\n";
                readed.push_back(make_tuple(a, b1, c));
            }
        }
        cout << "Wczytalem linie: (" << bufor << ")\n";
    }

    n = skaluj_krawedzie(readed, old_id_to_new, new_id_to_old);
    cout<< "n = " <<n<<endl;
    b.resize(n+1);
    for (int i = 0; i < n; i++) {
        b[i]=bvalue(method, new_id_to_old[i]);
        cout<<"kolejne b: "<<b.back()<<"  "<<i <<"\n";
    }

    N.resize(n+1);
    T.resize(n+1);
    S.resize(n+1);
    for (auto k : readed) {
        N[get<1>(k)].push_back(k);
        N[get<2>(k)].push_back(k);
    }
}

void wypisz_adorowanych() {
    cout << "wierzchoki i adorowani:\n";
    for (int i = 0; i < n; i++) {
        cout << new_id_to_old[i] << " ("<< b[i]<<"  "<<new_id_to_old[i]<<")"<< " adoruje : ";
        for (auto k : N[i]) {
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

    create_graph(&(input_filename[0]));

    match_sequence();
    wypisz_adorowanych();

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