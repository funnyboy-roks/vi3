#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

bool strip_prefix(char **str, const char *prefix) {
    int prefix_len = strlen(prefix);
    if (strncmp(*str, prefix, prefix_len) == 0) {
        *str += prefix_len;
        return true;
    }
    return false;
}

bool split_once(char *str, char delim, char **right) {
    char *ptr;
    if ((ptr = strchr(str, delim)) == NULL) {
        return false;
    }

    *right = ptr + 1;
    *ptr = '\0';
    return true;
}

void trim_start(char **str) {
    for (; isspace(**str) && **str != '\0'; *str += 1);
}

void trim_end(char *str) {
    int len = strlen(str);
    for(; isspace(str[--len]); str[len] = '\0');
}

void trim(char **str) {
    trim_start(str);
    trim_end(*str);
}

typedef struct Tree_t {
    char *command; 
    char *key;
    struct Tree_t **children;
    size_t children_count;
    size_t children_capacity;
} Tree;

size_t max(size_t a, size_t b) { return a > b ? a : b; };

void add_to_tree(Tree *root, char **steps, size_t steps_count, char *command) {
    Tree *curr = root;
    for (size_t i = 0; i < steps_count; ++i) {
        bool found = false;
        for (size_t t = 0; !found && t < curr->children_count; ++t) {
            if (!strcmp(curr->children[t]->key, steps[i])) {
                curr = curr->children[t];
                found = true;
            }
        }

        if (found) continue;

        if (!found) {
            // we must add child
            Tree *new_child = malloc(sizeof(Tree));
            // resize
            if (curr->children_count == curr->children_capacity) {
                Tree **new_children = malloc(sizeof(Tree *) * max((curr->children_capacity *= 2), 1));
                memcpy(new_children, curr->children, sizeof(Tree *) * curr->children_count);
                curr->children = new_children;
            }
            new_child->key = strdup(steps[i]);
            if (i == steps_count - 1) {
                new_child->command = command;
            }
            // insert new child
            curr->children[curr->children_count++] = new_child;
            curr = new_child;
        }
    }
}

void print_tree(Tree tree, size_t indent) {
    char id[indent + 1];
    for (size_t i = 0; i < indent; ++i) {
        id[i] = '\t';
    }
    id[indent] = '\0';

    fprintf(stderr, "%s%s -> %s\n", id, tree.key, tree.command);
    for (size_t i = 0; i < tree.children_count; i++) {
        print_tree(*tree.children[i], indent + 1);
    }
}

void write_tree(FILE *out, Tree tree, const char *prefix, const char **exits, size_t exits_len) {
    fprintf(out, "mode \"%s%s\" {\n", prefix, tree.key);
    size_t keylen = strlen(tree.key);
    size_t prefixlen = strlen(prefix);
    char *new_prefix = malloc(prefixlen + keylen + 1);
    memcpy(new_prefix, prefix, prefixlen);
    memcpy(new_prefix + prefixlen, tree.key, keylen);
    for (size_t i = 0; i < tree.children_count; ++i) {
        if (tree.children[i]->command == NULL) {
            fprintf(out, "    bindsym %s mode \"%s%s\"\n", tree.children[i]->key, new_prefix, tree.children[i]->key);
        } else {
            fprintf(out, "    bindsym %s %s; mode \"default\"\n", tree.children[i]->key, tree.children[i]->command);
        }
    }
    for (size_t i = 0; i < exits_len; ++i) {
        fprintf(out, "    bindsym %s mode \"default\"\n", exits[i]);
    }
    fprintf(out, "}\n");
    for (size_t i = 0; i < tree.children_count; ++i) {
        if (tree.children[i]->command == NULL) {
            write_tree(out, *tree.children[i], new_prefix, exits, exits_len);
        }
    }
    free(new_prefix);
}

#define RETURN(status)      \
do {                        \
    return_status = status; \
    goto done;              \
} while (0);


int main(int argc, char **argv) {
    char *config_path, *out_path;
    FILE *config, *out;
    int return_status = 0;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input-file> <output-file>\n", argv[0]);
        RETURN(1);
    }

    config_path = argv[1];
    out_path = argv[2];

    if (!strcmp(config_path, "-")) {
        config = stdin;
    } else {
        config = fopen(config_path, "rb");
        if (config == NULL) {
            fprintf(stderr, "Unable to open %s for reading: %m\n", config_path);
            RETURN(1);
        }
    }

    if (!strcmp(out_path, "-")) {
        out = stdout;
    } else {
        out = fopen(out_path, "wb");
        if (out == NULL) {
            fprintf(stderr, "Unable to open %s for writing: %m\n", out_path);
            RETURN(1);
        }
    }

    char *exits[16] = {0}; // keys that can be used for exit (mode "default)
    size_t exits_len = 0;

    char *steps[16] = {0}; // array of steps (each represented as a character)
    size_t steps_len = 0;

    int len;
    char *line = NULL;
    Tree tree = {0};
    tree.key = "\0";
    for (size_t n = 0; (len = getline(&line, &n, config)) != -1;) {
        trim(&line);
        if (*line == ';') { // vim(ish)-style comments, because why not
            continue;
        }
        if (strip_prefix(&line, "exit ")) { // prefix=$mod+v
            trim_start(&line);
            exits[exits_len++] = strdup(line);
            continue;
        }

        char *command;
        
        if (split_once(line, ' ', &command)) { // a,b command
            fprintf(stderr, "%s -> %s\n", line, command);
            char *line2 = line;
            for (char *t; (t = strtok(line2, ",")); line2 = NULL) {
                steps[steps_len++] = t;
            }
        }
        add_to_tree(&tree, steps, steps_len, strdup(command));
        steps_len = 0;
    }

    print_tree(tree, 0);
    write_tree(out, tree, "Vim: ", (const char **) exits, exits_len);

done:
    if (config) fclose(config);
    if (out) fclose(out);
    return return_status;
}
