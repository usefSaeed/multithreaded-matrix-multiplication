#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>

//Initializing global input matrices and their row col number
int r1,c1,r2,c2;
int** mat1;
int** mat2;


struct argsOfThreadElem {
    int rowIndex;
    int colIndex;
};

int* copyPointerArray(int* p,int size){
    int *traversing_p = p;
    int *res = (int *) malloc (100 * sizeof (int*));
    for (int i=0;i<size;i++,traversing_p++){
        res[i] = *traversing_p;
    }
    return res;
}

char* substr(const char *src, int m, int n) {
    int len = n - m;
    int smallLen = 0;
    char *dest = (char*)malloc(sizeof(char) * (len + 1));
    for (int i = m; i < n && (*(src + i) != '\0'); i++){
        *dest = *(src + i);
        dest++;
        smallLen++;
    }
    *dest = '\0';
    return dest - smallLen;
}


void readMatrixFromFile(char *m, bool isFirst){
    FILE *fp;
    char fname[30];
    strcpy(fname,"Tests/");
    strcat(fname,m);
    strcat(fname,".txt");
    fp = fopen(fname,"r");
    if (fp == NULL) {
        printf("Sorry :( couldn't find file %s\n", fname);
        return;
    }
    char rows[10] = {0};
    char cols[10];
    fscanf(fp,"%s %s",rows,cols);
    int r,c;
    if (isFirst) {
        r = r1 = atoi(substr(rows, 4, 10));
        c = c1 = atoi(substr(cols, 4, 10));
        mat1 = (int**) malloc(r*c*sizeof (int*));
    }else{
        r = r2 = atoi(substr(rows, 4,10));
        c = c2 = atoi(substr(cols, 4,10));
        mat2 = (int**) malloc(r*c*sizeof (int*));
    }

    for (int i=0 ; i<r ; i++) {
        int tempRow[c];
        for (int j = 0; j < c; j++)
            fscanf(fp," %d",&tempRow[j]);
        if (isFirst)
            mat1[i] = copyPointerArray(tempRow,c);
        else
            mat2[i] = copyPointerArray(tempRow,c);

    }
    fclose(fp);
}
//Simple matrix multiplication by 2D array return type
void* routineMat(){
    int** res = (int**) malloc(r1*c2 * sizeof (int*));
    for (int i = 0; i < r1; i++) {
        int *row = (int*) malloc(c2 * sizeof (int*));
        for (int j = 0; j < c2; j++) {
            int elem = 0;
            for (int k = 0; k < c1; k++)
                elem += mat1[i][k] * mat2[k][j];
            row[j]=elem;
        }
        res[i] = row;
    }
    return res;
}

//ONE THREAD ONLY
void evaluateMultPerMatrix( char *fileName) {
    FILE *fp;
    char filePath[30];
    struct timeval start, stop;
    strcpy(filePath,"Outputs/");
    strcat(filePath,fileName);
    fp = fopen(strcat(filePath, "_per_matrix.txt"), "w");
    fprintf(fp,"Method: A thread per matrix\n");
    fprintf(fp,"row=%d col=%d\n",r1,c2);

    gettimeofday(&start, NULL); //start checking time

    pthread_t th;
    if (pthread_create(&th,NULL,routineMat,NULL))
        perror("Thread failed.");
    int **res = (int**) malloc(r1*c2*sizeof (int*));
    if (pthread_join(th,(void*) &res))
        perror("Thread failed.");

    gettimeofday(&stop, NULL); //start checking
    printf("Thread per matrix runtime: %lu μs\n", stop.tv_usec - start.tv_usec);

    for (int i=0;i<r1;i++){
        for (int j=0;j<c2;j++){
            fprintf(fp,"%d ",res[i][j]);
        }
        fprintf(fp,"\n");
    }
    fclose(fp);
}

//ITERATES OVER EACH ELEMENT IN A ROW OF MAT1 AND MULTIPLY BY CORRESPONDING
//CHOSEN COL IN MAT2 RETURNS CORRESPONDING ROW OF OUTPUT MATRIX
void* routineRow(void* args){
    int i = *(int*)args;
    int* res = (int*) malloc(c2 * sizeof (int*));
    for (int j = 0; j < c2; j++) {
        int elemAcc = 0;
        for (int k = 0; k < c1; k++)
            elemAcc += mat1[i][k] * mat2[k][j];
        res[j] = elemAcc;
    }
    return res;
}

//ONE THREAD PER ROW
void evaluateMultPerRow(char* fileName) {
    FILE *fp;
    char filePath[30];
    struct timeval stop, start;
    strcpy(filePath,"Outputs/");
    strcat(filePath,fileName);
    fp = fopen(strcat(filePath, "_per_row.txt"), "w");
    fprintf(fp,"Method: A thread per row\n");
    fprintf(fp,"row=%d col=%d\n",r1,c2);

    gettimeofday(&start, NULL); //start checking time

    pthread_t th[r1];
    for (int i=0;i<r1;i++) {
        int* rowIndex = malloc(sizeof (int));
        *rowIndex = i;
        if (pthread_create(&th[i],NULL,routineRow,rowIndex))
            perror("Thread failed.");
    }
    int **res = (int**) malloc(r1*c2*sizeof (int*));
    for (int i=0;i<r1;i++){
        if (pthread_join(th[i],(void*) &res[i])){
            perror("Thread failed.");
        }
    }
    gettimeofday(&stop, NULL); //start checking
    printf("Thread per row runtime: %lu μs\n", stop.tv_usec - start.tv_usec);
    for (int i=0;i<r1;i++){
        for (int j=0;j<c2;j++){
            fprintf(fp,"%d ",res[i][j]);
        }
        fprintf(fp,"\n");
    }
    fclose(fp);

}

//MULTIPLIES 2 VECTORS AND RETURNS AN ELEMENT IN OUTPUT MATRIX
void* routineElem(void* indices){
    struct argsOfThreadElem* ind = (struct argsOfThreadElem*) indices;
    int i = ind->rowIndex;
    int j = ind->colIndex;
    int res = 0;
    for (int k = 0; k < c1; k++)
        res += mat1[i][k] * mat2[k][j];
    return res;
}

//THREAD FOR EACH ELEMENT
void evaluateMultPerElem(char* fileName) {
    FILE *fp;
    char filePath[30];
    struct timeval start,stop;
    strcpy(filePath,"Outputs/");
    strcat(filePath,fileName);
    fp = fopen(strcat(filePath, "_per_element.txt"), "w");
    fprintf(fp,"Method: A thread per element\n");
    fprintf(fp,"row=%d col=%d\n",r1,c2);
    pthread_t th[r1*c2];
    int c=0;
    gettimeofday(&start, NULL); //start checking
    for (int i=0;i<r1;i++) {
        for (int j=0;j<c2;j++){
            struct argsOfThreadElem* s;
            s = malloc (sizeof (struct argsOfThreadElem));
            s->rowIndex = i;
            s->colIndex = j;
            if (pthread_create(&th[c++],NULL,routineElem,s))
                perror("Thread failed.");
        }
    }
    int res[r1][c2];
    c=0;
    for (int i=0;i<r1;i++)
        for (int j=0;j<c2;j++)
            if (pthread_join(th[c++],(void*) &res[i][j]))
                perror("Thread failed.");

    gettimeofday(&stop, NULL); //start checking
    printf("Thread per element runtime: %lu μs\n", stop.tv_usec - start.tv_usec);

    for (int i=0;i<r1;i++){
        for (int j=0;j<c2;j++){
            fprintf(fp,"%d ",res[i][j]);
        }
        fprintf(fp,"\n");
    }
    fclose(fp);
}


int main(int argc, char* argv[]) {
    char* mat1Name = (char*)malloc(10*sizeof (char));
    char* mat2Name = (char*)malloc(10*sizeof (char));
    char* matoutName = (char*)malloc(10*sizeof (char));
    mat1Name = "a";
    mat2Name = "b";
    matoutName = "c";
    if(argc==4){
        mat1Name = argv[1];
        mat2Name = argv[2];
        matoutName = argv[3];
    }
    readMatrixFromFile(mat1Name, 1);
    readMatrixFromFile(mat2Name, 0);
    if (mat1 == NULL || mat2 == NULL) return 0;
    if (r2 != c1) {
        printf("Matrices cannot be multiplied.\n");
        return 0;
    }

    struct timeval stop, start;
    //THREAD PER MATRIX
    evaluateMultPerMatrix(matoutName);

    //THREAD PER ROW
    evaluateMultPerRow(matoutName);

    //THREAD PER ELEMENT
    evaluateMultPerElem(matoutName);
    return 0;
}
