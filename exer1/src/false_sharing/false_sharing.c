#include <sys/times.h>
#include <time.h>
#include <stdio.h> 
#include <pthread.h> 

long long int tmsBegin1,tmsEnd1,tmsBegin2,tmsEnd2,tmsBegin3,tmsEnd3;

int array[100];

void *heavy_loop(void *param) { 
  int   index = *((int*)param);
  int   i;
  for (i = 0; i < 100000000; i++)
    array[index]+=3;
} 

int main(int argc, char *argv[]) { 
  int       first_elem  = 0;
  int       bad_elem    = 1;
  int       good_elem   = 32;
  long long time1;
  long long time2;
  long long time3;
  pthread_t     thread_1;
  pthread_t     thread_2;

  tmsBegin3 = clock();
  heavy_loop((void*)&first_elem);
  heavy_loop((void*)&bad_elem);
  tmsEnd3 = clock();

  tmsBegin1 = clock();
  pthread_create(&thread_1, NULL, heavy_loop, (void*)&first_elem);
  pthread_create(&thread_2, NULL, heavy_loop, (void*)&bad_elem);
  pthread_join(thread_1, NULL);
  pthread_join(thread_2, NULL);
  tmsEnd1 = clock(); 

  tmsBegin2 = clock();
  pthread_create(&thread_1, NULL, heavy_loop, (void*)&first_elem);
  pthread_create(&thread_2, NULL, heavy_loop, (void*)&good_elem);
  pthread_join(thread_1, NULL);
  pthread_join(thread_2, NULL);
  tmsEnd2 = clock();

  printf("%d %d %d\n", array[first_elem],array[bad_elem],array[good_elem]);
  time1 = (tmsEnd1-tmsBegin1)*1000/CLOCKS_PER_SEC;
  time2 = (tmsEnd2-tmsBegin2)*1000/CLOCKS_PER_SEC;
  time3 = (tmsEnd3-tmsBegin3)*1000/CLOCKS_PER_SEC;
  printf("%lld ms\n", time1);
  printf("%lld ms\n", time2);
  printf("%lld ms\n", time3);

  return 0; 
} 
