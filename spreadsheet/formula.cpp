#include "formula.h"

#include "FormulaAST.h"
#include "common.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <exception>
#include <iterator>
#include <sstream>
#include <vector>

using namespace std::literals;

/**
 * FormulaError
 */

FormulaError::FormulaError(FormulaError::Category category) :
    category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
    case Category::Ref:
        return "#REF!"sv;
    case Category::Value:
        return "#VALUE!"sv;
    case Category::Div0:
        return "#DIV/0!"sv;
    default:
        assert(false);
        std::terminate();
    }
}

std::ostream &operator<<(std::ostream &output, FormulaError fe) {
    return output << fe.ToString();
}

/**
 * Formula
 */

namespace {

class Formula : public FormulaInterface {
  public:
    explicit Formula(const std::string& expression)
        : ast_(ParseFormulaAST(expression)) {
        ;
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        } catch (const FormulaError e) {
            return e;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream expression;
        ast_.PrintFormula(expression);
        return expression.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        const auto& cells = ast_.GetCells();
        std::vector<Position> unique;
        std::unique_copy(cells.begin(), cells.end(), std::back_inserter(unique));
        return unique;
    }

  private:
    FormulaAST ast_;
};

} // namespace

std::unique_ptr<FormulaInterface> ParseFormula(const std::string& expression) {
    try {
        return std::make_unique<Formula>(expression);
    } catch (std::exception& e) {
        std::throw_with_nested(FormulaException(e.what()));
    }
}