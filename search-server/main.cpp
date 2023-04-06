#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <optional>
#include <stdexcept>

using namespace std;

template<typename Container>
void Print(ostream& o, const Container& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) {
            o << ", "s;
        }
        is_first = false;
        o << element;
    }
}

template<typename Key, typename Value>
void Print(ostream& o, const map<Key, Value>& container) {
    bool is_first = true;
    for (const auto& [key, value] : container) {
        if (!is_first) {
            o << ", "s;
        }
        is_first = false;
        o << key << ": "s << value;
    }
}

template<typename Any>
ostream& operator<<(ostream& o, const vector<Any>& container) {
    o << "["s;
    Print(o, container);
    o << "]"s;
    return o;
}

template<typename Any>
ostream& operator<<(ostream& o, const set<Any>& container) {
    o << "{"s;
    Print(o, container);
    o << "}"s;
    return o;
}

template<typename Key, typename Value>
ostream& operator<<(ostream& o, const map<Key, Value>& container) {
    o << "{"s;
    Print(o, container);
    o << "}"s;
    return o;
}


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename callable>
void RunTestImpl(const callable& function, const string& function_string) {
    function();
    cerr << function_string << " "s << "OK" << endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)


void AssertThrowsImpl(const string& expr_str, const string& exception_str, const string& file, const string& func, unsigned line, const string& hint) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_THROWS("s << expr_str << ") expected to throw -> " << exception_str << " which it did not."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
}

#define ASSERT_THROWS_HINT(expr, exception, hint)                                      \
	try {                                                                              \
		expr;                                                                          \
	} catch(const exception& e){                                                       \
	} catch(...) {                                                                     \
		AssertThrowsImpl(#expr, #exception, __FILE__, __FUNCTION__, __LINE__, (hint)); \
	}                                                                                  \

#define ASSERT_THROWS(expr, exception) ASSERT_THROWS_HINT(expr, exception, "");

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

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

    Document() {
        id = 0;
        relevance = 0.0;
        rating = 0;
    }

    Document(int id_, double relevance_, int rating_) {
        id = id_;
        relevance = relevance_;
        rating = rating_;
    }
};


enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

ostream& operator << (ostream& o, const DocumentStatus& status) {
    switch (status) {
        case DocumentStatus::ACTUAL:
            o << "ACTUAL"s;
            break;
        case DocumentStatus::IRRELEVANT:
            o << "IRRELEVANT"s;
            break;
        case DocumentStatus::BANNED:
            o << "BANNED"s;
            break;
        case DocumentStatus::REMOVED:
            o << "REMOVED"s;
            break;
    }
    return o;
}

class SearchServer {
public:

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    SearchServer() = default;

    SearchServer(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            CheckIfWordIsValid(word);
            stop_words_.insert(word);
        }
    }

    template<typename Container>
    explicit SearchServer(const Container& stop_words) {
        for (const string& word : stop_words) {
            CheckIfWordIsValid(word);
            if (!word.empty() && stop_words_.count(word) == 0) {
                stop_words_.insert(word);
            }
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        if (document_id < 0 || documents_.count(document_id) != 0) {
            throw(invalid_argument("Document id can't be negative nor be equal to already added documents"s));
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const string& word : words) {
            CheckIfWordIsValid(word);
        }
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    template<typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter filter) const {
        Query query = ParseQuery(raw_query);

        vector<Document> matched_documents = FindAllDocuments(query, filter);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                    return lhs.rating > rhs.rating;
                } else {
                    return lhs.relevance > rhs.relevance;
                }
            }
        );
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus& status) const {
        return FindTopDocuments(
            raw_query,
            [&status](int id, DocumentStatus status_to_filter, int rating){ return status_to_filter == status; }
        );
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(
            raw_query,
            [](int id, DocumentStatus status_to_filter, int rating){ return status_to_filter == DocumentStatus::ACTUAL; }
        );
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    const set<string>& GetStopWords() const {
        return stop_words_;
    }

    int GetDocumentId(int index) const {
        if (index < 0 || index >= static_cast<int>(documents_.size())) {
            throw(out_of_range("Document index is out of range"s));
        }
        int counter = 0;
        for (const auto& [id, document] : documents_) {
            if (counter == index) {
                return id;
            }
            ++counter;
        }
        return SearchServer::INVALID_DOCUMENT_ID;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(
        const string& raw_query, int document_id
    ) const {

        if (document_id < 0) {
            throw(invalid_argument("Document ID cannot be negative"s));
        }
        Query query = ParseQuery(raw_query);

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

    static void CheckIfWordIsValid(const string& word) {
        // A valid word must not contain special characters
        if (std::any_of(std::begin(word), std::end(word), [](char c) {
            return c >= '\0' && c < ' ';
        })) {
            throw(invalid_argument("Word cannot contain special symbols"s));
        }
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
        QueryWord query_word;
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        CheckIfWordIsValid(text);
        if (text.size() == 0 || text[0] == '-') {
            throw(invalid_argument("Word can not be empty nor start with multiple minus signs"s));
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
                QueryWord query_word = ParseQueryWord(word);
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
    {
        SearchServer server;
        (void) server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server{"in the"s};
        (void) server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_HINT(found_docs.empty(), "Stop words must be excluded from documents"s);
    }
}


void TestDocumentsWithMinusWordsAreExcludedFromResults() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    SearchServer server;
    (void) server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    // Ищем кошку, но не в городе, поэтому -city должен исключить документы с словом "city" из выборки
    const auto found_docs = server.FindTopDocuments("cat -city"s);
    ASSERT_HINT(found_docs.empty(), "Minus words should exlude document from search result"s);
}

void TestMatchDocumentMethod() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    // Запрос без минус слов, MatchDocument должен вернуть пересечение из слов между запросом и документом.
    {
        SearchServer server;
        (void) server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<string> expected_words = {"cat"s, "city"s};
        const auto match_result = server.MatchDocument("cat city"s, doc_id);
        ASSERT_EQUAL(get<0>(match_result), expected_words);
        ASSERT_EQUAL(get<1>(match_result), DocumentStatus::ACTUAL);
    }

    // Минус-слова в запросе, MatchDocument должен вернуть пустой список слов.

    {
        SearchServer server;
        (void) server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto match_result = server.MatchDocument("cat -city"s, doc_id);
        ASSERT(get<0>(match_result).empty());
        ASSERT_EQUAL(get<1>(match_result), DocumentStatus::ACTUAL);
    }
    // Невалидные поисковые слова для метода MatchDocument.
    {
        SearchServer server;
        (void) server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<string> test_cases = {
            "cat --city"s,
            "cat -"s,
            "cat \x12"s,
        };
        for (const auto& case_words : test_cases) {
            try {
                (void) server.MatchDocument(case_words, doc_id);
                ASSERT_HINT(false, "MatchDocument did not throw an exception for invalid input");
            } catch(const invalid_argument& e) {
            } catch(...) {
                ASSERT_HINT(false, "MatchDocument shoul've raised invalid_argument exception");
            }
        }
    }
    // Вызов MatchDocument с отрицательным id документа.
    {
        SearchServer server;

        try {
            (void) server.MatchDocument("cat -city"s, -1);
            ASSERT_HINT(false, "MatchDocument did not throw an exception for invalid input");
        } catch(const invalid_argument& e) {
        } catch(...) {
            ASSERT_HINT(false, "MatchDocument shoul've raised invalid_argument exception");
        }
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

    SearchServer server{"in the"s};
    for (const auto& [id, content, ratings] : test_documents_data) {
        (void) server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
    }
    // Для такого запроса более релевантным документом должен быть документ под id 43,
    // т.к. IDF для документов 43 и 44 будет одинаков, но TF поднимет документ с id 43 вверх, т.к.
    // в этом документе меньше слов.
    {

        const auto found_docs = server.FindTopDocuments("dog has big puffy tail"s);
        ASSERT_EQUAL(found_docs.size(), 2u);
        ASSERT_EQUAL(found_docs[0].id, 43u);
        ASSERT_EQUAL(found_docs[1].id, 44u);
        double prev_relevance = found_docs[0].relevance;
        for (auto& doc : found_docs) {
            // Если из текущего relevance вычесть relevance предыдущего документа в очереди, то должно получиться число
            // меньше, либо равное нулю, т.к. первыми в выдаче должны идти документы с бОльшим relevance.
            ASSERT_HINT(doc.relevance - prev_relevance <= 0, "Documents are not sorted by it's relevance");
            prev_relevance = doc.relevance;
        }
    }
    // Чем больше слов в документе тем меньше TF слова из поискового запроса, а соответственно и произведение TF-IDF, поэтому
    // для такого запроса вверху должен оказаться документ под id 42, а 44 уйти вниз.
    // 45ый должен уйти вниз, т.к. для слова cat TF в документе 44 выше, а соответственно и TF-IDF поднимет его в поиске выше.
    {
        const auto found_docs = server.FindTopDocuments("big fluffy cat"s);
        ASSERT_EQUAL(found_docs.size(), 3u);
        ASSERT_EQUAL(found_docs[0].id, 42u);
        ASSERT_EQUAL(found_docs[1].id, 44u);
        ASSERT_EQUAL(found_docs[2].id, 45u);
        double prev_relevance = found_docs[0].relevance;
        for (auto& doc : found_docs) {
            // Если из текущего relevance вычесть relevance предыдущего документа в очереди, то должно получиться число
            // меньше, либо равное нулю, т.к. первыми в выдаче должны идти документы с бОльшим relevance.
            ASSERT_HINT(doc.relevance - prev_relevance <= 0, "Documents are not sorted by it's relevance");
            prev_relevance = doc.relevance;
        }
    }
}

void TestDocumentsRatingCalculations() {
    const int doc_id = 42;
    const string content = "cat in the city"s;

    // Рейтинг документа для документа без оценок должен быть нулём.
    {
        SearchServer server;
        const vector<int> ratings = {};
        (void) server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat city"s);
        const Document& doc = found_docs[0];
        ASSERT_EQUAL(doc.rating, 0.0);
    }
    // Если переданы оценки - рейтинг документа это среднее арифметическое всех оценок.
    // Для оценок 5 4 5 3 среднее арифметическое = 4.25, но т.к. рейтинг у нас целочисленое значение, то получаем 4.
    {
        SearchServer server;
        const vector<int> ratings = {5, 4, 5, 3};
        (void) server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat city"s);
        const Document& doc = found_docs[0];
        ASSERT_EQUAL(doc.rating, 4u);
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

    SearchServer server{"in the"s};
    for (const auto& [id, content, ratings, status] : test_documents_data) {
        (void) server.AddDocument(id, content, status, ratings);
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
        ASSERT_EQUAL(found_docs.size(), 2u);
        ASSERT_EQUAL(found_docs[0].id, 2u);
        ASSERT_EQUAL(found_docs[1].id, 4u);
    }
    // Попробуем отфильтровать по статусу
    {
        const auto found_docs = server.FindTopDocuments(
            "city"s, [](int document_id, DocumentStatus status, int rating) {
                return status == DocumentStatus::BANNED;
            }
        );
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].id, 4u);
    }
    // Получим все документы с рейтингом больше 4.
    {
        const auto found_docs = server.FindTopDocuments(
            "city"s, [](int document_id, DocumentStatus status, int rating) {
                return rating > 4;
            }
        );
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].id, 3u);
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

    SearchServer server{"in the"s};
    for (const auto& [id, content, ratings, status] : test_documents_data) {
        (void) server.AddDocument(id, content, status, ratings);
    }
    // Без явного указания статуса, метод возвращает все документы с статусом ACTUAL.
    {
        const auto found_docs = server.FindTopDocuments("cat and city"s);
        ASSERT_EQUAL(found_docs.size(), 2);
    }
    // Если указан статус, то должны найтись документы только с этим статусом
    {
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs.size(), 2);
        for (const auto& doc : found_docs) {
            if (doc.id == 1 || doc.id == 2) {
                ASSERT_HINT(false, "Documents with incorrect status were found"); // Документы с другим статусом
            }
        }
    }
    // Если документов с таким статусом нет, то должен вернуться пустой результат.
    {
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::BANNED);
        ASSERT_HINT(found_docs.empty(), "Search Server returned documents with incorrect status"s);
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

    SearchServer server{"in the and of"s};
    for (const auto& [id, content, ratings] : test_documents_data) {
        (void) server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
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
    const double EPSILON_TEST = 0.002;
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
            ASSERT_EQUAL(found_docs[i].id, expected_id);
            ASSERT_HINT(abs(found_docs[i].relevance - expected_relevance) < EPSILON_TEST, "Error threshold exceeded maximum allowed value.");
        }

    }

}

void TestSearchServerInitializer() {
    set<string> expected = {"и"s, "в"s, "на"s};
    {
        const vector<string> stop_words_vector = {"и"s, "в"s, "на"s, ""s, "в"s};
        SearchServer search_server(stop_words_vector);
        const set<string>& actual = search_server.GetStopWords();
        ASSERT_EQUAL(expected, actual);
    }
    {
        const set<string> stop_words_set = {"и"s, "в"s, "на"s};
        SearchServer search_server(stop_words_set);
        const set<string>& actual = search_server.GetStopWords();
        ASSERT_EQUAL(expected, actual);
    }
    {
        SearchServer search_server("  и  в на   "s);
        const set<string>& actual = search_server.GetStopWords();
        ASSERT_EQUAL(expected, actual);
    }
}

void TestAddDocument() {
    // Проверяем, что не можем добавить документ с отрицательным id.
    string content = "Test adding documents";
    vector<int> ratings = {5, 5, 5};
    DocumentStatus status = DocumentStatus::ACTUAL;
    {
        SearchServer search_server;
        int id = -1;
        try {
            search_server.AddDocument(id, content, status, ratings);
            ASSERT_HINT(false, "Search server should disallow adding documents with negative id"s);
        } catch (const invalid_argument& e) {
        } catch (...) {
            ASSERT_HINT(false, "Search server should've raised invalid_argument exception."s);
        }
    }
    // Проверяем, что не можем добавить документ с id, если документ с таким id уже
    // добавлен
    {
        SearchServer search_server;
        int id = 1;
        search_server.AddDocument(id, content, status, ratings);
        string content2 = "Test adding another document";
        try {
            search_server.AddDocument(id, content2, status, ratings);
            ASSERT_HINT(false, "Search server should disallow adding documents if id already exists");
        } catch (const invalid_argument& e) {
        } catch (...) {
            ASSERT_HINT(false, "Search server should've raised invalid_argument exception."s);
        }

    }
    // Не можем добавить документ, если в тексте есть слова с спецсимволами.
    {
        SearchServer search_server;
        int id = 1;
        string content = "Test adding another document\x10\x12\x15";
        try {
            search_server.AddDocument(id, content, status, ratings);
            ASSERT_HINT(false, "Search server added document with special symbols, which it shouldn't do");
        } catch (const invalid_argument& e) {
        } catch (...) {
            ASSERT_HINT(false, "Search server should've raised invalid_argument exception."s);
        }
    }
}

void TestFindTopDocsWithInvalidQuery() {
    SearchServer server;
    vector<string> test_cases = {
        "cat --city"s,
        "cat -"s,
        "cat \x12"s,
    };
    for (const auto& case_words : test_cases) {
        try {
            (void) server.FindTopDocuments(case_words);
            ASSERT_HINT(false, "Search server found docs using invalid query.");
        } catch (const invalid_argument& e) {
        } catch (...) {
            ASSERT_HINT(false, "Search server should've raised invalid_argument exception."s);
        }
    }
}

void TestGetDocumentId() {
    struct DocumentData {
        int id;
        string content;
        vector<int> ratings;
    };
    vector<DocumentData> test_documents_data = {
        {4, "cat in the city"s, {1, 2, 3}},
        {43, "dog in the city of Moscow"s, {1, 2, 3}},
        {49, "cat and dog in the city with mayor cat"s, {1, 2, 3}},
        {55, "cat in the city of Beijing of China country"s, {1, 2, 3}},
    };

    SearchServer server;
    for (const auto& [id, content, ratings] : test_documents_data) {
        (void) server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
    }
    ASSERT(server.GetDocumentId(0) == 4);
    ASSERT(server.GetDocumentId(1) == 43);
    ASSERT(server.GetDocumentId(2) == 49);
    ASSERT(server.GetDocumentId(3) == 55);

    // GetDocumentById за пределами кол-ва документов.
    // Отрицательные айди документа
    for (int doc_id : {4, -1}) {
        try {
            (void) server.GetDocumentId(doc_id);
            ASSERT_HINT(false, "GetDocumentId did not throw an exception for out of range document ID"s);
        } catch (const out_of_range& e) {
            ASSERT(e.what() == "Document index is out of range"s);
        } catch (...) {
            ASSERT_HINT(false, "GetDocumentId did not throw out_of_range"s);
        }
    }
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestDocumentsWithMinusWordsAreExcludedFromResults);
    RUN_TEST(TestDocumentsAreSortedByItDescendingRelevance);
    RUN_TEST(TestDocumentsRatingCalculations);
    RUN_TEST(TestDocumentsFiltering);
    RUN_TEST(TestFindTopDocumentsWithSpecificStatus);
    RUN_TEST(TestDocsRelevanceAreCalculatedCorrectly);
    RUN_TEST(TestSearchServerInitializer);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMatchDocumentMethod);
    RUN_TEST(TestFindTopDocsWithInvalidQuery);
    RUN_TEST(TestGetDocumentId);
}
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    TestSearchServer();
    SearchServer search_server{"и в на"s};
    (void) search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    (void) search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    (void) search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    (void) search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    cout << "ACTUAL by default:"s << endl;
    {
        const auto documents = search_server.FindTopDocuments("пушистый ухоженный кот"s);
        for (const Document& document : documents) {
            PrintDocument(document);
        }

    }

    {
        const auto documents = search_server.FindTopDocuments(
                "пушистый ухоженный кот"s,
                [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
        cout << "ACTUAL:"s << endl;
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }

    {
        const auto documents = search_server.FindTopDocuments(
            "пушистый ухоженный кот"s,
            [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        cout << "Even ids:"s << endl;
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }

    return 0;

}
