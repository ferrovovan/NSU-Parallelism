# Задание 1
## Описание
1. Заполнить массив типа *float/double*
значениями *синуса* (один период на всю длину массива), 
*10^7* элементами (посчитанными значениями).
2. Посчитать и вывести в терминал *сумму элементов*.
3. Написать make и cmake файлы для сборки.
4. Во время сборки дать возможность выбора типа массива (float или double).
5. В репозитории разделить на 2 ветки: make и cmake.
6. В Readme файле укажите как выбирать тип массива, и какой вывод дали float и double варианты.

Обратите внимание на следующие ссылки:
CMake: [](https://cmake.org/cmake/help/latest/guide/tutorial/Adding%20a%20Library.html),
Conditional part of Makefiles: [](https://makefiletutorial.com/#conditional-part-of-makefiles),
[](https://www.gnu.org/software/make/manual/html_node/Conditionals.html#Conditionals),
[](https://en.cppreference.com/w/cpp/preprocessor/conditional).


-------------------------------------------
## Make-версия
### Float
To use float type use command
```
make float
```
float   ->   -0.0277862

### Double
To use double type use command
```
make double
```
double  ->   -6.76916e-10

------------------------------------------- 
## CMake-версия
### Float
To use float type use command
```
mkdir build && cd build  
cmake  .. -DUSE_DOUBLE=OFF
make  
./task1_sin  
```
**Output:** -> -0.0277862

### Double
To use double type use command
```
mkdir build && cd build  
cmake ..  -DUSE_DOUBLE=ON
make  
./task1_sin  
```
**Output:**  -> 3.68912e-10
