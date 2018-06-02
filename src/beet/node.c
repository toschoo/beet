/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * B+Node Abstraction
 * ========================================================================
 *
 * The binary format of a node, which is store in a page, is
 *
 * 1) Internal Node
 *    +------------------------------------------+
 *    | Size | Keys[nodesize] | Kids[nodesize+1] |
 *    +------------------------------------------+
 *     4byte  keysize*nodesz    4*(nodsz+1)
 *
 *    Here, keysize and nodesz mean the repective size
 *    stored in the tree structure (i.e. 
 *    - keysize: size of 1 key
 *    - nodesz : max number of keys per node)
 *
 *    The following diagram illustrates the 
 *    releation between keys and their kids:
 *
 *            k1  k2  k3  k4  ...
 *           /  \/  \/  \/  \/
 *          n1  n2  n3  n4  n5 ...
 *
 *    where k1, k2, etc. represent keys and
 *          n1, n2, etc. represent pageids.
 *    
 *    That is, each key points to two nodes:
 *    - one with keys less          than the pointing key
 *    - one with keys greater/equal than the pointing key
 *
 *    With the execption of the first key, the left node
 *    of key(n) is at the same time the right node of key(n-1).
 *
 *    With the exception of the last key, the right node
 *    of key(n) is at the same time the left node of key(n+1).
 *
 *    We consequently need n+1 kids for n keys.
 *
 * 2) Leaf Node
 *    +------------------------------------------------------+
 *    | Size | Next | Prev | Keys[nodesize] | Kids[nodesize] |
 *    +------------------------------------------------------+
 *     4byte  4byte  4byte   keysize*nodesz   datasize*nodesz
 *
 *    The keys, as before, contain the keys of this tree and
 *    the kids contain the data for their keys.
 *
 *    Next points to the next leaf node in the chain.
 *    Prev points to the previous leaf node in the chain.
 *
 *    This way, leaf nodes form a doubly linked list
 *    through which we can iterate scanning a range of keys
 *    (and their data) or even the entire tree.
 * ========================================================================
 */
#include <beet/node.h>
#include <string.h>

/* ------------------------------------------------------------------------
 * initialise a node from a page (i.e. page -> node)
 * ------------------------------------------------------------------------
 */
void beet_node_init(beet_node_t *node,
                    beet_page_t *page,
                    uint32_t   nodesz,
                    uint32_t    keysz,
                    char         leaf) {
	int off = 0;

	node->page = page;
	node->self = page->pageid;
	node->leaf = leaf;

	memcpy(&node->size, page->data, sizeof(uint32_t));
	off += sizeof(uint32_t);
	if (leaf) {
		memcpy(&node->next, page->data+off, sizeof(uint32_t));
		off += sizeof(int32_t);
		memcpy(&node->prev, page->data+off, sizeof(uint32_t));
		off += sizeof(int32_t);
	}
	
	node->keys = page->data+off;
	node->kids = page->data+off + keysz*nodesz;
}

/* ------------------------------------------------------------------------
 * Serialise node to its page (node -> page)
 * ------------------------------------------------------------------------
 */
void beet_node_serialise(beet_node_t *node) {
	int off=0;
	memcpy(node->page->data, &node->size, sizeof(uint32_t));
	if (node->leaf) {
		off+=sizeof(int32_t);
		memcpy(node->page->data+off, &node->next, sizeof(int32_t));
		off+=sizeof(int32_t);
		memcpy(node->page->data+off, &node->prev, sizeof(int32_t));
	}
}

/* ------------------------------------------------------------------------
 * Helper: add data to node (using ins 'inserter' method)
 * ------------------------------------------------------------------------
 */
static inline beet_err_t ad3ata(beet_node_t *node,
                                uint32_t    dsize,
                                uint32_t     slot,
                                const void  *data,
                                beet_ins_t   *ins) {
	if (ins == NULL) return BEET_OK;
	return ins->inserter(ins->rsc, dsize, node->kids+slot*dsize, data);
}

/* ------------------------------------------------------------------------
 * Helper: add (key,data) to node
 * ------------------------------------------------------------------------
 */
static inline beet_err_t add2slot(beet_node_t *node,
                                  uint32_t    ksize,
                                  uint32_t    dsize,
                                  uint32_t     slot,
                                  const void   *key,
                                  const void  *data,
                                  beet_ins_t   *ins) {
	char    *src = node->keys+slot*ksize;
	uint64_t shift = (node->size-slot)*ksize;
	uint32_t dsz;

	/* shift keys starting from 'slot' one to the right */
	if (shift > 0) memmove(src+ksize,src,shift);

	/* new key goes to 'slot' */
	memcpy(node->keys+slot*ksize, key, ksize);

	/* if we have no data, we're done */
	if (data == NULL) return BEET_OK;

	/* datasize depends on node type */
	dsz=node->leaf?dsize:sizeof(beet_pageid_t);

 	shift = (node->size-slot)*dsz;
	src = node->kids+slot*dsz;

	/* in a nonleaf node the data consist of 
 	 * pageids; the left pageid of the old key
 	 * is now the left pointer of the new key
 	 * and, therefore, stays where it is */
	if (!node->leaf) src += dsz;

	/* shift data one to the right */
	if (shift > 0) memmove(src+dsz,src,shift);

	/* in a leaf we use the callback */
	if (node->leaf) {
		if (ins != NULL) {
			ins->clear(ins->rsc, node->kids+slot*dsize);
		}
		ad3ata(node, dsize, slot, data, ins);
	
	/* in a nonleaf we copy the new key to the right of 'slot' */
	} else {
		if (dsz > 0) memcpy(node->kids+(slot+1)*dsz, data, dsz);
	}

	/* done ! */
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Get key at slot
 * ------------------------------------------------------------------------
 */
void *beet_node_getKey(beet_node_t *node,
                       uint32_t     slot,
                       uint32_t   keysz) 
{
	return node->keys+slot*keysz;
}

/* ------------------------------------------------------------------------
 * Get data at slot
 * ------------------------------------------------------------------------
 */
void *beet_node_getData(beet_node_t *node,
                        uint32_t     slot,
                        uint32_t  datasz) 
{
	return node->kids+slot*datasz;
}

/* ------------------------------------------------------------------------
 * Get pageid at slot (internal node only!)
 * ------------------------------------------------------------------------
 */
beet_pageid_t beet_node_getPageid(beet_node_t *node,
                                  uint32_t     slot) {
	return (*(beet_pageid_t*)(node->kids+slot*sizeof(beet_pageid_t)));
}

/* ------------------------------------------------------------------------
 * Linear search
 * -------------
 * Either from min->max (dir=1) or
 *        from max->min (dir=-1).
 * The function returns the slot with the least key >= key
 * ------------------------------------------------------------------------
 */
static inline int32_t linsearch(char *keys, void *key,
                                uint32_t        ksize,
                                uint32_t          min, 
                                uint32_t          max,
                                ts_algo_comprsc_t cmp,
                                char dir)
{
	if (dir < 0) {
		int i = (int)min+1;
		while(i >= 0) {
			ts_algo_cmp_t r = cmp(NULL,key,keys+i*ksize);
			if (r == ts_algo_cmp_equal) return i;
			if (r == ts_algo_cmp_greater) return i+1;
			i--;
		}
		return 0;
	} else {
		int i = (int)min;
		while(i < max) {
			ts_algo_cmp_t r = cmp(NULL,key,keys+i*ksize);
			if (r == ts_algo_cmp_equal) return i;
			if (r == ts_algo_cmp_less) return i;
			i++;
		}
		return max;
	}
}

/* ------------------------------------------------------------------------
 * Binary search
 * -------------
 * Binary search between min and max.
 * If the key is found, its slot is returned.
 * Otherwise, we call binsearch with the appropriate direction:
 * - to the left (dir=-1) if key < keys[slot] or
 * - to the right (dir=1) if key > keys[slot].
 * If we do not find the key before max/step = 1,
 * we continue with linsearch, which
 * returns the slot with the least key >= key
 * ------------------------------------------------------------------------
 */
static inline int32_t binsearch(char *keys, void *key,
                                uint32_t        ksize,
                                uint32_t          min,
                                uint32_t          max,
                                uint32_t         step,
                                ts_algo_comprsc_t cmp,
                                char              dir)
{
	int h = max/step;
	if (h <= 1) {
		return linsearch(keys, key, ksize, min, max, cmp, dir);
	}
	int32_t i = (int)min + (int)(dir*h);
	char r = cmp(NULL,key,keys+i*ksize);
	if (r == ts_algo_cmp_equal) return i;
	if (r == ts_algo_cmp_less) {
		return binsearch(keys,key,ksize,i,max,step*2,cmp,-1);
	}
	return binsearch(keys,key,ksize,i,max,step*2,cmp,1);
}

/* ------------------------------------------------------------------------
 * Add (key,data) to node
 * ------------------------------------------------------------------------
 */
beet_err_t beet_node_add(beet_node_t     *node,
                         uint32_t        nsize,
                         uint32_t        ksize,
                         uint32_t        dsize,
                         const void       *key,
                         const void      *data,
                         ts_algo_comprsc_t cmp,
                         beet_ins_t       *ins,
                         char           *wrote) 
{
	beet_err_t err;

	*wrote = 0;

	if (node->size >= nsize) {
		fprintf(stderr, "node size invalid: %u / %u\n", node->size, nsize);
		return BEET_ERR_INVALID;
	}

	/* find slot */
	int32_t slot = binsearch(node->keys, (void*)key, ksize,
	                             0, node->size, 2, cmp, 1);
	if (slot < 0) {
		fprintf(stderr, "slot invalid\n");
		return BEET_ERR_INVALID;
	}

	/* if we already have that node: add the data */
	if (slot < node->size && beet_node_equal(node,slot,ksize,key,cmp)) {
		if (node->leaf) {
			*wrote = 1;
			return ad3ata(node, dsize, slot, data, ins);
		}
		return BEET_OK;
	}

	/* otherwise: add the key */
	err = add2slot(node, ksize, dsize, slot, key, data, ins);
	if (err != BEET_OK) return err;

	*wrote = 1;
	node->size++;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Search link to follow
 * ------------------------------------------------------------------------
 */
beet_pageid_t beet_node_searchPageid(beet_node_t *node,
                                     uint32_t    keysz,
                                     const void   *key,
                                 ts_algo_comprsc_t cmp) {
	int idx = binsearch(node->keys, (void*)key, keysz,
	                        0, node->size, 2, cmp, 1);
	if (idx >= 0 && idx <= node->size) {
		if (idx < node->size &&
		    beet_node_equal(node, idx, keysz, key, cmp)) idx++;
		return beet_node_getPageid(node,idx);
	}
	return BEET_PAGE_NULL;
}

/* ------------------------------------------------------------------------
 * Generic search
 * ------------------------------------------------------------------------
 */
int32_t beet_node_search(beet_node_t     *node,
                         uint32_t        keysz,
                         const void       *key,
                         ts_algo_comprsc_t cmp) {
	return binsearch(node->keys, (void*)key, keysz,
	                     0, node->size, 2, cmp, 1);
}

/* ------------------------------------------------------------------------
 * Test that key at slot is equal to 'key'
 * ------------------------------------------------------------------------
 */
char beet_node_equal(beet_node_t *node,
                     uint32_t     slot,
                     uint32_t    keysz,
                     const void   *key,
                 ts_algo_comprsc_t cmp) {
	return (cmp(NULL,(void*)key,node->keys+slot*keysz) ==
	                                ts_algo_cmp_equal);
}

