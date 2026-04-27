#ifndef CONFIG_HPP
#define CONFIG_HPP

// Параметры задачи
// const double EPS = 1e-5;  // требуемая относительная точность
// const double TAU = 1e-4;  // коэффициент сходимости
// const int N = 13960;      // размерность задачи

// Значения по умолчанию (можно переопределить через -D при сборке)
#ifndef EPS
#define EPS 1e-5
#endif

#ifndef TAU
#define TAU 9e-5
#endif

#ifndef N
#define N 14997
#endif

#endif // CONFIG_HPP
