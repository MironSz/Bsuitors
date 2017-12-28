//TODO random_shuffle wierzchołków, to może być bardzo ważne
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
#include <condition_variable>

#define DR if(0)
using namespace std;

using kraw = tuple<int, int, int, int>;///koszt, from,to, id
using kraw_lite = tuple<int, int, int>;///koszt,to, id
int method_in_thread[100];

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
        return get<1>(k1) > get<1>(k2);
    }
    return false;
}

struct cmp_set {
    bool operator()(const kraw_lite k1, const kraw_lite k2) const {
        return comp(k1, k2);
    }
};

int n_threads;
condition_variable *bariery;
mutex *ochrona_bariery;
vector<int> ile_czeka;
vector<int> result_in_thread;

void czekaj_na_pozostale(int ktora_bariera) {
    unique_lock<mutex> lk(ochrona_bariery[ktora_bariera]);
    ile_czeka[ktora_bariera]++;

    if (ile_czeka[ktora_bariera] != n_threads) {
        bariery[ktora_bariera].wait(lk);
    } else {
        bariery[ktora_bariera].notify_all();
    }
}

vector<vector<kraw_lite>> N;//pososrtowane krawędzie po koszcie//TODO lite

vector<int> b;
mutex *ochrona1;
mutex *ochrona2;
int * last_in_node_destin;
int * last_in_node_koszt;
vector<set<kraw_lite, cmp_set>> S;//ci ktorzy mi sie oświadczyli
vector<int> T;//ci którym się oświadczyłem, sama ich liczba
list<int> Q;//Q
vector<list < int>>
Q_in_thread;
vector<set<int>> in_Q_in_thread;
mutex ochrona_Q;
set<int> in_Q;
int n, method;
int b_limit;
vector<int> it;


pair<int, int> range(int thread_id) {
//    cout << " thread id: " << thread_id << " threads: " << n_threads << endl;
    int skok = n / n_threads;
    int from = skok * thread_id;
    int to = from + skok;
    if (thread_id == n_threads - 1) {
//        cout << " ostatni watek\n";
        to = n;
    }
//    cout << "range : " << from << "  " << to << endl;
    return {from, to};
}

kraw_lite last(int u) {
    if (S[u].size() < b[u]) {
        return make_tuple(-1, -1, -1);
    }
    return *(--S[u].end());
}

void add_to_Q(int lost_adorator, int thread_id, int dodano){
    Q_in_thread[thread_id].push_back(lost_adorator);
}

void update(int from, kraw_lite &k, int thread_id, int dodano) {
    if (S[destin(k)].size() == b[destin(k)]) {
        auto deleted = *(--S[destin(k)].end());
        int lost_adorator = destin(deleted);
//        if (in_Q.find(lost_adorator) == in_Q.end()) {//ten który przestaje adorować
//            in_Q.insert(lost_adorator);
//        }
        S[destin(k)].erase(deleted);
        add_to_Q(lost_adorator,thread_id,dodano);
        if (get<2>(k) > -1) {
            T[lost_adorator]--;
        }
    }
    T[from]++;
   DR printf("    thread %d %d bedzie adorowal %d\n" ,thread_id,from,destin(k));
    S[destin(k)].insert(make_lite(from, k));
    if(destin(k) >= 0){
        last_in_node_koszt[destin(k)] = price(last(destin(k)));
        last_in_node_destin[destin(k)] = destin(last(destin(k)));
    }
}

void count_result(int thread_id) {
    int result = 0;
    auto r = range(thread_id);
    for (int i = r.first; i < r.second; i++) {
        for (auto k:S[i]) {
            result += price(k);
        }
    }
    result_in_thread[thread_id] = result;
}

bool check_match(int from, kraw_lite &k, bool is_locked) {
    DR printf("    %d sprawdza czy moze adorowac %d kosztem %d\n" ,from,destin(k),price(k));
    if (b[destin(k)] == 0) {
        return false;
    }
    kraw_lite v;
    if(is_locked)
         v = last(destin(k));
    else
        v = {last_in_node_koszt[destin(k)],last_in_node_destin[destin(k)],-1};

    return comp(make_lite(from, k), v);
}


bool take_from_Q(int thread_id, int & u){
    u= Q_in_thread[thread_id].front();
    Q_in_thread[thread_id].pop_front();
    return true;

}
void match(int thread_id) {
    while (Q_in_thread[thread_id].empty() == false) {
        int u;
        take_from_Q(thread_id,u);
        lock_guard<mutex> lck(ochrona1[u]);
        DR printf("thread %d jest w %d (method %d)\n" ,thread_id,u,method_in_thread[thread_id]);
        DR cout <<"Probuje adorowac "<<u<<endl;
        while (T[u] < b[u] && it[u] < N[u].size()) {
            if (check_match(u, N[u][it[u]], 0)) {
                lock_guard<mutex> lck2(ochrona2[destin(N[u][it[u]])]);
                DR printf("   thread %d zaczyna przerabiac %d (method %d)\n" ,thread_id,destin(N[u][it[u]]),method_in_thread[thread_id]);
                if (check_match(u, N[u][it[u]],1)) {
                    update(u, N[u][it[u]], thread_id, 0);
                }
                DR printf("   thread %d skonczyl przerabiac  %d method(%d)\n" ,thread_id,destin(N[u][it[u]]),method_in_thread[thread_id]);
            }
            it[u]++;
        }
    }
}

int skaluj_krawedzie(vector<kraw> &old_id, map<int, int> &map_old_to_new, map<int, int> &map_new_to_old) {
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

void generate_b(int thread_id) {
    auto r = range(thread_id);
    for (int i = r.first; i < r.second; i++) {
        b[i] = bvalue(method, new_id_to_old[i]);
    }
}

void create_graph(char *file) {
    ifstream input_file(file);
    string bufor;
    vector<kraw> readed;
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
    if (n < n_threads)
        n_threads = n;

    last_in_node_destin = new int[n];
    last_in_node_koszt = new int[n];
    b.resize(n);
    N.resize(n);
    T.resize(n);
    S.resize(n);
    it.resize(n);
    ochrona1 = new mutex[n];
    ochrona2 = new mutex[n];
    bariery = new condition_variable[(b_limit+1) * 10];
    ile_czeka.resize(b_limit * 10);
    result_in_thread.resize(n_threads);

    for (auto &a:ile_czeka)
        a = 0;
    ochrona_bariery = new mutex[(b_limit+1) * 10];

    Q_in_thread.resize(n_threads);
    in_Q_in_thread.resize(n_threads);

    method = 0;


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


void clear_graph(int thread_id) {
    auto r = range(thread_id);
    for (int i = r.first; i < r.second; i++) {
        assert(Q_in_thread[thread_id].size() == 0);
        S[i].clear();
        T[i] = 0;
        it[i] = 0;
        b[i] = 0;
        last_in_node_destin[i] = -1;
        last_in_node_koszt[i] = -1;
    }
    if (thread_id == 0) {
        in_Q.clear();
        Q.clear();
    }
    result_in_thread[thread_id] = 0;
}

void prepare_Q(int thread_id) {
    //dodaj wszyskie z range do Q_in_thread
    auto r = range(thread_id);
    for (int i = r.first; i < r.second; i++) {
        if (b[i]){
            Q_in_thread[thread_id].push_back(i);
        }
    }

}

void wypisz_Q(int thread_id){
    cout <<"W kolejce "<<thread_id<<"\n";
    auto r = range(thread_id);
    for(int a: Q_in_thread[thread_id]){
        cout<<a<<" ";
    }
}

void wypisz_wynik() {
    int result = 0;
    for (auto &a :result_in_thread) {
        result += a;
        a = 0;
    }
    cout << result/2 << endl;
}

void f(int thread_id) {
    int ktora_bariera = 0;
    while (method <= b_limit) {
//        DR cout <<"Startuje petle dla "<<thread_id<<endl;
        clear_graph(thread_id);
        generate_b(thread_id);
        prepare_Q(thread_id);
        czekaj_na_pozostale(++ktora_bariera);
        DR printf(" minal bariere %d\n" ,thread_id);
        method_in_thread[thread_id]++;
        match(thread_id);
        printf("watek %d skonczyl matchowanie method = %d\n" ,thread_id, method);
        czekaj_na_pozostale(++ktora_bariera);

        count_result(thread_id);
        czekaj_na_pozostale(++ktora_bariera);

        if (thread_id == 0) {
            wypisz_wynik();
            method++;
        }

        czekaj_na_pozostale(++ktora_bariera);
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

    n_threads = std::stoi(argv[1]);
    b_limit = std::stoi(argv[3]);
    std::string input_filename{argv[2]};
    method = 0;

    create_graph(&(input_filename[0]));
    thread myThread[n_threads];

    for(int i=1;i< n_threads;i++)
        myThread[i]= thread(f,i);
    f(0);
    for(int i=1;i< n_threads;i++)
        myThread[i].join();
   // wypisz_adorowanych();
    delete[] bariery;
    delete[] ochrona1;
    delete[] ochrona2;

}
