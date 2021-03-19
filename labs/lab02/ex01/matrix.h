typedef struct mat_s
{
    float *data;
    int nrows;
    int ncols;
} mat_t;

void initialize_matrix(mat_t **m, int nrows, int ncols);
void free_matrix(mat_t *m);

void randomize(mat_t *m);

int dot(mat_t *res, mat_t *m1, mat_t *m2);
float row_column_product(mat_t *m1, int nrow, mat_t *m2, int ncol);

int transpose(mat_t *res, mat_t *m);

void print_matrix(mat_t *m);