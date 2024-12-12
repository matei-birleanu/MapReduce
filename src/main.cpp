#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <set>


using namespace std;

struct thread_info_mapper {
    int thread_id;
    std::map<std::string, pair<int, int>> filelist;
    vector<string> files;
    std::map<std::string, set<int>> wordlist;
    pthread_mutex_t *mutex;
    pthread_barrier_t *barrier;
};

struct thread_info_reducer {
    int thread_id;
    vector<char> assigned_letters;
    vector<thread_info_mapper> *mappers_info;
    map<string, set<int>> *aggregate_list;
    pthread_mutex_t *mutex;
    pthread_barrier_t *barrier;
};

void processWord(std::string &word) {
    for (char &c : word) {
        c = std::tolower(c);
    }
    std::string result;
    for (char c : word) {
        if (std::isalpha(c)) {
            result += c;
        }
    }
    word = result;
}
bool compareWords(const std::pair<std::string, std::set<int>>& a, const std::pair<std::string, std::set<int>>& b) {
    if (a.second.size() != b.second.size())
        return a.second.size() > b.second.size();
    else
        return a.first < b.first;
}
void *freduce(void *arg) {
    struct thread_info_reducer *reducer_info = (struct thread_info_reducer *)arg;
    pthread_barrier_wait(reducer_info->barrier);
    // creez lista agregata 
    for (const auto &mapper : (*reducer_info->mappers_info)) {
        for (const auto &wordfile : mapper.wordlist) {
            string word = wordfile.first;
            set<int> file_ids = wordfile.second;
            if (!word.empty()) {
                char first_char = word[0];
                vector<char> letters = reducer_info->assigned_letters;
                if (std::find(letters.begin(), letters.end(), first_char) != letters.end()) {
                    // fac lock/unlock pe lista agregata si adaug cuvintele care incep cu literele asignate
                    int ret = pthread_mutex_lock(reducer_info->mutex);
                    if (ret != 0)
                        fprintf(stderr, "Error: Mutex lock failed (%s)\n", strerror(ret));
                    (*reducer_info->aggregate_list)[word].insert(file_ids.begin(), file_ids.end());
                    ret = pthread_mutex_unlock(reducer_info->mutex);
                    if (ret != 0)
                        fprintf(stderr, "Error: Mutex unlock failed (%s)\n", strerror(ret));
                }
            }
        }
    }
    // realizez scrierea in fisiere
    for (char letter : reducer_info->assigned_letters) {
        string output_file = string(1, letter) + ".txt";
        ofstream out(output_file);
        vector<pair<string, set<int>>> wordswrite;
        for (const auto &pair : (*reducer_info->aggregate_list)) {
            string word = pair.first;
            const set<int> ids = pair.second;
            if (!word.empty() && word[0] == letter)
                wordswrite.emplace_back(word, ids);
        }
    sort(wordswrite.begin(), wordswrite.end(), compareWords);


        for (const auto &word_files : wordswrite) {
            out << word_files.first << ":[";
            const std::set<int> &file_ids = word_files.second;
            int size = file_ids.size();
            int k = 0;
            for (int id : file_ids) {
                if (k == size - 1)
                    out << id;
                else
                    out << id << " ";
                k++;
            }
            out << "]" << endl;
        }
    }
    pthread_exit(NULL);
}

void *fmap(void *arg) {
    struct thread_info_mapper *info = (struct thread_info_mapper *)arg;

    for (string file : info->files) {
        int id_file = info->filelist[file].second;
        string path = file;

        std::ifstream in(path);
        if (!in.is_open())
            std::cerr << "Error: Could not open file " << file << "\n";
        // realizez operatia de map
        string word;
        while (in >> word) {
            processWord(word);
            if (!word.empty()) {
                info->wordlist[word].insert(id_file);
            }
        }
    }
    pthread_barrier_wait(info->barrier);
    pthread_exit(NULL);
}

std::map<std::string, pair<int, int>> store_files(char *filename) {
    std::ifstream file(filename);
    std::map<std::string, pair<int, int>> filelist;
    int nr;
    file >> nr;
    file.get();
    string name;
    int k = 0;
    // pentru ficare fisier stochez id si size
    while (nr) {
        file >> name;
        name.insert(0, "../checker/");
        std::ifstream in(name);
        if (!in.is_open()) {
            std::cerr << "Error: Could not open file " << name << "\n";
        }
        in.seekg(0, std::ios::end);
        std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);
        filelist[name].first = size;
        k++;
        filelist[name].second = k;
        nr--;
    }
    return filelist;
}

bool compareFiles(const std::pair<std::string, int> &a, const std::pair<std::string, int> &b) {
    return a.second > b.second;
}
bool compareLoad(const std::pair<const int, int>& a, const std::pair<const int, int>& b) {
    return a.second < b.second;
}
std::map<int, vector<string>> make_files_split(std::map<std::string, pair<int, int>> filelist, int nr_mappers) {
    vector<pair<string, int>> files;
    std::map<int, vector<string>> result;
    for (const auto &[name, intpair] : filelist)
        files.emplace_back(name, intpair.first);

    std::sort(files.begin(), files.end(), compareFiles);
    std::map<int, int> thread_workload;
    for (int i = 0; i < nr_mappers; ++i) {
        thread_workload[i] = 0;
    }
    // la fiecare pas thread ul cu cel mai mic workload va prelua fisierul
    for (const auto &[name, size] : files) {
        auto min_thread = std::min_element(
            thread_workload.begin(), thread_workload.end(), compareLoad);
        int aux = min_thread->first;
        aux++;
        result[aux].push_back(name);
        min_thread->second += size;
    }
    return result;
}

map<int, vector<char>> assign_letters(int nr_reducers) {
    map<int, vector<char>> assign;
    char letter = 'a';
    int letters_per_thread = 26 / nr_reducers;
    int extra = 26 % nr_reducers;
    int num;
    for (int i = 1; i <= nr_reducers; i++) {
        if (extra) {
            num = letters_per_thread + 1;
            extra--;
        } else
            num = letters_per_thread;
        for (int j = 0; j < num; j++) {
            assign[i].push_back(letter);
            letter++;
        }
    }
    return assign;
}

int main(int argc, char **argv) {
    if(argc != 4){
        printf("The program should run like this ./tema1 <numar_mapperi> <numar_reduceri> <fisier_intrare>\n");
        exit(EXIT_FAILURE);
    }
    int nr_mappers = atoi(argv[1]);
    int nr_reducers = atoi(argv[2]);
    int i, r;
    char filename[100];
    int total = nr_mappers + nr_reducers;
    pthread_t threads[total];

    strcpy(filename, argv[3]);
    std::map<std::string, pair<int, int>> filelist;
    std::map<int, vector<string>> thread_files;
    std::map<std::string, set<int>> wordlist;
    map<string, set<int>> aggregate_list;

    filelist = store_files(filename);

    pthread_mutex_t mutex;
    pthread_barrier_t barrier;

    int ok = pthread_mutex_init(&mutex, NULL);
    if (ok != 0) {
        fprintf(stderr, "Error: Mutex initialization failed (%s)\n", strerror(ok));
        exit(EXIT_FAILURE);
    }

    ok = pthread_barrier_init(&barrier, NULL, total);
    if (ok != 0) {
        fprintf(stderr, "Error: Barrier initialization failed (%s)\n", strerror(ok));
        exit(EXIT_FAILURE);
    }

    thread_files = make_files_split(filelist, nr_mappers);
    vector<struct thread_info_mapper> mappersinfo;
    vector<struct thread_info_reducer> reducersinfo;
    map<int, vector<char>> assigned_letters;

    assigned_letters = assign_letters(nr_reducers);
    // initializez structurile ce vor fi trimise thread-urilor
    for (int i = 1; i <= nr_mappers; i++) {
        struct thread_info_mapper a;
        a.files = thread_files[i];
        a.thread_id = i;
        a.filelist = filelist;
        a.wordlist = wordlist;
        a.mutex = &mutex;
        a.barrier = &barrier;
        mappersinfo.push_back(a);
    }

    for (int i = 1; i <= nr_reducers; i++) {
        struct thread_info_reducer a;
        a.thread_id = i;
        a.assigned_letters = assigned_letters[i];
        a.mappers_info = &mappersinfo;
        a.aggregate_list = &aggregate_list;
        a.mutex = &mutex;
        a.barrier = &barrier;
        reducersinfo.push_back(a);
    }

    for (i = 0; i < nr_mappers + nr_reducers; i++) {
        if (i >= nr_mappers) {
            r = pthread_create(&threads[i], NULL, freduce, &reducersinfo[i - nr_mappers]);
        } else {
            r = pthread_create(&threads[i], NULL, fmap, &mappersinfo[i]);
        }
        if (r) {
            printf("Error creating thread %d\n", i);
            exit(-1);
        }
    }

    void *status;
    for (i = 0; i < nr_mappers + nr_reducers; i++) {
        r = pthread_join(threads[i], &status);
        if (r) {
            printf("Error joining thread %d\n", i);
            exit(-1);
        }
    }

    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);

    return 0;
}
