#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {

    if (pos.IsValid()) {                                                            //Проверка на валидность позиции

        const auto& tmpCell = sheet_.find(pos);                                     //Проверка на наличие ячейки в uMap

        if (tmpCell == sheet_.end()) {                                              //если find вернул end, то ячейка ни разу не заполнялась и её нужно создать
            sheet_.emplace(pos, std::make_unique<Cell>(*this));                     //Создаём
        }

        sheet_.at(pos)->Set(std::move(text));                                        //Мувим значение

    }
    else {
        throw InvalidPositionException("Position not valid!");                      //Исключение, передали неверные координаты ячейки
    }

}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellPointer(pos);  //Проверка на валидность позиции вунтри 
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetCellPointer(pos);//Проверка на валидность позиции вунтри 
}

void Sheet::ClearCell(Position pos) {
    if (pos.IsValid()) {
        const auto& tmpCell = sheet_.find(pos);                                     //Проверка на наличие ячейки в uMap

        if (tmpCell != sheet_.end() && tmpCell->second != nullptr) {                //Если упёрлись в end, значит такой ячейки нет. Если nullptr - значит пустая
            tmpCell->second->Clear();
            //Возможно надо стирать зависимости
            if (!tmpCell->second->IsReferenced()) {
                tmpCell->second.reset();
            }
        }
    }
    else {
        throw InvalidPositionException("Position not valid!");
    }

}

Size Sheet::GetPrintableSize() const {

    Size tmpRes{0, 0};

    if (sheet_.begin() == sheet_.end()) {       //таблица пустая.

        tmpRes.cols = 0;
        tmpRes.rows = 0;
        return tmpRes;
    }

    for (auto itr = sheet_.begin(); itr != sheet_.end(); itr++) {   //Крутимся по таблице

        /*Проверка на пустоту*/
        if (itr->second == nullptr) {
            continue; //Пустая ячейка, ничего не делаем
        }

        /*Тут ищем крайнюю граница по строкам и столбцам. В tmpRes аккумулируем */
        if (tmpRes.cols <= itr->first.col) {
            tmpRes.cols = itr->first.col + 1;
        }

        if (tmpRes.rows <= itr->first.row) {
            tmpRes.rows = itr->first.row + 1;
        }


    }
    return { tmpRes.rows , tmpRes.cols };
}

void Sheet::PrintValues(std::ostream& output) const {
    Size tmpSize = GetPrintableSize(); //Печатное окно

    /*2 цикла, по строкам и столбцам*/
    for (int row = 0; row < tmpSize.rows; row++) {

        for (int col = 0; col < tmpSize.cols; ++col) {

            if (col > 0) { output << "\t"; }                        //Таб

            Position tmpPosition = { row, col };                    //Текущие координаты по таблице

            const auto& tmpItr = sheet_.find(tmpPosition);                 //Возвращаем значение(указатель) по текущей позиции

            if (tmpItr != sheet_.end() && tmpItr->second != nullptr && !tmpItr->second->GetText().empty()) {//Если не конец таблицы, не пустая ячейка

                auto tmpVal = tmpItr->second->GetValue();                           //...то вытащим значение ячейки
                std::visit([&output](auto&& arg) {output << arg; }, tmpVal);        //выкинем в поток
            }

        }

        output << "\n";
    }
}
void Sheet::PrintTexts(std::ostream& output) const {

    Size tmpSize = GetPrintableSize(); //Печатное окно

    /*2 цикла, по строкам и столбцам*/
    for (int row = 0; row < tmpSize.rows; row++) {
        
        for (int col = 0; col < tmpSize.cols; ++col) {

            if (col > 0) { output << "\t"; }            //Таб

            Position tmpPosition = { row, col };        //Текущие координаты по таблице

            auto tmpItr = sheet_.find(tmpPosition);     //Возвращаем значение(указатель) по текущей позиции

            if (tmpItr != sheet_.end() && tmpItr->second != nullptr && !tmpItr->second->GetText().empty()) {               ////Если не конец таблицы, не пустая ячейка

                output << tmpItr->second->GetText();
            }

        }

        output << "\n";
    }


}

const Cell* Sheet::GetCellPointer(Position pos) const {

    if (!pos.IsValid()) {
        throw InvalidPositionException("Position not valid!");
    }

    const auto cell = sheet_.find(pos);         //Ищем ячейку
    if (cell == sheet_.end()) {                 //Поиск вернул конец таблицы, значит такой ячейки нет
        return nullptr;
    }

    return sheet_.at(pos).get();
}

Cell* Sheet::GetCellPointer(Position pos) {
    return const_cast<Cell*>(static_cast<const Sheet&>(*this).GetCellPointer(pos));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}