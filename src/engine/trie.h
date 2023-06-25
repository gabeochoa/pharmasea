

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "assert.h"
#include "util.h"  // for split

struct Trie {
    struct Node {
        char letter;
        bool is_end = false;
        std::map<char, Node> children;
        Node() : letter(0) {}
        explicit Node(char c) : letter(c) {}
    };

    Node root;

    Trie() { reset(); }

    void add(const std::string& phrase) { add_(root, phrase); }

    bool contains(const std::string& phrase) { return contains_(root, phrase); }

    [[nodiscard]] std::vector<std::string> dump(Node cur,
                                                const std::string& prefix) {
        std::vector<std::string> results;

        std::vector<std::pair<std::string, Node> > q;
        q.push_back(std::make_pair(prefix, cur));
        while (!q.empty()) {
            auto qtop = q.front();
            q.erase(q.begin());
            if (qtop.second.is_end) {
                results.push_back(qtop.first);
            }
            for (auto kv : qtop.second.children) {
                q.push_back(std::make_pair(qtop.first + kv.first, kv.second));
            }
        }
        return results;
    }

    [[nodiscard]] std::vector<std::string> dump(
        const std::string& prefix = "") {
        auto cur = subtrie(root, prefix);
        if (cur.has_value()) return dump(*cur, prefix);
        return std::vector<std::string>();
    }

   private:
    void add_(Node& cur, const std::string& phrase) {
        if (phrase.empty()) {
            cur.is_end = true;
            return;
        }
        char c = phrase[0];
        if (cur.children.find(c) == cur.children.end())
            cur.children.insert(std::make_pair(c, Node(c)));
        add_(cur.children.at(c), phrase.substr(1));
    }

    void reset() { root = Node(0); }

    std::optional<Node> subtrie(Node cur, const std::string& phrase) {
        if (phrase.empty()) return Node(cur);
        char c = phrase[0];
        if (cur.children.find(c) == cur.children.end())
            return std::optional<Node>();
        return subtrie(cur.children.at(c), phrase.substr(1));
    }

    bool contains_(Node cur, const std::string& phrase) {
        auto resp = subtrie(cur, phrase);
        if (resp.has_value()) {
            return resp->is_end;
        }
        return false;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Trie::Node& obj) {
    os << "me: " << obj.letter << "\n";
    for (auto kv : obj.children) os << kv.second;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Trie& obj) {
    os << obj.root;
    return os;
}

inline void test_trie_insert_once() {
    Trie trie;
    trie.add("hello");
}

inline void test_trie_insert_once_and_found() {
    Trie trie;
    trie.add("hello");
    VALIDATE(trie.contains("hello"), "trie should contain hello");
}

inline void test_trie_insert_once_missing() {
    Trie trie;
    trie.add("hello");
    VALIDATE(!trie.contains("hellp"),
             "trie should contain hello and not hellp");
}

inline void test_trie_insert_once_and_dump() {
    Trie trie;
    std::string hello = "hello";
    trie.add(hello);

    std::vector<std::string> results = trie.dump();

    VALIDATE(results.size() == 1, "Should have a single string in results");
    VALIDATE(results[0] == hello, "and results should have hello ");
}

inline void test_trie_insert_ten() {
    Trie trie;

    std::string sent("hello my name is john and I love you");
    auto words = util::split_re(sent, " ");
    for (auto word : words) trie.add(word);
    trie.add("a lot");
}

inline void test_trie_insert_ten_and_found() {
    Trie trie;

    std::string sent("hello my name is john and I love you");
    auto words = util::split_re(sent, " ");
    for (auto word : words) trie.add(word);
    trie.add("a lot");

    for (auto word : words) {
        VALIDATE(trie.contains(word), "Tree should contain all the words");
    }
    VALIDATE(trie.contains("a lot"), "Tree should contain the last word");
}

inline void test_trie_insert_ten_and_dump() {
    Trie trie;

    std::string sent("hello my name is john and I love you");
    auto words = util::split_re(sent, " ");
    for (auto word : words) trie.add(word);
    trie.add("a lot");

    auto results = trie.dump();

    VALIDATE(results.size() == 10, "Tree dump should contain all the words");
}

inline void test_trie_insert_ten_similar_and_dump_prefix() {
    Trie trie;

    std::string sent(
        "angel anger angle anglo angry angst anime anise annex anvil");
    auto words = util::split_re(sent, " ");
    for (auto word : words) trie.add(word);

    auto results = trie.dump();
    VALIDATE(results.size() == 10, "Tree dump should contain all the words");

    results = trie.dump("ang");
    VALIDATE(results.size() == 6,
             "Tree dump should contain all the words starting with ang ");

    results = trie.dump("angl");
    VALIDATE(results.size() == 2,
             "Tree dump should contain all the words starting with ang ");
}

inline void test_trie() {
    test_trie_insert_once();
    test_trie_insert_once_and_found();
    test_trie_insert_once_missing();
    test_trie_insert_once_and_dump();
    test_trie_insert_ten();
    test_trie_insert_ten_and_found();
    test_trie_insert_ten_and_dump();
    test_trie_insert_ten_similar_and_dump_prefix();
}
