/*
 * openmp.c
 *
 *  Created on: Oct 25, 2013
 *      Author: bogdan
 */

int main(int argc, char **argv) {
	int a[1000];
	#pragma omp parallel for schedule(static, 2)
    for (int i = 0; i < 1000;i++) {
        if (i == 1) {
            usleep(10000);
        }
        a[i] = i;
    }
    return 0;
}

