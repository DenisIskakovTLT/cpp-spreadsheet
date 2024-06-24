#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sh);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    bool IsReferenced() const;          //Проверка на зависимость
    //void ClearCache();                // Внутри FormulaImpl
    //bool CacheIsEmpty() const;        // Внутри FormulaImpl

private:

    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    void InvalidateCache(void);							//Инвалидация кэша
    bool CircularDependency(const Impl& inputImpl) const;			//Проверка на цикличность формулы 

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    std::unordered_set<Cell*> dependOnOther;                        //Сет ячеек которые зависят от текущей
    std::unordered_set<Cell*> dependToThis;                         //Сет ячеек от которых зависит текущая
};