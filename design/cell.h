#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <variant>

class Cell : public CellInterface {
  public:
    Cell();
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

  private:
    //можете воспользоваться нашей подсказкой, но это необязательно.
    /*    class Impl;
        class EmptyImpl;
        class TextImpl;
        class FormulaImpl;
        std::unique_ptr<Impl> impl_;
    */
    struct FormulaValue {
        std::unique_ptr<FormulaInterface> formula;
        SheetInterface &sheet;
    };

    using ValueImpl = std::variant<std::monostate, std::string, FormulaValue>;

    ValueImpl value_;
    std::optional<Value> cache_;
};