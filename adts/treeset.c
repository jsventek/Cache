/*
 * Copyright (c) 2013, Court of the University of Glasgow
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the University of Glasgow nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "treeset.h"
#include <stdlib.h>

/*
 * implementation for generic treeset implementation
 * implemented as an AVL tree
 */

typedef struct tnode {
    struct tnode *link[2];	/* 0 is left, 1 is right */
    void *element;
    int balance;			/* difference between heights of l and r subs */
} TNode;

struct treeset {
    long size;
    TNode *root;
    int (*cmp)(void *,void *);
};

/*
 * structure needed for recursive population of array of pointers
 */
typedef struct popstruct {
    void **a;
    long len;
} PopStruct;

/*
 * routines used in rotations when rebalancing the tree
 */

/*
 * allocates a new node with the given element and NULL left and right links
 */
static TNode *newNode(void *element) {
    TNode *node = (TNode *)malloc(sizeof(TNode));

    if (node != NULL) {
        node->element = element;
        node->link[0] = node->link[1] = NULL;
        node->balance = 0;
    }
    return node;
}

static TNode *singleRotate(TNode *root, int dir) {
    TNode *save = root->link[!dir];

    root->link[!dir] = save->link[dir];
    save->link[dir] = root;
    return save;
}

static TNode *doubleRotate(TNode *root, int dir) {
    TNode *save = root->link[!dir]->link[dir];

    root->link[!dir]->link[dir] = save->link[!dir];
    save->link[!dir] = root->link[!dir];
    root->link[!dir] = save;
    save = root->link[!dir];
    root->link[!dir] = save->link[dir];
    save->link[dir] = root;
    return save;
}

static void adjustBalance(TNode *root, int dir, int bal) {
    TNode *n = root->link[dir];
    TNode *nn = n->link[!dir];

    if (nn->balance == 0)
        root->balance = n->balance = 0;
    else if (nn->balance == bal) {
        root->balance = -bal;
        n->balance = 0;
    } else {	/* nn->balance == -bal */
        root->balance = 0;
        n->balance = bal;
    }
    nn->balance = 0;
}

static TNode *insertBalance(TNode *root, int dir) {
    TNode *n = root->link[dir];
    int bal = (dir == 0) ? -1 : +1;

    if (n->balance == bal) {
        root->balance = n->balance = 0;
        root = singleRotate(root, !dir);
    } else {	/* n->balance == -bal */
        adjustBalance(root, dir, bal);
        root = doubleRotate(root, !dir);
    }
    return root;
}

static TNode *insert(TNode *root, void *element, int *done,
                     int (*cmp)(void*,void*)) {
    if (root == NULL)
        root = newNode(element);
    else {
        int dir = ((*cmp)(root->element, element) < 0);

        root->link[dir] = insert(root->link[dir], element, done, cmp);
        if (! *done) {
            root->balance += (dir == 0) ? -1 : +1;
            if (root->balance == 0)
                *done = 1;
            else if (abs(root->balance) > 1) {
                root = insertBalance(root, dir);
                *done = 1;
            }
        }
    }
    return root;
}

static TNode *removeBalance(TNode *root, int dir, int *done) {
    TNode *n = root->link[!dir];
    int bal = (dir == 0) ? -1 : +1;

    if (n->balance == -bal) {
        root->balance = n->balance = 0;
        root = singleRotate(root, dir);
    } else if (n->balance == bal) {
        adjustBalance(root, !dir, -bal);
        root = doubleRotate(root, dir);
    } else {	/* n->balance == 0 */
        root->balance = -bal;
        n->balance = bal;
        root = singleRotate(root, dir);
        *done = 1;
    }
    return root;
}

static TNode *remove(TNode *root, void *element, int *done,
                     int (*cmp)(void*,void*), void (*uf)(void*)) {
    if (root != NULL) {
        int dir;

        if ((*cmp)(element, root->element) == 0) {
            if (root->link[0] == NULL || root->link[1] == NULL) {
                TNode *save;

                dir = (root->link[0] == NULL);
                save = root->link[dir];
                if (uf != NULL)
                    (*uf)(root->element);
                free(root);
                return save;
            } else {
                TNode *heir = root->link[0];

                while (heir->link[1] != NULL)
                    heir = heir->link[1];
                root->element = heir->element;
                element = heir->element;
            }
        }
        dir = ((*cmp)(root->element, element) < 0);
        root->link[dir] = remove(root->link[dir], element, done, cmp, uf);
        if (! *done) {
            root->balance += (dir != 0) ? -1 : +1;
            if (abs(root->balance) == 1)
                *done = 1;
            else if (abs(root->balance) > 1)
                root = removeBalance(root, dir, done);
        }
    }
    return root;
}

/*
 * finds element in the set; returns null if it cannot be found
 */
static TNode *find(void *element, TNode *tree, int (*cmp)(void*,void*)) {
    int result;

    if (tree == NULL)
        return NULL;
    result = (*cmp)(element, tree->element);
    if (result < 0)
        return find(element, tree->link[0], cmp);
    else if (result > 0)
        return find(element, tree->link[1], cmp);
    else
        return tree;
}

/*
 * infix traversal to populate array of pointers
 */
static void populate(PopStruct *ps, TNode *node) {
    if (node != NULL) {
        populate(ps, node->link[0]);
        (ps->a)[ps->len++] = node->element;
        populate(ps, node->link[1]);
    }
}

TreeSet *ts_create(int (*cmpFunction)(void *, void *)) {
    TreeSet *ts = (TreeSet *)malloc(sizeof(TreeSet));

    if (ts != NULL) {
        ts->size = 0L;
        ts->root = NULL;
        ts->cmp = cmpFunction;
    }
    return ts;
}

/*
 * postorder traversal, invoking userFunction and then freeing node
 */
static void postpurge(TNode *leaf, void (*userFunction)(void *element)) {
    if (leaf != NULL) {
        postpurge(leaf->link[0], userFunction);
        postpurge(leaf->link[1], userFunction);
        if (userFunction != NULL)
            (*userFunction)(leaf->element);
        free(leaf);
    }
}

void ts_destroy(TreeSet *ts, void (*userFunction)(void *element)) {
    postpurge(ts->root, userFunction);
    free(ts);
}

int ts_add(TreeSet *ts, void *element) {
    int done = 0;

    if (find(element, ts->root, ts->cmp) != NULL)
        return 0;
    ts->root = insert(ts->root, element, &done, ts->cmp);
    ts->size++;
    return 1;
}

static TNode *Min(TNode *n1, TNode *n2, int (*cmp)(void*,void*)) {
    TNode *ans = n1;
    if (n1 == NULL)
        return n2;
    if (n2 == NULL)
        return n1;
    if ((*cmp)(n1->element, n2->element) > 0)
        ans = n2;
    return ans;
}

static TNode *Max(TNode *n1, TNode *n2, int (*cmp)(void*,void*)) {
    TNode *ans = n1;
    if (n1 == NULL)
        return n2;
    if (n2 == NULL)
        return n1;
    if ((*cmp)(n1->element, n2->element) < 0)
        ans = n2;
    return ans;
}

int ts_ceiling(TreeSet *ts, void *element, void **ceiling) {
    TNode *t = ts->root;
    TNode *current = NULL;

    while (t != NULL) {
        int cmp = (*ts->cmp)(element, t->element);
        if (cmp == 0) {
            current = t;
            break;
        } else if (cmp < 0) {
            current = Min(t, current, ts->cmp);
            t = t->link[0];
        } else {
            t = t->link[1];
        }
    }
    if (current == NULL)
        return 0;
    *ceiling = current->element;
    return 1;
}

void ts_clear(TreeSet *ts, void (*userFunction)(void *element)) {
    postpurge(ts->root, userFunction);
    ts->root = NULL;
    ts->size = 0L;
}

int ts_contains(TreeSet *ts, void *element) {
    return (find(element, ts->root, ts->cmp) != NULL);
}

/*
 * find node with minimum value in subtree
 */
TNode *findMin(TNode *tree) {
    if (tree != NULL)
        while (tree->link[0] != NULL)
            tree = tree->link[0];
    return tree;
}

int ts_first(TreeSet *ts, void **element) {
    TNode *current = findMin(ts->root);

    if (current == NULL)
        return 0;
    *element = current->element;
    return 1;
}

int ts_floor(TreeSet *ts, void *element, void **floor) {
    TNode *t = ts->root;
    TNode *current = NULL;

    while (t != NULL) {
        int cmp = (*ts->cmp)(element, t->element);
        if (cmp == 0) {
            current = t;
            break;
        } else if (cmp > 0) {
            current = Max(t, current, ts->cmp);
            t = t->link[1];
        } else {
            t = t->link[0];
        }
    }
    if (current == NULL)
        return 0;
    *floor = current->element;
    return 1;
}

int ts_higher(TreeSet *ts, void *element, void **higher) {
    TNode *t = ts->root;
    TNode *current = NULL;

    while (t != NULL) {
        int cmp = (*ts->cmp)(element, t->element);
        if (cmp < 0) {
            current = Min(t, current, ts->cmp);
            t = t->link[0];
        } else {
            t = t->link[1];
        }
    }
    if (current == NULL)
        return 0;
    *higher = current->element;
    return 1;
}

int ts_isEmpty(TreeSet *ts) {
    return (ts->size == 0L);
}

/*
 * find node with maximum value in subtree
 */
TNode *findMax(TNode *tree) {
    if (tree != NULL)
        while (tree->link[1] != NULL)
            tree = tree->link[1];
    return tree;
}

int ts_last(TreeSet *ts, void **element) {
    TNode *current = findMax(ts->root);

    if (current == NULL)
        return 0;
    *element = current->element;
    return 1;
}

int ts_lower(TreeSet *ts, void *element, void **lower) {
    TNode *t = ts->root;
    TNode *current = NULL;

    while (t != NULL) {
        int cmp = (*ts->cmp)(element, t->element);
        if (cmp > 0) {
            current = Max(t, current, ts->cmp);
            t = t->link[1];
        } else {
            t = t->link[0];
        }
    }
    if (current == NULL)
        return 0;
    *lower = current->element;
    return 1;
}

int ts_pollFirst(TreeSet *ts, void **element) {
    TNode *node = findMin(ts->root);
    int done = 0;

    if (node == NULL)
        return 0;
    *element = node->element;
    ts->root = remove(ts->root, node->element, &done, ts->cmp, NULL);
    return 1;
}

int ts_pollLast(TreeSet *ts, void **element) {
    TNode *node = findMax(ts->root);
    int done = 0;

    if (node == NULL)
        return 0;
    *element = node->element;
    ts->root = remove(ts->root, node->element, &done, ts->cmp, NULL);
    return 1;
}

int ts_remove(TreeSet *ts, void *element, void (*userFunction)(void *element)) {
    int done = 0;

    if (find(element, ts->root, ts->cmp) == NULL)
        return 0;
    ts->root = remove(ts->root, element, &done, ts->cmp, userFunction);
    ts->size--;
    return 1;
}

long ts_size(TreeSet *ts) {
    return ts->size;
}

/*
 * generates an array of void * pointers on the heap and copies
 * tree elements into the array
 *
 * returns pointer to array or NULL if malloc failure
 */
static void **genArray(TreeSet *ts) {
    void **tmp = NULL;
    PopStruct ps;
    if (ts->size > 0L) {
        size_t nbytes = ts->size * sizeof(void *);
        tmp = (void **)malloc(nbytes);
        if (tmp != NULL) {
            ps.a = tmp;
            ps.len = 0;
            populate(&ps, ts->root);
        }
    }
    return tmp;
}

void **ts_toArray(TreeSet *ts, long *len) {
    void **array = genArray(ts);

    if (array != NULL)
        *len = ts->size;
    return array;
}

Iterator *ts_it_create(TreeSet *ts) {
    Iterator *it = NULL;
    void **tmp = genArray(ts);

    if (tmp != NULL) {
        it = it_create(ts->size, tmp);
        if (it == NULL)
            free(tmp);
    }
    return it;
}
