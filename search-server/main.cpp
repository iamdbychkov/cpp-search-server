#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include <tuple>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
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
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    template<typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter filter) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, filter); sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus& status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(
            raw_query,
            [&status](int id, DocumentStatus status_to_filter, int rating){ return status_to_filter == status; }
        );
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

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

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename Filter>
    vector<Document> FindAllDocuments(const Query& query, Filter filter) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (filter(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        assert(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        assert(doc0.id == doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        assert(server.FindTopDocuments("in"s).empty());
    }
}

void TestDocumentsWithMinusWordsAreExcludedFromResults() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    // Ищем кошку, но не в городе, поэтому -city должен исключить документы с словом "city" из выборки
    assert(server.FindTopDocuments("cat -city"s).empty());
}

void TestMatchDocumentMethodReturnsAllMatchedWordsAndDocumentStatus() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    // Запрос без минус слов, MatchDocument должен вернуть пересечение из слов между запросом и документом.
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status] = server.MatchDocument("cat city"s, doc_id);
        vector<string> expected_words = {"cat"s, "city"s};
        assert(words == expected_words);
        assert(status == DocumentStatus::ACTUAL);
    }

    // Минус-слова в запросе, MatchDocument должен вернуть пустой список слов.
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status] = server.MatchDocument("cat -city"s, doc_id);
        assert(words.empty());
        assert(status == DocumentStatus::ACTUAL);
    }
}

void TestDocumentsAreSortedByItDescendingRelevance() {
    /*
    Релевантность документа зависит от TF-IDF индекса.
    */
    struct DocumentData {
        int id;
        string content;
        vector<int> ratings;
    };
    vector<DocumentData> test_documents_data = {
        {42, "cat in the city"s, {1, 2, 3}},
        {43, "dog in the city of Moscow"s, {1, 2, 3}},
        {44, "cat and dog in the city with mayor cat"s, {1, 2, 3}},
        {45, "cat in the city of Beijing of China country"s, {1, 2, 3}},
    };

    SearchServer server;
    server.SetStopWords("in the"s);
    for (const auto& [id, content, ratings] : test_documents_data) {
        server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
    }
    // Для такого запроса более релевантным документом должен быть документ под id 43,
    // т.к. IDF для документов 43 и 44 будет одинаков, но TF поднимет документ с id 43 вверх, т.к.
    // в этом документе меньше слов.
    {
        const auto found_docs = server.FindTopDocuments("dog has big puffy tail"s);
        assert(found_docs.size() == 2);
        assert(found_docs[0].id == 43);
        assert(found_docs[1].id == 44);
    }
    // Чем больше слов в документе тем меньше TF слова из поискового запроса, а соответственно и произведение TF-IDF, поэтому
    // для такого запроса вверху должен оказаться документ под id 42, а 44 уйти вниз.
    // 45ый должен уйти вниз, т.к. для слова cat TF в документе 44 выше, а соответственно и TF-IDF поднимет его в поиске выше.
    {
        const auto found_docs = server.FindTopDocuments("big fluffy cat"s);
        assert(found_docs.size() == 3);
        assert(found_docs[0].id == 42);
        assert(found_docs[1].id == 44);
        assert(found_docs[2].id == 45);
    }
}

void TestDocumentsRatingCalculations() {
    const int doc_id = 42;
    const string content = "cat in the city"s;

    // Рейтинг документа для документа без оценок должен быть нулём.
    {
        SearchServer server;
        const vector<int> ratings = {};
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs  = server.FindTopDocuments("cat city"s);
        const Document& doc = found_docs[0];
        assert(doc.rating == 0.0);
    }
    // Если переданы оценки - рейтинг документа это среднее арифметическое всех оценок.
    // Для оценок 5 4 5 3 среднее арифметическое = 4.25, но т.к. рейтинг у нас целочисленое значение, то получаем 4.
    {
        SearchServer server;
        const vector<int> ratings = {5, 4, 5, 3};
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs  = server.FindTopDocuments("cat city"s);
        const Document& doc = found_docs[0];
        assert(doc.rating == 4);
    }
}

void TestDocumentsFiltering() {
    struct DocumentData {
        int id;
        string content;
        vector<int> ratings;
        DocumentStatus status;
    };
    vector<DocumentData> test_documents_data = {
        {1, "cat in the city"s, {2, 3, 3}, DocumentStatus::ACTUAL},
        {2, "dog in the city of Moscow"s, {4, 5, 3}, DocumentStatus::ACTUAL},
        {3, "cat and dog in the city with mayor rat"s, {5, 5, 5}, DocumentStatus::ACTUAL},
        {4, "cat in the city of Beijing of China country"s, {3, 3, 4}, DocumentStatus::BANNED},
    };

    SearchServer server;
    server.SetStopWords("in the"s);
    for (const auto& [id, content, ratings, status] : test_documents_data) {
        server.AddDocument(id, content, status, ratings);
    }

    // Проверяем возможность фильтровать по ID, получаем только чётные документы.
    {
        const auto found_docs = server.FindTopDocuments(
            "city"s, [](int document_id, DocumentStatus status, int rating) {
                if (document_id % 2 != 0) {
                    return false;
                }
                return true;
            }
        );
        assert(found_docs.size() == 2);
        assert(found_docs[0].id == 2);
        assert(found_docs[1].id == 4);
    }
    // Попробуем отфильтровать по статусу
    {
        const auto found_docs = server.FindTopDocuments(
            "city"s, [](int document_id, DocumentStatus status, int rating) {
                return status == DocumentStatus::BANNED;
            }
        );
        assert(found_docs.size() == 1);
        assert(found_docs[0].id == 4);
    }
    // Получим все документы с рейтингом больше 4.
    {
        const auto found_docs = server.FindTopDocuments(
            "city"s, [](int document_id, DocumentStatus status, int rating) {
                return rating > 4;
            }
        );
        assert(found_docs.size() == 1);
        assert(found_docs[0].id == 3);
    }
}

void TestFindTopDocumentsWithSpecificStatus() {
    struct DocumentData {
        int id;
        string content;
        vector<int> ratings;
        DocumentStatus status;
    };
    vector<DocumentData> test_documents_data = {
        {1, "cat in the city"s, {2, 3, 3}, DocumentStatus::ACTUAL},
        {2, "dog in the city of Moscow"s, {4, 5, 3}, DocumentStatus::ACTUAL},
        {3, "cat and dog in the city with mayor rat"s, {5, 5, 5}, DocumentStatus::IRRELEVANT},
        {4, "cat in the city of Beijing of China country"s, {3, 3, 4}, DocumentStatus::IRRELEVANT},
    };

    SearchServer server;
    server.SetStopWords("in the"s);
    for (const auto& [id, content, ratings, status] : test_documents_data) {
        server.AddDocument(id, content, status, ratings);
    }
    // По умолчанию FindTopDocuments ищет только документы с статусом ACTUAL.
    {
        const auto found_docs = server.FindTopDocuments("cat and city"s);
        assert(found_docs.size() == 2);
        for (const auto& doc : found_docs) {
            if (doc.id == 3 || doc.id == 4) {
                assert(false); // Документы с другим статусом
            }
        }
    }
    // Если указан статус, то должны найтись документы только с этим статусом
    {
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::IRRELEVANT);
        assert(found_docs.size() == 2);
        for (const auto& doc : found_docs) {
            if (doc.id == 1 || doc.id == 2) {
                assert(false); // Документы с другим статусом
            }
        }
    }
    // Если документов с таким статусом нет, то должен вернуться пустой результат.
    {
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::BANNED);
        assert(found_docs.empty());
    }
}

void TestDocsRelevanceAreCalculatedCorrectly() {
        struct DocumentData {
        int id;
        string content;
        vector<int> ratings;
    };
    vector<DocumentData> test_documents_data = {
        {42, "cat in the city"s, {1, 2, 3}},
        {43, "dog in the city of Moscow"s, {1, 2, 3}},
        {44, "cat and dog in the city with mayor cat"s, {1, 2, 3}},
        {45, "cat in the city of Beijing of China country"s, {1, 2, 3}},
    };

    SearchServer server;
    server.SetStopWords("in the and of"s);
    for (const auto& [id, content, ratings] : test_documents_data) {
        server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
    }
    /* Мы можем рассчитать IDF слов сразу:
     * 1. cat     = log(4 / 3) = 0.2851
     * 2. city    = log(4 / 4) = 0.00
     * 3. dog     = log(4 / 2) = 0.6931
     * 4. Moscow  = log(4 / 1) = 1.3862
     * 5. with    = log(4 / 1) = 1.3862
     * 6. mayor   = log(4 / 1) = 1.3862
     * 7. Beijing = log(4 / 1) = 1.3862
     * 8. China   = log(4 / 1) = 1.3862
     * 9. country = log(4 / 1) = 1.3862
     *
     * + Посчитаем итоговый TF для слов документов:
     *
     *          0.5        0.5
     *   42 -> "cat in the city"
     *          0.33       0.33    0.33
     *   43 -> "dog in the city of Moscow"
     *          0.33    0.16       0.16 0.16 0.16  -//-
     *   44 -> "cat and dog in the city with mayor cat"
     *          0.2        0.2     0.2        0.2   0.2
     *   45 -> "cat in the city of Beijing of China country"
     *
     * для запроса "cat in the city" мы найдём все 4 документа c расчётными relevance:
     * 42 -> (0.5 * 0.2851) + (0.5 * 0.0) = 0.14255
     * 43 -> (0.33 * 0.0) = 0
     * 44 -> (0.33 * 0.2851) + (0.33 * 0.0) = 0.09408
     * 45 -> (0.2 * 0.2851) + (0.2 * 0.0) = 0.05702
     *
     * Получается документы получим в следующем порядке: 42, 44, 45, 43.
     */

    // Погрешность в две десятки? Ну, может быть, ручные вычисления вряд ли точно попадут в ожидания от программы, поэтому так.
    const double EPSILON = 0.002;
    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s);
        vector<tuple<int, double>> expected_result = {
            {42, 0.14255},
            {44, 0.09408},
            {45, 0.05702},
            {43, 0.00000}
        };
        for (int i = 0; i < static_cast<int>(expected_result.size()); ++i) {
            const auto [expected_id, expected_relevance] = expected_result[i];
            assert(found_docs[i].id == expected_id);
            assert(abs(found_docs[i].relevance - expected_relevance) < EPSILON);
        }

    }

}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestDocumentsWithMinusWordsAreExcludedFromResults();
    TestDocumentsAreSortedByItDescendingRelevance();
    TestDocumentsRatingCalculations();
    TestDocumentsFiltering();
    TestFindTopDocumentsWithSpecificStatus();
    TestDocsRelevanceAreCalculatedCorrectly();
}

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    TestSearchServer();
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "ACTUAL:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; })) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}
