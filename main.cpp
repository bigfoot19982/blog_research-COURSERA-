#include "test_runner.h"
#include "profile.h"

#include <map>
#include <string>
#include <vector>
#include <future>
using namespace std;

template <typename Iterator>
class IteratorRange {
public:
  IteratorRange(Iterator begin, Iterator end)
    : first(begin)
    , last(end)
    , size_(distance(first, last))
  {
  }

  Iterator begin() const {
    return first;
  }

  Iterator end() const {
    return last;
  }

  size_t size() const {
    return size_;
  }

private:
  Iterator first, last;
  size_t size_;
};

template <typename Iterator>
class Paginator {
private:
  vector<IteratorRange<Iterator>> pages;

public:
  Paginator(Iterator begin, Iterator end, size_t page_size) {
    for (size_t left = distance(begin, end); left > 0; ) {
      size_t current_page_size = min(page_size, left);
      Iterator current_page_end = next(begin, current_page_size);
      pages.push_back({begin, current_page_end});

      left -= current_page_size;
      begin = current_page_end;
    }
  }

  auto begin() const {
    return pages.begin();
  }

  auto end() const {
    return pages.end();
  }

  size_t size() const {
    return pages.size();
  }
};

template <typename C>
auto Paginate(C& c, size_t page_size) {
  return Paginator<typename C::iterator>(begin(c), end(c), page_size);
}

struct Stats {
    map<string, int> word_frequences;

    void operator += (const Stats& other) {
        for (auto it = other.word_frequences.begin(); it != other.word_frequences.end(); ++it) {
            word_frequences[(*it).first] += (*it).second;
        }
    }
};

Stats Explore(IteratorRange<vector<string>::iterator>& page, const set<string>& key_words) {
    Stats cur;
    for (const auto& str : page) {
        string now = "";
        for (size_t i = 0; i <= str.size(); ++i) {
            if (i < str.size() && str[i] != ' ') now += str[i];
            if (i == str.size() || str[i] == ' ') {
                if (now != "" && key_words.find(now) != key_words.end()) {
                    cur.word_frequences[now]++;
                }
                now = "";
            }
        }
    }
    return cur;
}

Stats ExploreKeyWords(const set<string>& key_words, istream& input) {
    string s;
    vector<string> vec;
    while (getline(input, s, '\n')) {
        vec.push_back(s);
    }
    Stats stat;
    vector<IteratorRange<vector<string>::iterator>> pages;
    for (auto& page : Paginate(vec, 2000)) pages.push_back(page);
    vector<future<Stats>> tmp;
    for (auto& page : pages) {
        tmp.push_back(
            async(Explore, ref(page), ref(key_words))
        );
    }
    for (auto& i : tmp) {
        stat += i.get();
    }
    return stat;
}

void TestBasic() {
  const set<string> key_words = {"yangle", "rocks", "sucks", "all"};

  stringstream ss;
  ss << "this new yangle service really rocks\n";
  ss << "It sucks when yangle isn't available\n";
  ss << "10 reasons why yangle is the best IT company\n";
  ss << "yangle rocks others suck\n";
  ss << "Goondex really sucks, but yangle rocks. Use yangle\n";

  const auto stats = ExploreKeyWords(key_words, ss);
  const map<string, int> expected = {
    {"yangle", 6},
    {"rocks", 2},
    {"sucks", 1}
  };
  ASSERT_EQUAL(stats.word_frequences, expected);
}

int main() {
    TestRunner tr;
    RUN_TEST(tr, TestBasic);
}
