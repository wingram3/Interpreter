/*   Define a doubly-linked list of heap-allocated strings. Write functions
     to insert, find, and delete items from it. Test them. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* node structure */
struct dllnode {
    char *word;
    int length;
    struct dllnode *prev;
    struct dllnode *next;
};

/* doubly linked list structure */
struct dll {
    struct dllnode *head;
    struct dllnode *tail;
    int size;
};

struct dllnode *create_node(const char *);
struct dll *create_dll(void);
void append_node(struct dll *, const char *);
void delete_node(struct dll *, const char *);
struct dllnode *lookup(struct dll *, const char *);
void print_list(struct dll *);
void free_list(struct dll *);


int main(void)
{
    struct dll *list = create_dll();

    append_node(list, "tebbis");
    append_node(list, "bepis");
    append_node(list, "abbis");
    append_node(list, "giganticus");

    printf("OG list: ");
    print_list(list);

    delete_node(list, "abbis");
    printf("List after deletion: ");
    print_list(list);

    struct dllnode *wordtofind;
    wordtofind = lookup(list, "bepis");
    if (wordtofind != NULL)
        printf("%s\n", wordtofind->word);
    else
        printf("word not found in list.\n");

    free_list(list);

    return 0;
}


/* create_node: initialize a new dllnode. */
struct dllnode *create_node(const char *word)
{
    struct dllnode *new_node = (struct dllnode *)malloc(sizeof(struct dllnode));
    if (new_node == NULL) {
        perror("Failed to allocate memory for new node.\n");
        exit(EXIT_FAILURE);
    }

    new_node->length = strlen(word);
    new_node->word = (char *)malloc(new_node->length + 1);
    if (new_node->word == NULL) {
        perror("Failed to allocate memory for new word.\n");
        free(new_node);
        exit(EXIT_FAILURE);
    }

    strcpy(new_node->word, word);
    new_node->prev = NULL;
    new_node->next = NULL;

    return new_node;
}


/* create_dll: initialize a doubly linked list. */
struct dll *create_dll(void)
{
    struct dll *list = (struct dll *)malloc(sizeof(struct dll));
    if (list == NULL) {
        perror("Failed to allocate memory for doubly linked list.\n");
        exit(EXIT_FAILURE);
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;

    return list;
}


/* append_node: append a dllnode to the end of a dll. */
void append_node(struct dll *list, const char *word)
{
    struct dllnode *new_node = create_node(word);

    if (list->tail == NULL) {
        list->head = new_node;
        list->tail = new_node;
    } else {
        new_node->prev = list->tail;
        list->tail->next = new_node;
        list->tail = new_node;
    }
    list->size++;
}


/* delete_node: delete a node from a doubly linked list. */
void delete_node(struct dll *list, const char *word)
{
    if (list == NULL || word == NULL)
        return;

    struct dllnode *current = list->head;
    while (current != NULL) {
        if (strcmp(current->word, word) == 0) {
            if (current == list->head && current == list->tail) {   /* only node in the list */
                list->head = NULL;
                list->tail = NULL;
            } else if (current == list->head) {                     /* first node in the list */
                list->head = current->next;
                if (list->head != NULL)
                    list->head->prev = NULL;
            } else if (current == list->tail) {                     /* last node in the list */
                list->tail = current->prev;
                if (list->tail != NULL)
                    list->tail->next = NULL;
            } else {                                                /* middle node */
                if (current->prev != NULL)
                    current->prev->next = current->next;
                if (current->next != NULL)
                    current->next->prev = current->prev;
            }
            free(current->word);
            free(current);
            list->size--;
            return;
        }
        current = current->next;
    }
}


/* lookup: traverse the list to find a word in a doubly linked list. */
struct dllnode *lookup(struct dll *list, const char *word)
{
    struct dllnode *np;

    for (np = list->head; np != NULL; np = np->next)
        if (strcmp(word, np->word) == 0)
            return np;
    return NULL;
}


/* print_list: print the words in the dll. */
void print_list(struct dll *list)
{
    struct dllnode *current = list->head;
    while (current != NULL) {
        printf("%s ", current->word);
        current = current->next;
    }
    printf("\n");
}


/* free_list: free the memory allocated for the list, its nodes and their words. */
void free_list(struct dll *list)
{
    struct dllnode *current = list->head;
    while (current != NULL) {
        struct dllnode *next = current->next;
        free(current->word);
        free(current);
        current = next;
    }
    free(list);
}
