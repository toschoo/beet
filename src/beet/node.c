/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * B+Node Abstraction
 * ========================================================================
 *
 * The binary format of a node, which is stored in a page, is
 *
 * 1) Internal Node
 *    +------------------------------------------+
 *    | Size | Keys[nodesize] | Kids[nodesize+1] |
 *    +------------------------------------------+
 *     4byte  keysize*nodesz    4*(nodsz+1)
 *
 *    Here, keysize and nodesz mean the respective size
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
 *    +-------------------------------------------------------------------+
 *    | Size | Next | Prev | Control    | Keys[nodesize] | Kids[nodesize] |
 *    +-------------------------------------------------------------------+
 *     4byte  4byte  4byte   nodesz/8+1   keysize*nodesz   datasize*nodesz
 *
 *    The keys, as before, contain the keys of this tree and
 *    the kids contain the data for their keys.
 *
 *    Next points to the next leaf node in the chain.
 *    Prev points to the previous leaf node in the chain.
 *
 *    Control is a block of bits that indicate whether the key is
 *      - actually present or
 *      - hidden (i.e. soft-deleted)
 *
 *    This way, leaf nodes form a doubly linked list
 *    through which we can iterate scanning a range of keys
 *    (and their data) or even the entire tree.
 * ========================================================================
 */
#include <beet/node.h>
#include <string.h>

#define CTRLSZ BEET_NODE_CTRLSZ

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

	node->page  = page;
	node->self  = page->pageid;
	node->leaf  = leaf;

	memcpy(&node->size, page->data, sizeof(uint32_t));
	off += sizeof(uint32_t);
	if (leaf) {
		memcpy(&node->next, page->data+off, sizeof(uint32_t));
		off += sizeof(int32_t);
		memcpy(&node->prev, page->data+off, sizeof(uint32_t));
		off += sizeof(int32_t);
		node->ctrl = page->data+off; off += CTRLSZ(nodesz);

		/* debug
		uint16_t x = 0xdead;
		memcpy(node->ctrl, &x, 2);
		*/
	}

	// fprintf(stderr, "NODE SIZE : %d\n", node->size);
	
	node->keys = page->data+off; off += keysz*nodesz;
	node->kids = page->data+off;

	/* debug
	fprintf(stderr, "CTRL BLOCK (%u): ", CTRLSZ(nodesz));
	for(int i=1;i>=0;i--) {
		fprintf(stderr, "%x", (uint8_t)*(node->ctrl+i));
	}
	fprintf(stderr, "\n");
	*/
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
                                char          upd,
                                beet_ins_t   *ins) {
	if (ins == NULL) return BEET_OK;
	return ins->inserter(ins->rsc, upd, dsize,
	             node->kids+slot*dsize, data);
}

/* ------------------------------------------------------------------------
 * Helper: test if key at slot is hidden
 * ------------------------------------------------------------------------
 */
static inline uint8_t hidden(beet_node_t  *node,
                             uint32_t      slot) {
	int     y = slot/8;
	int     i = slot%8;
	uint8_t m = 1<<i;
	return (node->ctrl[y] & m);
}

/* ------------------------------------------------------------------------
 * Helper: set hidden
 * ------------------------------------------------------------------------
 */
static inline void hide(beet_node_t *node,
                        uint32_t     slot) {
	int     y = slot/8;
	int     i = slot%8;
	uint8_t m = 1<<i;
	node->ctrl[y] |= m;
}

/* ------------------------------------------------------------------------
 * Helper: unset hidden
 * ------------------------------------------------------------------------
 */
static inline void unhide(beet_node_t *node,
                          uint32_t     slot) {
	int     y = slot/8;
	int     i = slot%8;
	uint8_t m = 1<<i;

	if ((node->ctrl[y] & m)) {
		node->ctrl[y] ^= m;
	}
}

/* ------------------------------------------------------------------------
 * Helper: map for shifting control block
 * ------------------------------------------------------------------------
 */
static inline uint8_t mkmap(int x) {
	int k=1<<(x+1);
	for(int i = x; i<8; i++) {
		k |= k<<1;
	}
	return k;
}

/* ------------------------------------------------------------------------
 * Helper: shift control block to match keys after shift
 * ------------------------------------------------------------------------
 */
static inline void shiftctrl(beet_node_t *node,
                             uint32_t     slot,
                             uint32_t    nsize) {
	int y = slot/8;
	int i = slot%8;

	uint8_t carry = node->ctrl[y] & 1;

	carry <<= 7; // the last will be the first of the next byte

	uint8_t m = mkmap(i);           // map keep the first part
	uint8_t n = node->ctrl[y] >> 1; // shift

	m &= node->ctrl[y];             // apply map to old
	n |= m;                         // apply modified map to new

	node->ctrl[y] = n;              // set result

	for (int z = y+1; z < nsize; z++) {

		uint8_t o = node->ctrl[z]; // old map
		uint8_t t = o & 1;         // carry

		n = o >> 1;                // shift
		n |= carry;                // apply carry

		node->ctrl[z] = n;         // set new map

		carry = t << 7;            // update carry
	}
}

/* ------------------------------------------------------------------------
 * Helper: add (key,data) to node
 * ------------------------------------------------------------------------
 */
static inline beet_err_t add2slot(beet_node_t *node,
                                  uint32_t    nsize,
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
	if (shift > 0) {
		memmove(src+ksize,src,shift);
		if (node->leaf) shiftctrl(node, slot, CTRLSZ(nsize));
	}

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
		ad3ata(node, dsize, slot, data, 1, ins);
	
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
 * Binary search
 * -------------
 * Searches key in keys.
 * If the key is found, its slot is returned.
 * Otherwise, we return the slot with the least key >= key
 *            if the key is greater than the greatest in the node,
 *            we return the first free slot.
 * ------------------------------------------------------------------------
 */
static inline int32_t binsearch(char          *keys, 
                                const void    *key,
                                uint32_t       ksize,
				uint32_t       size,
                                beet_compare_t cmp,
                                void          *rsc)
{
	int r = size; // the least key greater than key
	int s = 0;    // start index
	int e = r-1;  // end   index

	while(s<=e) {
		int i = (s+e)/2;
		char x = cmp(key,keys+i*ksize,rsc);
		if (x == BEET_CMP_EQUAL) return i;
		if (x == BEET_CMP_LESS) {
			e = i-1; // go left
			if (i < r) r = i; // this is the smallest greater one
		}
		else s = i+1; // go right
	}
	return r;
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
                         beet_compare_t    cmp,
                         void             *rsc,
                         beet_ins_t       *ins,
                         char              upd,
                         char           *wrote) 
{
	beet_err_t err;

	*wrote = 0;

	if (node->size >= nsize) return BEET_ERR_BADSIZE;

	/* find slot */
	int32_t slot = node->size == 0?0:
	               binsearch(node->keys, key, ksize, node->size, cmp, rsc);

	/* if we already have that key: add the data */
	if (slot < node->size &&
	    beet_node_equal(node,slot,ksize,key,cmp,rsc)) {
		if (node->leaf) {
			*wrote = 1;
			if (beet_node_hidden(node, slot)) unhide(node, slot); // unhide
			return ad3ata(node, dsize, slot, data, upd, ins);
		}
		return BEET_OK;
	}

	/* otherwise: add the key */
	err = add2slot(node, nsize, ksize, dsize, slot, key, data, ins);
	if (err != BEET_OK) return err;

	*wrote = 1;
	node->size++;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Search link to follow
 * ------------------------------------------------------------------------
 */
beet_pageid_t beet_node_searchPageid(beet_node_t  *node,
                                     uint32_t     keysz,
                                     const void    *key,
                                     beet_compare_t cmp,
                                     void          *rsc) {
	if (node->size == 0) return BEET_PAGE_NULL;
	int idx = binsearch(node->keys, key, keysz, node->size, cmp, rsc);
	if (idx >= 0 && idx <= node->size) {
		if (idx < node->size &&
		    beet_node_equal(node, idx, keysz, key, cmp, rsc)) idx++;
		return beet_node_getPageid(node,idx);
	}
	return BEET_PAGE_NULL;
}

/* ------------------------------------------------------------------------
 * Generic search
 * ------------------------------------------------------------------------
 */
int32_t beet_node_search(beet_node_t  *node,
                         uint32_t     keysz,
                         const void    *key,
                         beet_compare_t cmp,
                         void          *rsc) {
	if (node->size == 0) return -1;
	return binsearch(node->keys, key, keysz, node->size, cmp, rsc);
}

/* ------------------------------------------------------------------------
 * Test that key at slot is equal to 'key'
 * ------------------------------------------------------------------------
 */
uint8_t beet_node_equal(beet_node_t *node,
                        uint32_t     slot,
                        uint32_t    keysz,
                        const void   *key,
                       beet_compare_t cmp,
                       void          *rsc) {
	return (cmp(key,node->keys+slot*keysz,rsc) == BEET_CMP_EQUAL);
}

/* ------------------------------------------------------------------------
 * Test if key at slot is hidden
 * ------------------------------------------------------------------------
 */
uint8_t beet_node_hidden(beet_node_t  *node,
                         uint32_t      slot) {
	return hidden(node, slot);
}

/* ------------------------------------------------------------------------
 * Hide a given key
 * ------------------------------------------------------------------------
 */
void beet_node_hide(beet_node_t *node,
                    uint32_t     slot) {
	hide(node, slot);
}

/* ------------------------------------------------------------------------
 * Unhide a given key
 * ------------------------------------------------------------------------
 */
void beet_node_unhide(beet_node_t *node,
                      uint32_t     slot) {
	unhide(node, slot);
}
