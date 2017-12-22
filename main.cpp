#include<iostream>
#include<thread>
#include<chrono>
#include <vector>
#include <mutex>
#include <set>
#include <list>
#include <condition_variable>

using namespace std;

using kraw = pair<int,int>;///koszt, cel


vector<vector<kraw>> N;
vector<mutex> ochrona;
vector<int> b;

vector<set<kraw>> S;//ci ktorzy mi sie oświadczyli
vector<set<kraw>> T;//ci którym się oświdczyłem
list<int> Q;//Q
mutex ochrona_kolejki;
condition_variable pustaKolejka;

void f(){
    int u=-1;
    while(u==-1){
        unique_lock<mutex> lock(ochrona_kolejki);
        if(Q.empty() == false){
            u=Q.front();
            Q.pop_front();
        }else
            pustaKolejka.wait(lock);
    }
    //Tutaj powinienem mieć wyliczony u

    int i=

}


void parrarel_b_suitor(){

    {
        set<int> dodani;
        for (kraw k: N) {
            if(dodani.find(k.second) == dodani.end()){
                dodani.insert(k.second);
                Q.push_back(k.second);
            }
        }
    }

}