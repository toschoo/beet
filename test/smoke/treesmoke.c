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

#define NODESZ 14
#define BYTES 128
#define KEYSZ   4
#define DATASZ  4

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
		perror("cannot open file");
		return -1;
	}
	if (fclose(f) != 0) {
		perror("cannot close file");
		return -1;
	}
	return 0;
}

#define DEREF(x) \
	(*(int*)x)

char compare(const void *one, const void *two, void *compare) {
	if (DEREF(one) < DEREF(two)) return BEET_CMP_LESS;
	if (DEREF(one) > DEREF(two)) return BEET_CMP_GREATER;
	return BEET_CMP_EQUAL;
}

int initTree(beet_tree_t *tree, char *base,
                                char *name1,
                                char *name2,
                                FILE *roof,
                       beet_pageid_t *root) {
	beet_err_t err;
	beet_rider_t *nlfs, *lfs;
	beet_ins_t *ins;

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
	ins = calloc(1, sizeof(beet_ins_t));
	if (ins == NULL) {
		fprintf(stderr, "out-of-mem\n");
		return -1;
	}
	beet_ins_setPlain(ins);
	
	if (initRider(nlfs, base, name1) != 0) return -1;
	if (initRider(lfs, base, name2) != 0) return -1;

	err = beet_tree_init(tree, NODESZ, NODESZ, KEYSZ, DATASZ,
	       nlfs, lfs, roof, &compare, NULL, NULL, NULL, ins);
	if (err != BEET_OK) {
		errmsg(err, "cannot initialise rider");
		return -1;
	}
	err = beet_tree_makeRoot(tree, root);
	if (err != BEET_OK) {
		errmsg(err, "cannot make root");
		return -1;
	}
	return 0;
}

int testWriteOneNode(beet_tree_t *tree, beet_pageid_t *root, int lo, int hi) {
	beet_err_t    err;

	fprintf(stderr, "writing %06d -- %06d\n", lo, hi);
	for(int z=hi-1;z>=lo;z--) {
		err = beet_tree_insert(tree, root, &z, &z);
		if (err != BEET_OK) {
			errmsg(err, "cannot insert into tree");
			return -1;
		}
	}
	return 0;
}

int testReadRandom(beet_tree_t *tree, beet_pageid_t *root, int hi) {
	beet_err_t    err;
	beet_node_t *node;
	int32_t slot;
	int k;

	for(int i=0;i<50;i++) {
		k=rand()%hi;

		err = beet_tree_get(tree, root, &k, &node);
		if (err != BEET_OK) return -1;

		// fprintf(stderr, "got node %u for %d\n", node->self, k);

		slot = beet_node_search(node, KEYSZ, &k, &compare, NULL);
		if (slot < 0) {
			fprintf(stderr, "unexpected result: %d\n", k);
			return -1;
		}
		if (!beet_node_equal(node, slot, KEYSZ, &k, &compare, NULL)) {
			fprintf(stderr, "key not found: %d in %u (%d - %d)\n", k,
					node->self,
			                (*(int*)node->keys),
			                (*(int*)(node->keys+KEYSZ*node->size-KEYSZ)));
			return -1;
		}
		// fprintf(stderr, "found: %d in %d\n", k, slot);
		if (memcmp(node->keys+slot*KEYSZ,
		           node->kids+slot*DATASZ, KEYSZ) != 0) {
			fprintf(stderr, "key and data differ: %d - %d\n",
				(*(int*)(node->keys+slot*KEYSZ)),
				(*(int*)(node->kids+slot*KEYSZ)));
			return -1;
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
	char *nlfs = "test10.noleaf";
	char *lfs = "test10.leaf";
	char *p;
	FILE *roof;
	int rc = EXIT_SUCCESS;
	beet_tree_t tree;
	beet_pageid_t root = BEET_PAGE_LEAF;
	char haveTree = 0;

	srand(time(NULL));

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

	if (initTree(&tree, path, nlfs, lfs, roof, &root) != 0) {
		fprintf(stderr, "cannot init tree\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveTree = 1;

	if (testWriteOneNode(&tree, &root, 0, NODESZ-1) != 0) {
		fprintf(stderr, "testWriteOneNode failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, NODESZ-1) != 0) {
		fprintf(stderr, "testReadRandom failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, NODESZ-1, 2*NODESZ-1) != 0) {
		fprintf(stderr, "testWriteOneNode failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 2*NODESZ-1) != 0) {
		fprintf(stderr, "testReadRandom failed 2x\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, 2*NODESZ-1, 4*NODESZ-1) != 0) {
		fprintf(stderr, "testWriteOneNode failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 4*NODESZ-1) != 0) {
		fprintf(stderr, "testReadRandom failed with 4x\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, 4*NODESZ-1, 8*NODESZ-1) != 0) {
		fprintf(stderr, "testWriteOneNode failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 8*NODESZ-1) != 0) {
		fprintf(stderr, "testReadRandom failed with 8x\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, 8*NODESZ-1, 16*NODESZ-1) != 0) {
		fprintf(stderr, "testWriteOneNode failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 16*NODESZ-1) != 0) {
		fprintf(stderr, "testReadRandom failed with 8x\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, 16*NODESZ-1, 32*NODESZ-1) != 0) {
		fprintf(stderr, "testWriteOneNode failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 32*NODESZ-1) != 0) {
		fprintf(stderr, "testReadRandom failed with 8x\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteOneNode(&tree, &root, 32*NODESZ-1, 64*NODESZ-1) != 0) {
		fprintf(stderr, "testWriteOneNode failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&tree, &root, 64*NODESZ-1) != 0) {
		fprintf(stderr, "testReadRandom failed with 8x\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

cleanup:
	if (haveTree) beet_tree_destroy(&tree);
	fclose(roof);
	if (rc == EXIT_SUCCESS) {
		fprintf(stderr, "PASSED\n");
	} else {
		fprintf(stderr, "FAILED\n");
	}
	return rc;
}

