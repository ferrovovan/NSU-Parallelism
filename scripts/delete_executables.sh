#!/bin/bash

# Ищем исполняемые файлы только внутри task1, task2, task3
# Исключаем любые пути, содержащие /venv/ или /__pycache__/
mapfile -t files < <(find task1 task2 task3 -type f -executable \
    ! -path "*/venv/*" \
    ! -path "*/__pycache__/*" \
    -print)

if [ ${#files[@]} -eq 0 ]; then
    echo "Исполняемые файлы не найдены."
    exit 0
fi

echo "Найденные исполняемые файлы:"
printf '%s\n' "${files[@]}"

read -p "Удалить их? (y/N): " answer
if [[ "$answer" == "y" || "$answer" == "Y" ]]; then
    for f in "${files[@]}"; do
        rm -f "$f"
    done
    echo "Удалено."
else
    echo "Отменено."
fi
