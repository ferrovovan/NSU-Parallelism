#!/bin/bash

# НЕ РАБОТАЕТ!
# Находим исполняемые файлы без расширения (исключая .git, .c, .cpp, .h, .py и т.д.)
# Проверка: файл исполняемый (+x) И НЕ имеет расширения (нет точки в имени после последнего слэша)

find . -type f -executable ! -name "*.*" ! -path "./.git/*" ! -path "./task2/venv/*" -print

# Спрашиваем подтверждение
read -p "Удалить найденные файлы? (y/n): " answer

if [[ "$answer" == "y" || "$answer" == "Y" ]]; then
    find . -type f -executable ! -name "*.*" ! -path "./.git/*" -delete
    echo "Файлы удалены."
else
    echo "Операция отменена."
fi


# find . -type f -executable -path "*/task[0-9]*/*" -print -exec rm -f {} +

# echo ""
