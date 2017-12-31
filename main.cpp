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
#define USE_SHARED_Q false
using namespace std;
using kraw = tuple<unsigned int, unsigned int, unsigned int, unsigned int>;///koszt, from,to, id
using kraw_lite = tuple<unsigned int, unsigned int, unsigned int>;///koszt,to, id
int method_in_thread[100];

kraw_lite make_lite(unsigned int from, kraw k) {
    if (get<1>(k) == from) {
        return make_tuple(get<0>(k), get<2>(k), get<3>(k));
    }
    return make_tuple(get<0>(k), get<1>(k), get<3>(k));
}

kraw_lite make_lite(unsigned int from, kraw_lite k) {
    get<1>(k) = from;
    return k;
}

unsigned int destin(kraw_lite k) {
    return get<1>(k);
}

unsigned int price(kraw k) {
    return get<0>(k);
}

unsigned int price(kraw_lite k) {
    return get<0>(k);
}

map<unsigned int, unsigned int> old_id_to_new;
map<unsigned int, unsigned int> new_id_to_old;

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

unsigned int n_threads;
condition_variable *bariery;
mutex *ochrona_bariery;
vector<unsigned int> ile_czeka;
vector<unsigned int> result_in_thread;

void czekaj_na_pozostale(int ktora_bariera) {
    unique_lock <mutex> lk(ochrona_bariery[ktora_bariera]);
    ile_czeka[ktora_bariera]++;

    if (ile_czeka[ktora_bariera] != n_threads) {
        bariery[ktora_bariera].wait(lk);
    } else {
        bariery[ktora_bariera].notify_all();
    }
}

vector <vector<kraw_lite>> N;//pososrtowane krawędzie po koszcie//TODO lite

vector<unsigned int> b;
mutex *ochrona1;
mutex *ochrona2;
unsigned int *last_in_node_destin;
unsigned int *last_in_node_koszt;
vector <set<kraw_lite, cmp_set>> S;//ci ktorzy mi sie oświadczyli
vector<unsigned int> T;//ci którym się oświadczyłem, sama ich liczba
list<unsigned int> Q;//Wspola kolejka
unsigned int threads_wating_for_node;
unsigned int Q_size;
condition_variable lacking_nodes;
vector <list<unsigned int>> Q_in_thread;
vector <set<unsigned int>> in_Q_in_thread;
mutex ochrona_Q;
set<unsigned int> in_Q;
unsigned int n, method;
unsigned int b_limit;
unsigned int min_cost;
vector<unsigned int> it;

vector<unsigned int> order_of_nodes;

pair<unsigned int, unsigned int> range(unsigned int thread_id) {
    unsigned int skok = n / n_threads;
    unsigned int from = skok * thread_id;
    unsigned int to = from + skok;
    if (thread_id == n_threads - 1) {
        to = n;
    }
    return {from, to};
}

kraw_lite last(unsigned int u) {
    if (S[u].size() < b[u]) {
        return make_tuple(0, 0, 0);
    }
    return *(--S[u].end());
}

void add_to_Q(unsigned int lost_adorator, int thread_id, unsigned int dodano) {
    if (Q_size == 0 && Q_in_thread[thread_id].empty() == false && USE_SHARED_Q) {
        unique_lock <mutex> lck(ochrona_Q);
        Q.push_back(lost_adorator);
        Q_size++;
        if (threads_wating_for_node) {
            lacking_nodes.notify_one();
        }
    } else {
        Q_in_thread[thread_id].push_back(lost_adorator);
    }
}

void update(unsigned int from, kraw_lite &k, int thread_id, unsigned int dodano) {
    if (S[destin(k)].size() == b[destin(k)]) {
        auto deleted = *(--S[destin(k)].end());
        unsigned int lost_adorator = destin(deleted);
        S[destin(k)].erase(deleted);
        add_to_Q(lost_adorator, thread_id, dodano);
    }
    T[from]++;
    DR { printf("    thread %d %d bedzie adorowal %d\n", thread_id, from, destin(k)); }
    S[destin(k)].insert(make_lite(from, k));
    if (destin(k) >= 0) {
        last_in_node_koszt[destin(k)] = price(last(destin(k)));
        last_in_node_destin[destin(k)] = destin(last(destin(k)));
    }
}

void count_result(unsigned int thread_id) {
    unsigned int result = 0;
    auto r = range(thread_id);
    for (unsigned int i = r.first; i < r.second; i++) {
        for (auto k:S[i]) {
            result += price(k);
        }
    }
    result_in_thread[thread_id] = result;
}

bool check_match(unsigned int from, kraw_lite &k, bool is_locked) {
    DR { printf("    %d sprawdza czy moze adorowac %d kosztem %d\n", from, destin(k), price(k)); }
    if (b[destin(k)] == 0) {
        return false;
    }
    kraw_lite v;
    if (is_locked) {
        v = last(destin(k));
    } else {
        v = {last_in_node_koszt[destin(k)], last_in_node_destin[destin(k)],0};
    }

    return comp(make_lite(from, k), v);
}

bool take_from_Q(unsigned int thread_id, unsigned int &u) {
    if (Q_in_thread[thread_id].empty() == false) {
        u = Q_in_thread[thread_id].front();
        Q_in_thread[thread_id].pop_front();
        return true;
    }
    if (USE_SHARED_Q == false) {
        return false;
    }
    unique_lock <mutex> lck(ochrona_Q);
    printf("thread %d wants to take a node from Q\n", thread_id);
    if (Q_size == 0) {
        threads_wating_for_node++;
        if (threads_wating_for_node == n_threads) {
            printf("  all nodes are gone, waking other threads\n");
            lacking_nodes.notify_all();
            return false;
        }
        printf("  waiting for a node\n");
        lacking_nodes.wait(lck);
    }
    printf("thread %d woke up\n", thread_id);
    if (Q_size > 0) {
        printf("  found my node :)\n");
        u = Q.front();
        Q.pop_front();
        Q_size--;
        threads_wating_for_node--;
        return true;
    } else {
        printf("  all nodes are gone\n");
        lacking_nodes.notify_all();
        return false;
    }

}

void match(unsigned int thread_id) {
    while (true) {
        unsigned int u;
        bool should_continue = take_from_Q(thread_id, u);
        if (should_continue == false) {
            break;
        }
        lock_guard <mutex> lck(ochrona1[u]);
        T[u]--;
        DR { printf("thread %d jest w %d (method %d)\n", thread_id, u, method_in_thread[thread_id]); }
        DR { cout << "Probuje adorowac " << u << endl; }
        while (T[u] < b[u] && it[u] < N[u].size()) {
            if (check_match(u, N[u][it[u]], 0)) {
                lock_guard <mutex> lck2(ochrona2[destin(N[u][it[u]])]);
                DR {
                    printf("   thread %d zaczyna przerabiac %d (method %d)\n", thread_id, destin(N[u][it[u]]),
                           method_in_thread[thread_id]);
                }
                if (check_match(u, N[u][it[u]], 1)) {
                    update(u, N[u][it[u]], thread_id, 0);
                }
                DR {
                    printf("   thread %d skonczyl przerabiac  %d method(%d)\n", thread_id, destin(N[u][it[u]]),
                           method_in_thread[thread_id]);
                }
            }
            it[u]++;
        }
    }
}

int skaluj_krawedzie(vector <kraw> &old_id, map<unsigned int, unsigned int> &map_old_to_new,
                     map<unsigned int, unsigned int> &map_new_to_old) {
    long long prev = -1;
    unsigned int new_id = 0;
    vector<unsigned int> existing_nodes;
    for (auto id : old_id) {
        existing_nodes.push_back(get<1>(id));
        existing_nodes.push_back(get<2>(id));
    }

    sort(existing_nodes.begin(), existing_nodes.end());//Pararel

    for (unsigned int id : existing_nodes) {
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

void generate_b(unsigned int thread_id) {
    auto r = range(thread_id);
    for (unsigned int i = r.first; i < r.second; i++) {
        b[order_of_nodes[i]] = bvalue(method, new_id_to_old[order_of_nodes[i]]);
    }
}

void create_graph(char *file) {
    ifstream input_file(file);
    string bufor;
    vector <kraw> readed;
    vector<unsigned int> existing_nodes;
    unsigned int a, b1, c, d = 0;
    if (input_file.is_open()) {
        while (getline(input_file, bufor)) {

            if (bufor[0] != '#') {
                sscanf(&(bufor[0]), "%u%u%u", &a, &b1, &c);
                readed.push_back(make_tuple(c, a, b1, d++));
            }
        }
    }

    n = skaluj_krawedzie(readed, old_id_to_new, new_id_to_old);
    if (n < n_threads) {
        n_threads = n;
    }
    order_of_nodes.resize(n);
    for(unsigned int i=0; i <n; i++)
        order_of_nodes[i]=i;
    random_shuffle(order_of_nodes.begin(),order_of_nodes.end());
//    reverse(order_of_nodes.begin(),order_of_nodes.end());


    threads_wating_for_node = n_threads;
    last_in_node_destin = new unsigned int[n];
    last_in_node_koszt = new unsigned int[n];
    b.resize(n);
    N.resize(n);
    T.resize(n);
    S.resize(n);
    it.resize(n);
    ochrona1 = new mutex[n];
    ochrona2 = new mutex[n];
    bariery = new condition_variable[(b_limit + 1) * 10];
    ile_czeka.resize(b_limit * 10);
    result_in_thread.resize(n_threads);

    for (auto &a:ile_czeka) {
        a = 0;
    }
    ochrona_bariery = new mutex[(b_limit + 1) * 10];

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

void clear_graph(unsigned  int thread_id) {
    auto r = range(thread_id);
    for (unsigned int i = r.first; i < r.second; i++) {
        S[i].clear();
        T[i] = 1;
        it[i] = 0;
        b[i] = 0;
        last_in_node_destin[i] = 0;
        last_in_node_koszt[i] = 0;
    }
    if (thread_id == 0) {
        threads_wating_for_node = 0;
    }
    result_in_thread[thread_id] = 0;
}

void prepare_Q(unsigned  int thread_id) {
    //dodaj wszyskie z range do Q_in_thread
    auto r = range(thread_id);
    for ( unsigned int i = r.first; i < r.second; i++) {
        if (b[order_of_nodes[i]]) {
//            printf("do kolejki %d dodaje %d   (%d)\n" ,thread_id,order_of_nodes[i],method_in_thread[thread_id]);
            Q_in_thread[thread_id].push_back(order_of_nodes[i]);
        }
    }

}

void wypisz_Q(unsigned int thread_id) {
    cout << "W kolejce " << thread_id << "\n";
    for (auto  a: Q_in_thread[thread_id]) {
        cout << a << " ";
    }
}

void wypisz_wynik() {
    unsigned  int result = 0;
    for (auto &a :result_in_thread) {
        result += a;
        a = 0;
    }
    cout << result / 2 << endl;
}

void f(unsigned int thread_id) {
    int ktora_bariera = 0;
    while (method <= b_limit) {
        clear_graph(thread_id);
        czekaj_na_pozostale(++ktora_bariera);
        generate_b(thread_id);
        prepare_Q(thread_id);
        czekaj_na_pozostale(++ktora_bariera);

        method_in_thread[thread_id]++;//Potrzebne tylko do debugu.
        match(thread_id);
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
    for (unsigned int i = 0; i < n; i++) {
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

    for (unsigned int i = 1; i < n_threads; i++) {
        myThread[i] = thread(f, i);
    }
    f(0);
    for (unsigned int i = 1; i < n_threads; i++) {
        myThread[i].join();
    }

    delete[] bariery;
    delete[] ochrona1;
    delete[] ochrona2;

}
