#pragma once

#include "cell.h"
#include "common.h"

#include <cassert>
#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/*
 * Sheet storage abstraction levels:
 * 1. IndexedStorage: ordered index-value storage.
 * 2. SheetStorage: generalized 2D storage with multidimensional operations.
 * 3. Sheet: implementation of SheetInterface for CellInterface storage.
 */


/**
 * Sparse indexed storage.
 * 
 * Optimized for O(1) access complexity.
 */
template<typename T, typename Index = int>
class IndexedStorage final {

public:

    using DataMap = std::unordered_map<Index, T>;
    using IndicesContainer = std::vector<Index>;

    // all constructors and operators are default

    T& operator[](Index index) {
        auto it = data_.find(index);
        if (it == data_.end()) {
            // insert new index
            auto indices_it = std::lower_bound(indices_.begin(), indices_.end(), index);
            assert(indices_it == indices_.end() || *indices_it != index);
            indices_.insert(indices_it, index);
        }
        return data_[index];
    }

    const T& operator[](Index index) const {
        return const_cast<IndexedStorage&>(*this)[index];
    }

    T& At(Index index) {
        auto it = data_.find(index);
        if (it == data_.end()) {
            throw std::invalid_argument("index not found");
        }
        return it->second;
    }

    const T& At(Index index) const {
        return const_cast<IndexedStorage&>(*this)[index];
    }

    bool Empty() const noexcept {
        assert((data_.empty() && indices_.empty()) || (!data_.empty() && !indices_.empty()));
        return data_.empty();
    }

    Index Size() const noexcept {
        assert(data_.size() == indices_.size());
        return data_.size();
    }

    void Clear() noexcept {
        data_.clear();
        indices_.clear();
    }

    void Swap(IndexedStorage& other) noexcept {
        data_.swap(other.data_);
        indices_.swap(other.indices_);
    }

    Index Count(Index index) const noexcept {
        return data_.count(index);
    }

    void Erase(Index index) {
        auto data_it = data_.find(index);
        if (data_it == data_.end()) {
            throw std::invalid_argument("index not found");
        }
        auto index_it = std::lower_bound(indices_.begin(), indices_.end(), index);
        assert(index_it != indices_.end());
        data_.erase(data_it);
        indices_.erase(index_it);
        assert(data_.size() == indices_.size());
    }

    void Shrink() {
        data_.rehash(data_.size());
        indices_.shrink_to_fit();
    }

    // indices access

    int FrontIndex() const {
        assert(!indices_.empty());
        return indices_.front();
    }

    int BackIndex() const {
        assert(!indices_.empty());
        return indices_.back();
    }

    auto IndicesBegin() const noexcept {
        return indices_.cbegin();
    }

    auto IndicesEnd() const noexcept {
        return indices_.cend();
    }

    // value access

    // Base iterator class for non-const and const iterators
    // DataMapForIter can be DataMap or const DataMap
    template<typename DataMapForIter>
    class IteratorBase;

    // non-const iterator
    class Iterator : public IteratorBase<DataMap> {
    public:
        using IteratorBase<DataMap>::IteratorBase;
        friend class ConstIterator;  // fields access in converting constructor
    };

    // const iterator
    class ConstIterator : public IteratorBase<const DataMap> {
    public:
        using IteratorBase<const DataMap>::IteratorBase;
        ConstIterator(Iterator& it) : IteratorBase<const DataMap>(it.data_, it.indices_iter_) {}
    };

    Iterator begin() {
        return Iterator{data_, indices_.begin()};
    }

    ConstIterator begin() const {
        return ConstIterator{data_, indices_.begin()};
    }

    Iterator end() {
        return Iterator{data_, indices_.end()};
    }

    ConstIterator end() const {
        return ConstIterator{data_, indices_.end()};
    }

private:

    DataMap data_;                  // access by index
    IndicesContainer indices_;      // indices in ascending order
};

template<typename T, typename Index>
template<typename DataMapForIter>
class IndexedStorage<T, Index>::IteratorBase {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = typename DataMapForIter::value_type;
    using refernce          = typename DataMapForIter::reference;
    using difference_type   = typename DataMapForIter::difference_type;
    using pointer           = typename DataMapForIter::pointer;

    IteratorBase(DataMapForIter &data, typename IndicesContainer::const_iterator indices_iter) :
    data_(data), indices_iter_(indices_iter) {
        //
    }

    // iterators must be CopyConstructible, CopyAssignable, and Destructible [iterator.iterators]
    IteratorBase(const IteratorBase&) = default;
    IteratorBase& operator=(const IteratorBase&) = default;

    // auto for non-const and const return value_type
    auto& operator*() {
        auto it = data_.find(*indices_iter_);
        assert(it != data_.end());
        return *it;
    }

    IteratorBase& operator++() {
        ++indices_iter_;
        return *this;
    }
    
    IteratorBase operator++(int) {
        auto tmp = IteratorBase{*this};
        ++(*this);
        return tmp;
    }

    IteratorBase& operator--() {
        --indices_iter_;
        return *this;
    }

    IteratorBase operator--(int) {
        auto tmp = IteratorBase{*this};
        --(*this);
        return tmp;
    }

    IteratorBase& operator+=(Index n) {
        indices_iter_ += n;
        return *this;
    }

    IteratorBase& operator-=(Index n) {
        indices_iter_ -= n;
        return *this;
    }

    IteratorBase operator+(Index n) const {
        return IteratorBase{data_, indices_iter_ + n};
    }

    friend IteratorBase operator+(Index n, const IteratorBase& i) {
        return IteratorBase{i.data_, i.indices_iter_ + n};
    }

    IteratorBase operator-(Index n) const {
        return IteratorBase{data_, indices_iter_ - n};
    }

    friend Index operator-(const IteratorBase& b, const IteratorBase& a) {
        return b.indices_iter_ - a.indices_iter_;
    }

    auto operator[](Index index) {
        return *(this + index);
    }

    bool operator==(const IteratorBase& other) {
        return indices_iter_ == other.indices_iter_;
    }

    bool operator!=(const IteratorBase& other) {
        return !(*this == other);
    }

    bool operator<(const IteratorBase& other) {
        return indices_iter_ < other.indices_iter_;
    }

    bool operator>(const IteratorBase& other) {
        return !(*this < other || *this == other);
    }

    bool operator<=(const IteratorBase& other) {
        return !(*this > other);
    }

    bool operator>=(const IteratorBase& other) {
        return !(*this < other);
    }

protected:
    DataMapForIter& data_;
    typename IndicesContainer::const_iterator indices_iter_;
}; // class IteratorBase

/**
 * Sheet storage. Generalized sheet storage for any sheet data.
 *
 * Only non-empty data is stored.
 * Small memory footprint regardless of the cells position.
 * Fast iterating over cells.
 * Access complexity is O(1).
 *
 * Can be used to store any sheet information: formulas, formatting, comments, names,
 * protection status, validity status etc. Might make sense to store it in separete "layers".
 */
template<typename T>
class SheetStorage {

public:

    using Storage2D = IndexedStorage<IndexedStorage<T>>;

    virtual ~SheetStorage() = default;

    void CheckValid(Position pos) const {
        if (!pos.IsValid())
            throw InvalidPositionException("invalid position");
    }

    int Count() const {
        int count = 0;
        for(const auto& row : rows_) {
            count += row.Size();
        }
        return count;
    }

    template <typename FT>
    void Set(Position pos, FT&& data) {
        CheckValid(pos);
        rows_[pos.row][pos.col] = std::forward<FT>(data);
    }

    const T* Get(Position pos) const {

        CheckValid(pos);

        if (rows_.Count(pos.row) == 0)
            return nullptr;

        const auto& cells_row = rows_[pos.row];

        if (cells_row.Count(pos.col) == 0)
            return nullptr;

        return &cells_row[pos.col];
    }

    void Clear() {
        rows_.Clear();
    }

    void Clear(Position pos) {
        CheckValid(pos);
        if (rows_.Count(pos.row) > 0) {
            auto& row = rows_[pos.row];
            if (row.Count(pos.col) > 0) {
                row.Erase(pos.col);
                if (row.Empty()) {
                    rows_.Erase(pos.row);
                }
            }
        }
    }

    Size GetPrintableSize() const {
        Size size;
        if (!rows_.Empty()) {
            size.rows = rows_.BackIndex() + 1;
            for (auto& row : rows_) {
                if (!row.second.Empty()) {
                    int cols = row.second.BackIndex() + 1;
                    size.cols = std::max(size.cols, cols);
                }
            }
        }
        return size;
    }

    Storage2D& Rows() noexcept {
        return rows_;
    }

    const Storage2D& Rows() const noexcept {
        return rows_;
    }

private:

    Storage2D rows_; // cells ctorage; first dimension is row index,
                     // second dimension is column index
}; // SheetStorage

class Sheet : public SheetInterface {
public:
    using CellPtr = std::unique_ptr<CellInterface>;

    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface *GetCell(Position pos) const override;
    CellInterface *GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream &output) const override;
    void PrintTexts(std::ostream &output) const override;

private:
    
    template<typename Printer>
    void Print(std::ostream &output, Printer printer) const;

    SheetStorage<CellPtr> cells_;

    // value is set of cells, that has reference to key
    std::unordered_map<Position, std::unordered_set<Position>> refs_from_;
};