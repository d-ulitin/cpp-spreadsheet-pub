#include "common.h"
#include "test_runner_p.h"

#include "sheet.h"

#include <algorithm>
#include <string>
#include <random>

using namespace std;

void TestIndexedStorage() {
    {
        IndexedStorage<string> storage;

        ASSERT(storage.Empty());

        storage[1] = "x"s;
        ASSERT(!storage.Empty());
        ASSERT_EQUAL(storage.Count(1), 1);
        ASSERT_EQUAL(storage.Size(), 1);
        ASSERT_EQUAL(storage[1], "x"s);
        ASSERT_EQUAL(storage.At(1), "x"s);

        storage[1] = "1"s;
        ASSERT(!storage.Empty());
        ASSERT_EQUAL(storage.Size(), 1);
        ASSERT_EQUAL(storage[1], "1"s);
        ASSERT_EQUAL(storage.At(1), "1"s);

        try {
            storage.At(2);
            ASSERT(false);
        } catch (...) {}

        // default contructed value for new index
        ASSERT_EQUAL(storage[0], ""s);
        ASSERT_EQUAL(storage.Size(), 2);
        ASSERT_EQUAL(storage.Count(0), 1);

        // overwrite
        storage[0] = "0"s;
        ASSERT_EQUAL(storage.Count(0), 1);
        ASSERT_EQUAL(storage.Size(), 2);
        ASSERT_EQUAL(storage[0], "0"s);
        ASSERT_EQUAL(storage[1], "1"s);

        // clear
        storage.Clear();
        ASSERT_EQUAL(storage.Size(), 0);
        ASSERT_EQUAL(storage.Count(0), 0);

        // iterator
        storage[1] = "1";
        storage[0] = "0";
        ASSERT_EQUAL(storage.Size(), 2);
        {
            auto i = storage.begin();
            assert((*i++ == std::pair<const int, string>(0, "0")));
            assert((*i++ == std::pair<const int, string>(1, "1")));
            assert(i == storage.end());
        }

        // Erase
        storage.Erase(0);
        ASSERT_EQUAL(storage.Size(), 1);
        ASSERT_EQUAL(storage.Count(0), 0);
        ASSERT_EQUAL(storage.Count(1), 1);
        {
            auto i = storage.begin();
            assert((*i++ == std::pair<const int, string>(1, "1")));
            assert(i == storage.end());
        }

        auto i = storage.IndicesBegin();
        ASSERT_EQUAL(*i++, 1);
        ASSERT(i == storage.IndicesEnd());

        // Random
        storage.Clear();
        vector<int> idx{1000, -1};
        iota(idx.begin(), idx.end(), 0);
        for (auto v : idx)
            storage[v] = to_string(v);
        shuffle(idx.begin(), idx.end(), std::mt19937{});
        for (auto v : idx) {
            ASSERT_EQUAL(storage.Count(v), 1);
            ASSERT_EQUAL(storage[v], to_string(v));
            storage.Erase(v);
            ASSERT_EQUAL(storage.Count(v), 0);
        }
        ASSERT(storage.Empty());
    }
}

// int main() {
//     TestRunner tr;
//     RUN_TEST(tr, TestIndexedStorage);
// }