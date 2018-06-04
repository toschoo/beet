/* ========================================================================
 * Simple Tests for tree
 * ========================================================================
 */
#include <beet/rider.h>
#include <beet/node.h>
#include <beet/tree.h>
#include <common/math.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

#define NODESZ 14
#define BIG   160
#define BYTES 128
#define KEYSZ   4

void errmsg(beet_err_t err, char *s) {
	fprintf(stderr, "%s: %s (%d)\n",
	     s, beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "OSERR: %s\n",
		            beet_oserrdesc());
	}
}

int initRider(beet_rider_t *rider, char *base, char *name) {
	beet_err_t err;
	err = beet_rider_init(rider, base, name, BYTES, 10);
	if (err != BEET_OK) {
		errmsg(err, "cannot initialise rider");
		return -1;
	}
	return 0;
}

int createFile(char *path, char *name) {
	FILE *f;
	char *p = NULL;
	size_t s;

	s = strlen(path);
	s += strlen(name);

	p = malloc(s+2);
	if (p == NULL) {
		fprintf(stderr, "cannot allocate path\n");
		return -1;
	}
	sprintf(p, "%s/%s", path, name);
	f = fopen(p, "w"); free(p);
	if (f == NULL) {
		fprintf(stderr, "cannot open file %s/%s\n", path, name);
		perror("cannot open file");
		return -1;
	}
	if (fclose(f) != 0) {
		perror("cannot close file");
		return -1;
	}
	return 0;
}

int createPath(char *path) {
	struct stat st;

	if (stat(path,&st) == 0) return 0;
	if (mkdir(path,0755) != 0) return -1;
	return 0;
}

#define DEREF(x) \
	(*(int*)x)

char compare(const void *one, const void *two) {
	if (DEREF(one) < DEREF(two)) return BEET_CMP_LESS;
	if (DEREF(one) > DEREF(two)) return BEET_CMP_GREATER;
	return BEET_CMP_EQUAL;
}

int initTree(beet_tree_t *tree, char *base,
                                char *name1,
                                char *name2,
                                 FILE *roof,
                        beet_pageid_t *root,
                            beet_ins_t *ins) {
	beet_err_t err;
	beet_rider_t *nlfs, *lfs;
	size_t ds;

	if (createPath(base) != 0) {
		perror("cannot create base");
		return -1;
	}

	if (createFile(base, name1) != 0) return -1;
	if (createFile(base, name2) != 0) return -1;

	nlfs = calloc(1, sizeof(beet_rider_t));
	if (nlfs == NULL) {
		fprintf(stderr, "out-of-mem\n");
		return -1;
	}
	lfs = calloc(1, sizeof(beet_rider_t));
	if (lfs == NULL) {
		fprintf(stderr, "out-of-mem\n");
		return -1;
	}
	
	if (initRider(nlfs, base, name1) != 0) return -1;
	if (initRider(lfs, base, name2) != 0) return -1;

	ds = ins == NULL ? 0 : sizeof(beet_pageid_t);
	err = beet_tree_init(tree, NODESZ, NODESZ, KEYSZ, ds,
	                     nlfs, lfs, roof, &compare, ins);
	if (err != BEET_OK) {
		errmsg(err, "cannot initialise rider");
		return -1;
	}
	if (ins != NULL) {
		err = beet_tree_makeRoot(tree, root);
		if (err != BEET_OK) {
			errmsg(err, "cannot make root");
			return -1;
		}
	}
	return 0;
}

int testWriteOneNode(beet_tree_t *tree, beet_pageid_t *root, int lo, int hi) {
	beet_err_t   err;
	beet_pair_t pair;

	fprintf(stderr, "writing %06d -- %06d\n", lo, hi);

	for(int z=hi-1;z>=lo;z--) {
		for(int i=0;i<=z;i++) {
			pair.key = &i;
			pair.data = NULL;
			err = beet_tree_insert(tree, root, &z, &pair);
			if (err != BEET_OK) {
				errmsg(err, "cannot insert into tree");
				return -1;
			}
		}
	}
	return 0;
}

int testReadRandom(beet_tree_t *tree, beet_pageid_t *root, int hi) {
	beet_err_t    err;
	beet_node_t *node;
	beet_pageid_t *root2;
	int32_t slot;
	int k;

	for(int i=0;i<10;i++) {
		k=rand()%hi;

		err = beet_tree_get(tree, root, &k, &node);
		if (err != BEET_OK) return -1;

		// fprintf(stderr, "got node %u for %d\n", node->self, k);

		slot = beet_node_search(node, KEYSZ, &k, &compare);
		if (slot < 0) {
			fprintf(stderr, "unexpected result: %d\n", k);
			return -1;
		}
		if (!beet_node_equal(node, slot, KEYSZ, &k, &compare)) {
			fprintf(stderr, "key not found: %d in %u (%d - %d)\n", k,
					node->self,
			                (*(int*)node->keys),
			                (*(int*)(node->keys+KEYSZ*node->size-KEYSZ)));
			return -1;
		}

		if (k > 0 && tree->ins != NULL) {
			root2 = (beet_pageid_t*)(node->kids+slot*KEYSZ);
			if (testReadRandom(tree->ins->rsc, root2, k) != 0) {
				return -1;
			}
		}

		err = beet_tree_release(tree, node);
		if (err != BEET_OK) {
			errmsg(err, "cannot release node");
			return -1;
		}
		free(node);
	}
	return 0;

}

int main() {
	char *path = "rsc";
	char *epath = "rsc/emb";
	char *nlfs = "test15.noleaf";
	char *lfs = "test15.leaf";
	char *p;
	FILE *roof=NULL;
	int rc = EXIT_SUCCESS;
	beet_tree_t tree;
	beet_tree_t *etree;
	beet_pageid_t root = BEET_PAGE_LEAF;
	beet_ins_t *ins;
	char haveTree = 0;

	srand(time(NULL));

	etree = calloc(1, sizeof(beet_tree_t));
	if (etree == NULL) {
		fprintf(stderr, "out-of-mem\n");
		return EXIT_FAILURE;
	}
	if (initTree(etree, epath, nlfs, lfs, NULL, NULL, NULL) != 0) {
		fprintf(stderr, "cannot init embedded tree\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	if (createFile(path, "roof") != 0) {
		fprintf(stderr, "cannot create roof\n");
		return EXIT_FAILURE;
	}
	p = malloc(strlen(path) + 6);
	if (p == NULL) {
		fprintf(stderr, "out-of-mem\n");
		return EXIT_FAILURE;
	}
	sprintf(p, "%s/roof", path);

	roof = fopen(p, "rb+"); free(p);
	if (roof == NULL) {
		fprintf(stderr, "cannot open roof\n");
		return EXIT_FAILURE;
	}
	ins = calloc(1, sizeof(beet_ins_t));
	if (ins == NULL) {
		fprintf(stderr, "cannot alloc ins\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	beet_ins_setEmbedded(ins, etree);

	if (initTree(&tree, path, nlfs, lfs, roof, &root, ins) != 0) {
		fprintf(stderr, "cannot init tree\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveTree = 1;

	if (testWriteOneNode(&tree, &root, 0, NODESZ-1) != 0) {
		fprintf(stderr, "cannot write to %d\n", NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, NODESZ-1) != 0) {
		fprintf(stderr, "cannot read %d\n", NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, NODESZ-1, 2*NODESZ-1) != 0) {
		fprintf(stderr, "cannot write to %d\n", 2*NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 2*NODESZ-1) != 0) {
		fprintf(stderr, "cannot read %d\n", 2*NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, 2*NODESZ-1, 3*NODESZ-1) != 0) {
		fprintf(stderr, "cannot write to %d\n", 3*NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 3*NODESZ-1) != 0) {
		fprintf(stderr, "cannot read %d\n", 3*NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, 3*NODESZ-1, 4*NODESZ-1) != 0) {
		fprintf(stderr, "cannot write to %d\n", 4*NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 4*NODESZ-1) != 0) {
		fprintf(stderr, "cannot read %d\n", 5*NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, 4*NODESZ-1, 5*NODESZ-1) != 0) {
		fprintf(stderr, "cannot write to %d\n", 5*NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 5*NODESZ-1) != 0) {
		fprintf(stderr, "cannot read %d\n", 5*NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, 5*NODESZ-1, 6*NODESZ-1) != 0) {
		fprintf(stderr, "cannot write to %d\n", 6*NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 6*NODESZ-1) != 0) {
		fprintf(stderr, "cannot read %d\n", 6*NODESZ-1);
		rc = EXIT_FAILURE; goto cleanup;
	}

cleanup:
	if (haveTree) beet_tree_destroy(&tree);
	if (roof != NULL) fclose(roof);
	if (rc == EXIT_SUCCESS) {
		fprintf(stderr, "PASSED\n");
	} else {
		fprintf(stderr, "FAILED\n");
	}
	return rc;
}

