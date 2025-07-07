#ifndef UPGRADED_MULTIPLY_H
#define UPGRADED_MULTIPLY_H
// #define USE_AVX2

#ifdef USE_AVX2
#include <immintrin.h>
#endif

#include "matrix.h"

/**
 * Улучшенная функция перемножения матриц (A * B)
 * Проверки на валидность перемножения нет
 * @param A матрица слева (m x n)
 * @param B матрица справа (n x k)
 * @return C = A * B. C: m x k
 */
inline matrix upgraded_multiply(matrix const &A, matrix const &B) {
    // Матрицы будем умножать блочно,
    // то есть не по строкам/столбцам искомой матрицы,
    // а по подматрицам фиксированных размеров

    // Как это работает?
    // При обработке блока однажды загруженные данные A и B остаются в кэше и используются многократно,
    // Из-за чего избегается "прокрутка" всей матрицы через кэш с вытеснением данных.
    // В результате число обращений не к L1, L2, L3 сокращается в разы

    // Причем разбивать матрицу будем на 3 уровня блоков.
    // Каждый из уровней будет в теормм вмещаться в свой уровень кэша (L3 --> L2 --> L1)

    // Как выбрать размер?
    // При умножении C = A * B нужно хранить A, B и C, то есть 3 блока, причем каждый элемент матрицы занимает 4 байта
    // То есть, если сторона блока - N, то занимаемая данными память будет равна X = 12 * N * N
    // X должна быть хотя бы с небольшим запасом меньше размера кэша текущего уровня
    // Из этих рассуждений получаем оценки
    /// L1 кэш - 32КБ. 32 * 32 * 12 ~ 12КБ
    const static size_t l1_block_size = 32;
    /// L2 кэш - 256КБ. 128 * 128 * 12 ~ 192КБ
    const static size_t l2_block_size = 128;
    // L3 кэш - 12МБ. 256 * 256 * 12 ~ 3МБ
    const static size_t l3_block_size = 256;

    // Вынесем размеры в отдельные переменные, т.к. их получение из экземпляра длинное
    // + в паре сначала почему-то идет количество столбцов....
    size_t a_rows = A.size().second, a_cols = A.size().first, b_cols = B.size().first;
    matrix C(a_rows, b_cols, true);

    // Сначала "выделим" блоки размером l2_block_size x l2_block_size элементов (точнее, максимально такого размера)
    for (size_t i_l2_min = 0; i_l2_min < a_rows; i_l2_min += l2_block_size) {
        for (size_t j_l2_min = 0; j_l2_min < b_cols; j_l2_min += l2_block_size) {
            for (size_t k_l2_min = 0; k_l2_min < a_cols; k_l2_min += l2_block_size) {
                // Считаем правые нижние границы с учетом того, что нельзя выходить за границы массива
                size_t i_l2_max = std::min(i_l2_min + l2_block_size, a_rows);
                size_t j_l2_max = std::min(j_l2_min + l2_block_size, b_cols);
                size_t k_l2_max = std::min(k_l2_min + l2_block_size, a_cols);
                // Получаем "внешние" блоки
                // (i_l2_min, k_l2_min) x (i_l2_max, k_l2_max) для матрицы A
                // (k_l2_min, j_l2_min) x (k_l2_max, j_l2_max) для матрицы B
                // (i_l2_min, j_l2_min) x (i_l2_max, j_l2_max) для матрицы C, соответственно

                // Теперь выделяем "внутренние" блоки l1_block_size x l1_block_size
                // Идея тут та же самая, что с блоками под L2 память, просто они меньше. т.к. сама память меньше
                for (size_t i_l1_min = i_l2_min; i_l1_min < i_l2_max; i_l1_min += l1_block_size) {
                    for (size_t j_l1_min = j_l2_min; j_l1_min < j_l2_max; j_l1_min += l1_block_size) {
                        for (size_t k_l1_min = k_l2_min; k_l1_min < k_l2_max; k_l1_min += l1_block_size) {
                            size_t i_l1_max = std::min(i_l1_min + l1_block_size, i_l2_max);
                            size_t j_l1_max = std::min(j_l1_min + l1_block_size, j_l2_max);
                            size_t k_l1_max = std::min(k_l1_min + l1_block_size, k_l2_max);
                            // С использованием AVX2 на тестах результаты были хуже, так как накладные расходы перекрывали
                            // выгоду на небольших 32 x 32 матрицах
#ifdef USE_AVX2
                            for (size_t i = i_l1_min; i < i_l1_max; ++i) {
                                for (size_t k = k_l1_min; k < k_l1_max; ++k) {
                                    // Создаем 8 копий A[i][k]
                                    __m256 a = _mm256_set1_ps(A[i][i]);

                                    // Обрабатываем элементы с "шагом" 8
                                    for (size_t j = j_l1_min; j + 7 < j_l1_max; j += 8) {
                                        // "Загружаем" в c, b 8 элементов из C, B
                                        __m256 c = _mm256_loadu_ps(&C[i][j]);
                                        __m256 b = _mm256_loadu_ps(&B[k][j]);

                                        // C[i][j] += A[i][k] * B[k][j], только сразу для элементов 8ми столбцов
                                        c = _mm256_fmadd_ps(a, b, c);
                                        _mm256_storeu_ps(&C[i][j], c);
                                    }
                                    // Количество столбцов может быть не кратно 8,
                                    // поэтому остальные элементы добавляем "классически"
                                    for (size_t j = (j_l1_max & ~7u); j < j_l1_max; ++j) {
                                        C[i][j] += A[i][k] * B[k][j];
                                    }
                                }
                            }
#else
                            // Классическое перемножение на случай отсуствия поддержки AVX2
                            for (size_t i = i_l1_min; i < i_l1_max; ++i) {
                                for (size_t k = k_l1_min; k < k_l1_max; ++k) {
                                    for (size_t j = j_l1_min; j < j_l1_max; ++j) {
                                        C[i][j] += A[i][k] * B[k][j];
                                    }
                                }
                            }
#endif
                        }
                    }
                }
            }
        }
    }

    return C;
}

#endif // UPGRADED_MULTIPLY_H
