#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "matrix.h"

void *safe_malloc(size_t s)
{
    void *mem = malloc(s);
    if (mem == NULL)
    {
        perror("Allocation error\n");
        exit(1);
    }
    return mem;
}

/*
 * Initialize an empty matrix of size nrows x ncols
 */
void initialize_matrix(mat_t **m, int nrows, int ncols)
{
    *m = (mat_t *)safe_malloc(sizeof(mat_t));
    (*m)->nrows = nrows;
    (*m)->ncols = ncols;
    (*m)->data = (float *)malloc((nrows * ncols) * sizeof(float));
}

/*
 * Fill the matrix with random values.
 */
void randomize(mat_t *m)
{
    for (int y = 0; y < m->nrows; y++)
    {
        for (int x = 0; x < m->ncols; x++)
        {
            // generate value in the range [-.5, .5)
            m->data[y * m->ncols + x] = (rand() / (1.0f * RAND_MAX)) - 0.5f;
        }
    }
}

/*
 * Transpose matrix m into res.
 */
int transpose(mat_t *res, mat_t *m)
{
    if (!(res->ncols == m->nrows && res->nrows == m->ncols))
    {
        return 1;
    }

    for (int y = 0; y < m->nrows; y++)
    {
        for (int x = 0; x < m->ncols; x++)
        {
            res->data[x * res->ncols + y] = m->data[y * m->ncols + x];
        }
    }

    return 0;
}

/*
 * Compute the dot product between m1 and m2.
 */
int dot(mat_t *res, mat_t *m1, mat_t *m2)
{
    if (m1->ncols != m2->nrows)
    {
        return 1;
    }

    if (!(res->nrows == m1->nrows && res->ncols == m2->ncols))
    {
        return 2;
    }

    for (int y = 0; y < res->nrows; y++)
    {
        for (int x = 0; x < res->ncols; x++)
        {
            res->data[y * res->ncols + x] = row_column_product(m1, y, m2, x);
        }
    }

    return 0;
}

void free_matrix(mat_t *m)
{
    free(m->data);
    free(m);
}

void print_matrix(mat_t *m)
{
    for (int y = 0; y < m->nrows; y++)
    {
        if (y == 0)
        {
            printf("[ ");
        }
        else
        {
            printf("  ");
        }

        for (int x = 0; x < m->ncols; x++)
        {
            printf(" %+1.4f", m->data[y * m->ncols + x]);
        }

        if (y != (m->nrows - 1))
        {
            printf("\n");
        }
        else
        {
            printf(" ]");
        }
    }
}

/*
 * Compute the product between the row 'nrow' of m1 and the column 'ncol' of m2.
 */
float row_column_product(mat_t *m1, int nrow, mat_t *m2, int ncol)
{
    float acc = 0.0f;
    for (int i = 0; i < m1->ncols; i++)
    {
        acc += m1->data[nrow * m1->ncols + i] * m2->data[i * m2->ncols + ncol];
    }

    return acc;
}