#include<cmath>
#include<cassert>
#include<map>
#include<ctime>
#include<cstdlib>
#include<cstring>
#include<string>
#include<iostream>
#include<fstream>
#include<vector>
using namespace std;

// dimension of word and class vector
int vec_size = 20;
// size of vocabulary
int word_num;
// number of class
int class_num;
// maximum words in a record, remaining words will be ignored
int max_record_words = 100;
// minimum words in a record, record has fewer words will be ignored
int min_record_words = 1;
// minimum apperance of a word, words appeared fewer than this will be ignored
int min_word_count = 20;
// how many times to go through th train file
int max_round = 2;
// initial update rate
float start_alpha = 0.025;

float **word_vec;
float **class_vec;

// index in word_vec of a word
map<string, int> word_table;
// index in class_vec of a class
map<string, int> class_table;

ifstream train_file;
ofstream word_vec_file;
ofstream class_vec_file;

// exp(vec1 * vec2)
float expprod(float *vec1, float *vec2) {
    float p = 0;
    for (int i = 0; i < vec_size; i++) {
        p += vec1[i] * vec2[i];
    }
    return exp(p);
}

// set vec = 0
void clear_vec(float *vec) {
    for (int i = 0; i < vec_size; i++) {
        vec[i] = 0;
    }
}

// set vec1 = vec1 + rate * vec2
void update(float *vec1, float *vec2, float rate) {
    for (int i = 0; i < vec_size; i++) {
        vec1[i] += rate * vec2[i];
    }
}

// allocate memory for word_vec and class_vec and give 
// them random initial value
void init_net() {
    word_vec = new float *[word_num];
    class_vec = new float *[class_num];
    if (!(word_vec && class_vec)) {
        cerr << "MEMORY ALLOCATION ERROR" << endl;
        exit(-1);
    }

    srand(time(NULL));
    for (int i = 0; i < word_num; i++) {
        word_vec[i] = new float[vec_size];
        assert(word_vec[i]);
        for (int j = 0; j < vec_size; j++) {
            word_vec[i][j] = 2.0 * ((float) rand() / RAND_MAX - 0.5);
        }
    }
    for (int i = 0; i < class_num; i++) {
        class_vec[i] = new float[vec_size];
        assert(class_vec[i]);
        for (int j = 0; j < vec_size; j++) {
            class_vec[i][j] = 2.0 * ((float) rand() / RAND_MAX - 0.5);
        }
    }
}

// read a record, which is a list of string, from train_file
// return 1 if success else 0
int read_record(vector<string> &record) {
    string line;
    record.clear();
    if(!getline(train_file, line)) return 0;

    int begin = 0, end = 0;
    while(begin < line.length()) {
        while (begin < line.length() && (line[begin] == '\t' 
            || line[begin] == ' ')) begin++;
        if (begin == line.length()) break;

        end = begin + 1;
        while(end < line.length() && line[end] != '\t' && line[end] != ' ') {
            end++;
        }
        record.push_back(line.substr(begin, end - begin));

        begin = end + 1;
    }
    return 1;
}

// learn vocabulary and classes from the train file
void learn_vocab() {
    train_file.seekg(0, ios::beg);
    vector<string> record;
    class_num = 0;
    word_num = 0;
    map<string, int> word_count;
    while (read_record(record)) {
        if (record.size() == 0) continue;
        if (class_table.count(record[0]) == 0) {
            class_table[record[0]] = class_num++;
        }
        for (int i = 1; i < record.size(); i++) {
            word_count[record[i]]++;
        }
    }
    for (map<string, int>::iterator it = word_count.begin();
        it != word_count.end(); it++) {
        if (it->second < min_word_count) continue;
        word_table[it->first] = word_num++;
    }
}

// get a instance form train file, words are transformed to its index
// return 1 if success else 0
int get_instance(int &class_id, vector<int> &words) {
    vector<string> record;
    words.clear();
    int nword = 0;
    if (read_record(record) == 0) return 0;
    class_id = word_table[record[0]];
    for (int i = 1; i < record.size() && nword < max_record_words; i++) {
        if (word_table.count(record[i]) == 0) continue;
        nword++;
        words.push_back(word_table[record[i]]);
    }
    return 1;
}

// train the model max_round times
void train() {
    int round = 0;
    int record_count = 0;
    int round_records = 0;
	int class_id;
	float alpha;
	vector<int> words;
    // expprod_table[w][c] = exp(word_vec[w] * class_vec[c])
    vector<vector<float> > expprod_table(max_record_words,
        vector<float>(class_num, 0));
    float **deta_w = new float*[max_record_words];
    assert(deta_w);
    for (int i = 0; i < max_record_words; i++) {
    	deta_w[i] = new float[vec_size];
    	assert(deta_w[i]);
	}
	float **deta_c = new float *[class_num];
	assert(deta_c);
	for (int i = 0; i < class_num; i++) {
		deta_c[i] = new float[vec_size];
		assert(deta_c);
	}
    train_file.clear();
    train_file.seekg(0, ios::beg);
    cerr << "cur " << train_file.tellg() << endl;
    while (round < max_round) {
        if (get_instance(class_id, words) == 0) {
            round++;
            cerr << round << ' ' << round_records << ' ' << alpha << endl;
            round_records = 0;
            train_file.clear();
            train_file.seekg(0, ios::beg);
            continue;
        }
        round_records++;

        if (words.size() < min_record_words) continue;

        alpha = start_alpha * (1 - (float)record_count / 100000);
        if (record_count < 100000) {
            record_count++;
        }
        alpha = alpha < 0.0001 * start_alpha ? 0.0001 * start_alpha : alpha;

        float A = 0;
        for (int w = 0; w < words.size(); w++) {
            for (int c = 0; c < class_num; c++) {
                float a = expprod(word_vec[words[w]], class_vec[c]);
                expprod_table[w][c] = a;
                A += a;
            }
        }

        float s = 0;
        for (int w = 0; w < words.size(); w++) {
            s += expprod_table[w][class_id];
        }

        for (int w = 0; w < words.size(); w++) {
            clear_vec(deta_w[w]);
            update(deta_w[w], class_vec[class_id], A * expprod_table[w][class_id]);
            for (int c = 0; c < class_num; c++) {
                update(deta_w[w], class_vec[c], - s * expprod_table[w][c]);
            }
            
        }

        for (int c = 0; c < class_num; c++) {
            clear_vec(deta_c[c]);
            if (c == class_id) {
                for (int w = 0; w < words.size(); w++) {
                    update(deta_c[c], word_vec[words[w]], A * expprod_table[w][c]); 
                }
            }
            for (int w = 0; w < words.size(); w++) {
                update(deta_c[c], word_vec[words[w]], - s * expprod_table[w][c]);
            }
        }


		float update_rate = alpha / (A * A);
		for (int w = 0; w < words.size(); w++) {
			update(word_vec[words[w]], deta_w[w], update_rate);
		}
		for (int c = 0; c < class_num; c++) {
			update(class_vec[c], deta_c[c], update_rate);
		}
    }
}

// save word_vec and class_vec into file
void save_model() {
    word_vec_file << word_num << '\t' << vec_size << endl;
    for (map<string, int>::iterator it = word_table.begin();
        it != word_table.end(); it++) {
        word_vec_file << it->first;
        for (int i = 0; i < vec_size; i++) {
            word_vec_file << '\t' << word_vec[it->second][i];
        }
        word_vec_file << endl;
    }

    class_vec_file << class_num << endl;
    for (map<string, int>::iterator it = class_table.begin();
        it != class_table.end(); it++) {
        class_vec_file << it->first;
        for (int i = 0; i < vec_size; i++) {
            class_vec_file << '\t' << class_vec[it->second][i];
        }
        class_vec_file << endl;
    }
}

int main (int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            vec_size = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-m") == 0) {
            min_word_count = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-r") == 0) {
            max_round = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-n") == 0) {
            min_record_words = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-w") == 0) {
            max_record_words = atoi(argv[i + 1]);
        }
    }

    train_file.open("train.dat");
    word_vec_file.open("word_vec.dat", ofstream::out);
    class_vec_file.open("class_vec.dat", ofstream::out);
    if (!(train_file.is_open() && word_vec_file.is_open()
        && class_vec_file.is_open())) {
        cerr << "CAN'T OPEN FILE." << endl;
        return -1;
    }
    learn_vocab();
    init_net();
    train();
    save_model();
    train_file.close();
    word_vec_file.close();
    class_vec_file.close();
    return 0;
}
