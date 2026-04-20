// commit.c - complete implementation
// PROVIDED: commit_parse, commit_serialize, commit_walk, head_read, head_update

#include "pes.h"
#include "commit.h"
#include "tree.h"
#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *commit_create(const char *message) {
    // 1. Load the index
    Index index;
    if (index_load(&index, ".pes") < 0) return NULL;

    if (index.count == 0) {
        fprintf(stderr, "Nothing to commit (index is empty)\n");
        free(index.entries);
        return NULL;
    }

    // 2. Build tree from index
    char *tree_hash = tree_from_index(&index);
    free(index.entries);
    if (!tree_hash) return NULL;

    // 3. Read current HEAD (may be NULL for first commit)
    char *parent_hash = head_read();

    // 4. Get author string
    const char *author = pes_author();

    // 5. Get current timestamp
    time_t now = time(NULL);

    // 6. Build commit content string
    // Format:
    //   tree <hash>\n
    //   [parent <hash>\n]   <- only if parent exists
    //   author <author> <timestamp>\n
    //   committer <author> <timestamp>\n
    //   \n
    //   <message>\n
    char content[4096];
    int len = 0;

    len += snprintf(content + len, sizeof(content) - len, "tree %s\n", tree_hash);

    if (parent_hash && strlen(parent_hash) > 0) {
        len += snprintf(content + len, sizeof(content) - len, "parent %s\n", parent_hash);
    }

    len += snprintf(content + len, sizeof(content) - len,
                    "author %s %ld\n", author, (long)now);
    len += snprintf(content + len, sizeof(content) - len,
                    "committer %s %ld\n", author, (long)now);
    len += snprintf(content + len, sizeof(content) - len, "\n");
    len += snprintf(content + len, sizeof(content) - len, "%s\n", message);

    // 7. Write commit object
    char hash_hex[65];
    if (object_write("commit", (unsigned char *)content, len, hash_hex) < 0) {
        free(tree_hash);
        free(parent_hash);
        return NULL;
    }

    // 8. Update HEAD to point to new commit
    if (head_update(hash_hex) < 0) {
        free(tree_hash);
        free(parent_hash);
        return NULL;
    }

    printf("[main %s] %s\n", hash_hex, message);

    free(tree_hash);
    free(parent_hash);
    return strdup(hash_hex);
}

