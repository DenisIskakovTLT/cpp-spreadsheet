#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <memory>
#include <unordered_map>

class Hasher {
public:
    size_t operator()(const Position p) const {
        return std::hash<std::string>()(p.ToString()) * 666;
    }
};

class Sheet : public SheetInterface {
public:
    using Table = std::unordered_map<Position, std::unique_ptr<Cell>, Hasher>;

    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    // Можете дополнить ваш класс нужными полями и методами
    const Cell* GetCellPointer(Position pos) const;
    Cell* GetCellPointer(Position pos);

private:
    // Можете дополнить ваш класс нужными полями и методами
    Table sheet_;
};