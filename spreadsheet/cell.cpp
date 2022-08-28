#include "cell.h"
#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <cassert>
#include <exception>
#include <iostream>
#include <optional>
#include <variant>

using namespace std;

Cell::Cell(Sheet &sheet) : sheet_(sheet) {
}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    if (text.empty()) {
        // empty cell
        Clear();
    } else if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        // formula
        try {
            string expression{text.begin() + 1, text.end()};
            value_ = FormulaValueImpl{ParseFormula(expression), &sheet_};
        } catch (const std::exception &exc) {
            std::throw_with_nested(FormulaException(exc.what()));
        }
    } else {
        // text (numbers stored as text)
        value_ = move(text);
    }
}

void Cell::Clear() {
    value_ = std::monostate{};
    cache_ = std::monostate{};
}

struct FormulaValueVisitor {
    template <typename T> Cell::Value operator()(T val) { return val; }
};

Cell::Value Cell::GetValue() const {

    struct CellValueVisitor {

        explicit CellValueVisitor(const Cell& cell) : cell_(cell) {}

        // empty cell
        Cell::Value operator()(std::monostate) {
            return string{};
        }

        // text cell
        Cell::Value operator()(const std::string &str) {
            if (str.size() > 0 && str.front() == ESCAPE_SIGN) {
                return string{str.begin() + 1, str.end()};
            } else {
                return str;
            }
        }

        // formula cell; use caching
        Cell::Value operator()(const FormulaValueImpl& fvalue) {
            if (holds_alternative<FormulaInterface::Value>(cell_.cache_)) {
                return std::visit(FormulaValueVisitor{}, get<FormulaInterface::Value>(cell_.cache_));
            } else {
                FormulaInterface::Value val = fvalue.formula->Evaluate(*fvalue.sheet);
                cell_.cache_ = val;
                return std::visit(FormulaValueVisitor{}, std::move(val));
            }
        }

        const Cell& cell_;
    };

    return std::visit(CellValueVisitor(*this), value_);
}


std::string Cell::GetText() const {

    struct CellTextVisitor {

        string operator()(std::monostate) { return string(); }

        string operator()(std::string str) { return str; }

        string operator()(const FormulaValueImpl& fvalue) {
            return string{FORMULA_SIGN} + fvalue.formula->GetExpression();
        }
    };

    return std::visit(CellTextVisitor{}, value_);
}

std::vector<Position> Cell::GetReferencedCells() const {
    if(holds_alternative<FormulaValueImpl>(value_))
        return get<FormulaValueImpl>(value_).formula->GetReferencedCells();
    else
        return {};
}


void Cell::InvalidateCache() const {
    cache_ = std::monostate{};
}