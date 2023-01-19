#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        ++document_count_;
        
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const auto& word : words) {
            int word_freq = count_if(words.begin(), words.end(), [&word](const auto& next_word){
                return word == next_word;
            });
            index_[word][document_id] = static_cast<double>(word_freq) / words.size();
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    
    struct Query {
        set<string> pwords;
        set<string> mwords;
    };

    int document_count_ = 0;
    map<string, map<int, double>> index_;
    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query{};
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                query.mwords.insert(word.substr(1));
            }
            else {
                query.pwords.insert(word);
            }
        }
        return query;
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        map<int, double> relevant_documents;
        for (const auto& word : query.pwords) {
            if (index_.count(word) != 0) {
                double word_idf = log(static_cast<double>(document_count_) / index_.at(word).size());
                for (const auto& [document_id, term_freq] : index_.at(word)) {
                    if (relevant_documents.count(document_id) == 0) {
                        relevant_documents[document_id] = 0.0;
                    }
                    
                    relevant_documents[document_id] += term_freq * word_idf;
                }
            }
        }
        for (const auto& word : query.mwords) {
            if (index_.count(word) != 0) {
                for (const auto& document_to_tf : index_.at(word)) {
                    if (relevant_documents.count(document_to_tf.first) != 0) {
                        relevant_documents.erase(document_to_tf.first);
                    }
                }
            }
        }
        vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : relevant_documents) {
            matched_documents.push_back({document_id, relevance});
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}
