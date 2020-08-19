#include <stdio.h>
#include <stdlib.h>
#include "mull.h"


#ifndef DEBUG
#define N   335567L
#else
#define N  104856L
#endif

#define ALLOCATIONMACRO(X)  mulloc(X)
#define INITMACRO(X)        mullinit(X, 0)
#define FREEMACRO(X)        mullfree(X, -1)

typedef struct node {
    long x;
    struct node *left, *right;
} TREENODE;

int id;
//size_t temp_count = 0;

TREENODE *makeleaf(int x);
long countnodes(TREENODE *t);
void hang(TREENODE *parent, long x);
void freetree(TREENODE *t);

int main(int argc, char *argv[])
{
    TREENODE *root;
    long i;
    long x;

    srandom(2);

    id = INITMACRO(sizeof(TREENODE));

    root = 0;
    for(i = 0L; i < N; i++) {
        x = random() % N;
        if (!root)
            root = makeleaf(x);
        else
            hang(root, x);
    }

    printf("%ld nodes\n", countnodes(root));

    freetree(root);
}
TREENODE *makeleaf(int x)
{
    TREENODE *t;

    t = (TREENODE *) ALLOCATIONMACRO(id);
    //printf("%lu\n", temp_count++);
    t->x = x;
    t->left = t->right = NULL;
    return t;
}
void hang(TREENODE *parent, long x)
{
    for(;;){
        if(x == parent->x)
            return;
        if(x < parent->x){
            if(parent->left){
                parent = parent->left;
            } else {
                parent->left = makeleaf(x);
                return;
            }
        } else {
            if(parent->right){
                parent = parent->right;
            } else {
                parent->right = makeleaf(x);
                return;
            }
        }
    }
}
long countnodes(TREENODE *t)
{
    if(t == 0)
        return 0L;
    return 1L + countnodes(t->left) + countnodes(t->right);
}
void freetree(TREENODE *t)
{
    if(t == 0)
        return;
    freetree(t->left);
    freetree(t->right);
    FREEMACRO(t);
}

