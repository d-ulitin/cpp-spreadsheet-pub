#pragma once

#include "common.h"
#include "formula.h"

#include <variant>
#include <optional>
#include <string>
#include <memory>

class Sheet;

class Cell : public CellInterface {
  public:
    Cell(Sheet &sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    // CellInterface implementation
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    void InvalidateCache() const;

  private:
    struct FormulaValueImpl {
        std::unique_ptr<FormulaInterface> formula;
        const SheetInterface *sheet;
    };

    using ValueImpl = std::variant<std::monostate, std::string, FormulaValueImpl>;

    SheetInterface& sheet_;
    ValueImpl value_;

    // Cache

    // Caching CellInterface::Value is simple. It can be done with respect to
    // single responsibility principle (SOLID). But it can result to duplicating every
    // string containing in cell.
    // So cache _only formula results_ and realize it inside Cell class.
    // Use variant to make it possible to add other chached types if needed.
    // monostate means no cached value
    using CachedValue = std::variant<std::monostate, FormulaInterface::Value>;

    // Caching keeps logical constantness
    mutable CachedValue cache_;
};
