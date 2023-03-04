#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>

/* Matris boyutlarını kontrol eden makro */
#define MATRIX_SIZE 5
 /* Normal bir diziyi sanki iki boyutluymuş gibi ele alan makro */
#define array(arr, i, j) arr[(int) MATRIX_SIZE * (int) i + (int) j]

void fill_matrix(int *matrix);
void print_matrix(int *matrix, int print_width);
int *matrix_page(int *matrix, unsigned long m_size);
void matrix_unmap(int *matrix, unsigned long m_size);
__attribute__ ((noreturn)) void row_multiply(void *row_args);

/* Her matris için tamsayı işaretçileri */
static int *matrix_a, *matrix_b, *matrix_c;

/* Her iş parçacığı için bağımsız değişken yapısı */
typedef struct arg_struct
{
  int *a;
  int *b;
  int *c;
  int row;
} thr_args;

/* Verilen matrisi 1'den 10'a kadar bir tamsayı ile doldurun */
void fill_matrix(int *matrix)
{
  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j < MATRIX_SIZE; j++)
    {
      array(matrix, i, j) = rand() % 10 + 1;
    }
  }
  return;
}

/* Verilen matrisi yazdır */
void print_matrix(int *matrix, int print_width)
{
  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j <  MATRIX_SIZE; j++)
    {
      printf("[%*d]", print_width, array(matrix, i, j));
    }
    printf("\n");
  }
  printf("\n");
  return;
}

/* Verilen matrisi mmap() kullanarak bir bellek sayfasına eşler */
int *matrix_page(int *matrix, unsigned long m_size)
{
  matrix = mmap(0, m_size, PROT_READ | PROT_WRITE,
    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  /* If mmap() failed, exit! */
  if (matrix == (void *) -1)
  {
    exit(EXIT_FAILURE);
  }
  memset((void *) matrix, 0, m_size);
  return matrix;
}

/* Verilen matrisin hafıza sayfasından eşlemesini kaldırır */
void matrix_unmap(int *matrix, unsigned long m_size)
{
  /* If munmap() failed, exit! */
  if (munmap(matrix, m_size) == -1)
  {
    exit(EXIT_FAILURE);
  }
}

/* Verilen satır için tüm endeksleri hesapla */
__attribute__ ((noreturn)) void row_multiply(void *row_args)
{
  thr_args *args = (thr_args *) row_args;
  for(int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j < MATRIX_SIZE; j++)
    {
      int add = array(args->a, args->row, j) * array(args->b, j, i);
      array(args->c, args->row, i) += add;
    }
  }
  pthread_exit(0);
}

int main(void)
{
  /* Matrislerin bellek boyutunu hesaplayın */
  unsigned long m_size = sizeof(int) * (unsigned long) (MATRIX_SIZE * MATRIX_SIZE);

  /* matrix_a, matrix_b ve matrix_c'yi bir bellek sayfasına eşleyin */
  matrix_a = matrix_page(matrix_a, m_size);
  matrix_b = matrix_page(matrix_b, m_size);
  matrix_c = matrix_page(matrix_c, m_size);

  /* Her iki matrisi de 1-10 arası rasgele tamsayılarla doldurun */
  fill_matrix(matrix_a);
  fill_matrix(matrix_b);

  /* Yazdırmadan önce her iki matrisi de yazdırın */
  printf("Matrix A:\n---------\n");
  print_matrix(matrix_a, 2);
  printf("Matrix B:\n---------\n");
  print_matrix(matrix_b, 2);

  /* İş parçacığı verileri için dizileri tahsis edin */
  pthread_t *thrs;
  thr_args *args;
  if ((thrs = malloc(sizeof(pthread_t) * (unsigned long) MATRIX_SIZE)) == NULL ||
    (args = malloc(sizeof(thr_args) * (unsigned long) MATRIX_SIZE)) == NULL)
  {
    exit(EXIT_FAILURE);
  }

  /* 0, 1, ..., N-1 dizileri oluşturun ve onlara verileriyle bir yapı verin */
  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    args[i] = (thr_args) {
      .a = matrix_a,
      .b = matrix_b,
      .c = matrix_c,
      .row = i
    };
    pthread_create(&thrs[i], NULL, (void *) &row_multiply, (void *) &args[i]);
  }

  /* Her iş parçacığını topla */
  for (int j = 0; j < MATRIX_SIZE; j++)
    pthread_join(thrs[j], NULL);

  /* Her iş parçacığı için ayrılan ücretsiz kaynaklar */
  if (thrs != NULL)
  {
    free(thrs);
    thrs = NULL;
  }
  if (args != NULL)
  {
    free(args);
    args = NULL;
  }

  /* Çarpmanın sonucunu yazdır */
  printf("Result matrix:\n--------------\n");
  print_matrix(matrix_c, 4);

  /* Ayrılan hafıza sayfalarını geri ver */
  matrix_unmap(matrix_a, m_size);
  matrix_unmap(matrix_b, m_size);
  matrix_unmap(matrix_c, m_size);
}
