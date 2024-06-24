#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

FormulaError::FormulaError(Category category)
    : category_(category) 
{

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
        return "#REF!";
    case Category::Value:
        return "#VALUE!";
    case Category::Arithmetic:
        return "#ARITHM!";
    }
    return "";
}

namespace {
    class Formula : public FormulaInterface {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression)
        : ast_(ParseFormulaAST(expression)) {};

        Value Evaluate(const SheetInterface& sh) const override {
            const std::function<double(Position)> arguments = [&sh](const Position p)->double {                               
                if (!p.IsValid()) throw FormulaError(FormulaError::Category::Ref);                                      //Валидность ячейки

                const auto* tmpCell = sh.GetCell(p);                                                                    //Вытащим ячейку из таблицы 

                if (!tmpCell) {                                                                                         //По переданной позиции ячейка пуста
                    return 0; 
                }

                if (std::holds_alternative<double>(tmpCell->GetValue())) {                                              //Вытащим значение, если оно double
                    return std::get<double>(tmpCell->GetValue());
                }

                if (std::holds_alternative<std::string>(tmpCell->GetValue())) {                                         //Вытащим значние если оно string
                    auto tmpValue = std::get<std::string>(tmpCell->GetValue());
                    double tmpRes = 0;
                    if (!tmpValue.empty()) {
                        std::istringstream in(tmpValue);
                        if (!(in >> tmpRes) || !in.eof()) {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                    }
                    return tmpRes;
                }

                throw FormulaError(std::get<FormulaError>(tmpCell->GetValue()));                                        //В ячейке мусор

            };

            try {
                return ast_.Execute(arguments);
            }
            catch (const FormulaError& e) {
                return e;
            }
        }

        std::vector<Position> GetReferencedCells() const override {
            std::vector<Position> vCells;
            for (auto cell : ast_.GetCells()) {                             //проходим все ячейки в формуле
                if (cell.IsValid()) vCells.push_back(cell);                 //если валидная пушим в вектор
            }
            vCells.resize(std::unique(vCells.begin(), vCells.end()) - vCells.begin());
            return vCells;
        }
        
        std::string GetExpression() const override {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

    private:
       const FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (...) {
        throw FormulaException("Formula Parsing Error");
    }
}