#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <deque>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

using namespace std::literals;

Sheet::Sheet() {
}

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    // create new cell
    auto cell = std::make_unique<Cell>(*this);
    cell->Set(std::move(text));

    // get existing cell references
    std::unordered_set<Position> old_refs;
    auto* old_cell = GetCell(pos);
    if (old_cell) {
        const auto old_refs_vec = old_cell->GetReferencedCells();
        old_refs.insert(old_refs_vec.begin(), old_refs_vec.end());
    }

    // get new cell references
    const auto new_refs_vec = cell->GetReferencedCells();
    std::unordered_set<Position> new_refs{new_refs_vec.begin(), new_refs_vec.end()};

    UpdateRefs(pos, old_refs, new_refs);

    // create empty cell for every reference (needed to pass tests)
    for (const auto& ref: new_refs) {
        if (!GetCell(ref))
            SetCell(ref, std::string{});
    }

    // set new cell value
    cells_.Set(pos, std::move(cell));
    InvalidateCache(pos);
}

const CellInterface *Sheet::GetCell(Position pos) const {
    return GetCellImpl(pos);
}

CellInterface *Sheet::GetCell(Position pos) {
    return GetCellImpl(pos);
}

const Cell *Sheet::GetCellImpl(Position pos) const {
    const CellPtr *cell_ptr = cells_.Get(pos);
    if (cell_ptr)
        return cell_ptr->get();
    else
        return nullptr;
}

Cell *Sheet::GetCellImpl(Position pos) {
    const CellPtr *cell_ptr = cells_.Get(pos);
    if (cell_ptr)
        return cell_ptr->get();
    else
        return nullptr;
}

void Sheet::ClearCell(Position pos) {
    Cell *cell = GetCellImpl(pos);
    if (cell) {
        const auto refs = cell->GetReferencedCells();
        UpdateRefs(pos, {refs.begin(), refs.end()}, {});
        cells_.Clear(pos);
    }
}

Size Sheet::GetPrintableSize() const {
    return cells_.GetPrintableSize();
}

static std::ostream &operator<<(std::ostream &output,
                                const CellInterface::Value &value) {
    std::visit([&](const auto &x) { output << x; }, value);
    return output;
}

void Sheet::PrintValues(std::ostream &output) const {
    Print(output,
        [](std::ostream &output, const CellInterface *cell) {
            output << cell->GetValue();});
}

void Sheet::PrintTexts(std::ostream &output) const {
    Print(output,
        [] (std::ostream &output, const CellInterface *cell) {
            output << cell->GetText(); });
}

template<typename Printer>
void Sheet::Print(std::ostream &output, Printer printer) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            const CellPtr *cell = cells_.Get({row, col});
            if (cell) {
                printer(output, cell->get());
            }
        }
        output << '\n';
    }
}

void Sheet::UpdateRefs(Position pos,
                       const std::unordered_set<Position>& old_refs,
                       const std::unordered_set<Position>& new_refs) {

    // compare old and new references, make added references set
    std::unordered_set<Position> refs_add;
    for (const auto new_ref: new_refs) {
        if (old_refs.count(new_ref) == 0) {
            // reference from pos to new_ref doesn't exists
            assert(refs_from_.count(new_ref) == 0 || refs_from_.at(new_ref).count(pos) == 0);
            refs_add.insert(new_ref);
        } else {
            // reference from pos to new_ref alredy exists, no action needed
            assert(refs_from_.count(new_ref) > 0 && refs_from_.at(new_ref).count(pos) > 0);
        }
    }

    // compare old and new references make, removed references set
    std::unordered_set<Position> refs_del;
    for (const auto old_ref: old_refs) {
        if (new_refs.count(old_ref) == 0) {
            // reference from pos to old_ref is removing
            assert(refs_from_.count(old_ref) > 0 && refs_from_.at(old_ref).count(pos) > 0);
            refs_del.insert(old_ref);
        } else {
            // reference from pos to old_ref doesn't exists
            assert(refs_from_.count(old_ref) == 0 || refs_from_.at(old_ref).count(pos) == 0);
        }
    }

    // check added references for cycles
    for (auto ref_add : refs_add) {
        if (IsCircularReference(pos, ref_add, refs_del)) {
            throw CircularDependencyException("Circular reference "s + pos.ToString() +
                                              " to "s + ref_add.ToString());
        }
    }

    // add refs
    for (auto ref_add: refs_add) {
        refs_from_[ref_add].insert(pos);
    }

    // remove refs
    for (auto ref_del: refs_del) {
        auto it = refs_from_.find(ref_del);
        assert(it != refs_from_.end());
        assert(it->second.count(pos) > 0);
        it->second.erase(pos);
        if (it->second.size() == 0)
            refs_from_.erase(it);
    }
}

bool Sheet::IsCircularReference(Position pos,
                                Position ref_add,
                                const std::unordered_set<Position>& refs_del) const {

    assert(refs_del.count(pos) == 0);
    assert(refs_del.count(ref_add) == 0);
    assert(refs_from_.count(ref_add) == 0 || refs_from_.find(ref_add)->second.count(pos) == 0);

    // DFS
    std::unordered_set<Position> discovered;
    std::vector<Position> queue{pos, ref_add}; // after ref_add popped, pos becomes one of adjacents

    while (!queue.empty()) {
        auto v = queue.back();
        queue.pop_back();
        if (discovered.count(v) > 0)
            return true;
        discovered.insert(v);

        auto it = refs_from_.find(v);
        if (it != refs_from_.end()) {
            for (auto refs_to_v : it->second) {
                // skip cells to be deleted
                if (refs_to_v == pos && refs_del.count(v) > 0)
                    continue;
                queue.push_back(refs_to_v);
            }
        }
    }
    return false;
}


void Sheet::InvalidateCache(Position pos) const {

    // DFS
    std::unordered_set<Position> discovered;
    std::vector<Position> queue{pos};

    while (!queue.empty()) {
        Position v = queue.back();
        queue.pop_back();

        const Cell* cell = GetCellImpl(v);
        if (cell)
            cell->InvalidateCache();
        discovered.insert(v);

        auto it = refs_from_.find(v);
        if (it != refs_from_.end()) {
            for (auto refs_to_v : it->second) {
                if (discovered.count(refs_to_v) == 0)
                    queue.push_back(refs_to_v);
            }
        }
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}