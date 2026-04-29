#!/bin/bash

folders=(task1 task2 task3)
patterns=(
    ".csv"
    ".o"
    "executable"
)

# Определяем путь к текущему скрипту, чтобы случайно не удалить его
script_path=$(realpath "$0" 2>/dev/null || readlink -f "$0" 2>/dev/null || echo "$0")

# Построение аргументов find для паттернов (объединение через -o)
find_conditions=()
for pat in "${patterns[@]}"; do
    if [ ${#find_conditions[@]} -gt 0 ]; then
        find_conditions+=(-o)
    fi
    if [ "$pat" == "executable" ]; then
        find_conditions+=(-executable)
    else
        find_conditions+=(-name "$pat")
    fi
done

# отладка
#echo -e command \'find "${directories[@]}" -type f \( "${find_conditions[@]}" \)\'


# Поиск файлов, исключая виртуальные окружения и кэш Python
mapfile -t files < <(find "${directories[@]}" -type f \( "${find_conditions[@]}" \) \
    ! -path "*/venv/*" \
    ! -path "*/__pycache__/*" \
    ! -path "*/.git/*" \
    ! -name "$(basename "$0")" \
    -print)

if [ ${#files[@]} -eq 0 ]; then
    echo "Файлы не найдены."
    exit 0
else
    echo "Найденные исполняемые файлы:"
    printf '%s\n' "${files[@]}"
fi


read -p "Удалить их? (y/N): " answer
if [[ "$answer" == "y" || "$answer" == "Y" ]]; then
    for f in "${files[@]}"; do
        rm -f "$f"
    done
    echo "Удалено."
else
    echo "Отменено."
fi
