/* ========================================================================
 * Simple Tests for node
 * ========================================================================
 */
#include <beet/beet.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void errmsg(beet_err_t err, char *msg) {
	fprintf(stderr, "%s: %s (%d)\n", msg, beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "%s\n", beet_oserrdesc());
	}
}

beet_config_t config;

int createIndex(beet_config_t *cfg) {
	beet_err_t err;

	err = beet_index_create("rsc/idx10", 1, cfg);
	if (err != BEET_OK) {
		errmsg(err, "cannot create index");
		return -1;
	}
	return 0;
}

int createDropIndex() {
	beet_err_t err;
	if (createIndex(&config) != 0) return -1;
	err = beet_index_drop("rsc/idx10");
	if (err != BEET_OK) {
		errmsg(err, "cannot drop index");
		return -1;
	}
	return 0;
}

int main() {
	int rc = EXIT_SUCCESS;

	config.indexType = BEET_INDEX_PLAIN;
	config.leafPageSize = 128;
	config.intPageSize = 128;
	config.leafNodeSize = 14;
	config.intNodeSize = 14;
	config.leafCacheSize = 10000;
	config.intCacheSize = 10000;
	config.keySize = 4;
	config.dataSize = 4;
	config.subPath = NULL;
	config.compare = NULL;

	if (createDropIndex() != 0) {
		fprintf(stderr, "createDropIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (createIndex(&config) != 0) {
		fprintf(stderr, "createIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

cleanup:
	if (rc == EXIT_SUCCESS) {
		fprintf(stderr, "PASSED\n");
	} else {
		fprintf(stderr, "FAILED\n");
	}
	return rc;
}
