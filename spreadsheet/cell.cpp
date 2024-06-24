#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <stack>
#include <string>
#include <optional>

class Cell::Impl {
public:
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual bool CacheNotEmpty() const { return true; };                        //Проверка валидности кэша
    virtual void ClearCache() {};                                               //Очистка кэша
    virtual std::vector<Position> GetReferencedCells() const { return {}; }     //Метод для получение всех зависимых ячеек
    ~Impl() = default;
};


//Класс пустой ячейки. Возвращает пустую строку.
class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override {
        return "";
    }

    std::string GetText() const override {
        return "";
    }
};


//Класс текстовой ячейки
class Cell::TextImpl : public Impl {
public:
    TextImpl(std::string text)
        : text_(std::move(text))
    {
        //may be throw
        if (text_.empty()) {
            throw std::logic_error("No text");
        }
    }

    Value GetValue() const override {

        if (text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        else {
            return text_;
        }

    }

    std::string GetText() const override {
        return text_;
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    FormulaImpl(std::string formula, const SheetInterface& sheet)
        : sheet_(sheet) {
        if (formula.empty() || formula[0] != FORMULA_SIGN) {
            throw std::logic_error("Empty formula or missing formula sign ");
        }
        formulaPtr_ = ParseFormula(formula.substr(1));
    }

    Value GetValue() const override {
        if (!cache_.has_value()) cache_ = formulaPtr_->Evaluate(sheet_);            //Запишем в кэш, если его нет

        auto tmpValue = formulaPtr_->Evaluate(sheet_);
        if (std::holds_alternative<double>(tmpValue)) {
            return std::get<double>(tmpValue);                                      //Вернём формулу
        }
        return std::get<FormulaError>(tmpValue);                                    //Ошибки в парсинге
    }

    std::string GetText() const override {
        return FORMULA_SIGN + formulaPtr_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const {
        return formulaPtr_->GetReferencedCells();
    }

    void ClearCache() override {
        cache_.reset();
    }
    bool CacheNotEmpty() const override {
        return cache_.has_value();
    }

private:
    std::unique_ptr<FormulaInterface> formulaPtr_;          //Указатель на формулу
    const SheetInterface& sheet_;                           //Константная ссыль на таблицу
    mutable std::optional<FormulaInterface::Value> cache_;  //Кэш
};


Cell::Cell(Sheet& sh) : impl_(std::make_unique<EmptyImpl>()), sheet_{ sh } {}	//Конструктор пустой ячейки

Cell::~Cell() {}											                    //Деструктор

void Cell::Set(std::string text) {

    InvalidateCache();                                                          //Инвалидируем кэш
    
    
    if (text.size() == 0) {									                    //Если передали пустой текст, то создаём пустую ячейку(указатель)
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text[0] == FORMULA_SIGN) {			            //Передали формулу, создаем формулу
        std::unique_ptr<Impl> tmpImpl;
        tmpImpl = std::make_unique<FormulaImpl>(std::move(text), sheet_);

        //Проверка на цикличность
        if (CircularDependency(*tmpImpl)) {
            throw CircularDependencyException("This cell contains a cyclic formula");
        }

        for (Cell* tmpRefCell : dependOnOther) {
            tmpRefCell->dependToThis.erase(this);                           //Удаляем старые зависимости, т.к. у ячейки будет новое значение
        }

        dependOnOther.clear();                                              //Очистка сета зависимых ячеек

        //Создание списка зависимых ячеек
        for (const auto& pos : tmpImpl->GetReferencedCells()) {

            Cell* tmpRefCell = sheet_.GetCellPointer(pos);                  //Берем из таблицы зависимую ячейку(указатель)

            if (tmpRefCell == nullptr) {                                    //Если такой ячейки в таблице нет
                sheet_.SetCell(pos, "");                                    //Добавим её
                tmpRefCell = sheet_.GetCellPointer(pos);                    //Перекинем указатель на новую ячейку
            }

            dependOnOther.insert(tmpRefCell);                               //Закинем этот указатель в сет зависимых ячеек
            tmpRefCell->dependToThis.insert(this);                          //Закинем в сет зависимой ячейки текущую ячейку, чтоб ей указать, что она зависит от текущей
        }

        impl_ = std::move(tmpImpl);
    }
    else {													                //Обычный текст
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }


}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();					                //Очистка = перевод в пустую ячейку
}


void Cell::InvalidateCache(void) {
    if (impl_->CacheNotEmpty()) {
        impl_->ClearCache();
        /*Возможно надо инвалидировать зависимости, тогда придётся сделать рекурсивной*/
    }
}
bool Cell::CircularDependency(const Impl& inputImpl) const {

    if (inputImpl.GetReferencedCells().empty()) {
        return false;                                           //Нету зависимых ячеек
    }

    std::unordered_set<const Cell*> refCells;                   //Сет зависимых ячеек. Нужны только уникальные, т.к. их достаточно, чтоб уловить цикличность

    for (const auto& pos : inputImpl.GetReferencedCells()) {
        refCells.insert(sheet_.GetCellPointer(pos));            //Наполнение сета
    }
    
    std::unordered_set<const Cell*> checkedCells;               //Сет проверенных ячеек
    std::stack<const Cell*> needCheckCells;                     //Стек с ячейками которые надо проверить

    needCheckCells.push(this);                       

    while (needCheckCells.empty() == false) {
        const Cell* curCell = needCheckCells.top();
        needCheckCells.pop();
        checkedCells.insert(curCell);

        if (refCells.find(curCell) != refCells.end()) return true;

        for (const Cell* cell : curCell->dependToThis) {
            if (checkedCells.find(cell) == checkedCells.end()) needCheckCells.push(cell);
        }
    }

    return false;
}

Cell::Value Cell::GetValue() const { return impl_->GetValue(); }
std::string Cell::GetText() const { return impl_->GetText(); }
std::vector<Position> Cell::GetReferencedCells() const { return impl_->GetReferencedCells(); }
bool Cell::IsReferenced() const { return !dependToThis.empty();}