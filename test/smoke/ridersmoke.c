/* ========================================================================
 * Simple Tests for rider
 * ========================================================================
 */
#include <beet/rider.h>
#include <common/math.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int initRider(beet_rider_t *rider, char *base, char *name) {
	beet_err_t err;
	err = beet_rider_init(rider, base, name, 64, 1000);
	if (err != BEET_OK) {
		fprintf(stderr, "cannot initialise rider: %s (%d)\n",
		                             beet_errdesc(err), err);
		if (err < BEET_OSERR_ERRNO) {
			fprintf(stderr, "OS error: %s\n", beet_oserrdesc());
		}
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

int testInitDestroy() {
	beet_rider_t rider;
	char *name = "test1.bin";
	char *path = "rsc";

	if (createFile(path, name) != 0) return -1;
	if (initRider(&rider, path, name) != BEET_OK) return -1;
	beet_rider_destroy(&rider);

	return 0;
}

int main() {
	int rc = EXIT_SUCCESS;

	if (testInitDestroy()) {
		fprintf(stderr, "testInitDestroy failed\n");
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

