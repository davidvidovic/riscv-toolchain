#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "riskyc.h"
#include "y.tab.h"
#include "symboltable.h"

extern int lineno;
extern FILE *yyin;
extern char *yytext;
ht* table;
ASTnode *root;

// Copied from ht.c
typedef struct {
    char* key;  // key is NULL if this slot is empty
    const char* value;
    int line;
    ASTnode* node;
} ht_entry;

struct ht {
    ht_entry* entries;  // hash slots
    size_t capacity;    // size of _entries array
    size_t length;      // number of items in hash table
};

typedef struct {
    char* key;
    const char* value;
    int line; 
    ASTnode* node;
} item;

void exit_nomem(void) {
    fprintf(stderr, "out of memory\n");
    exit(1);
}


int declare(const char* name, const char* datatype, int line, ASTnode* node)
{
  if(ht_get_key(table, name) != NULL) 
  {
    char error[100];
    strcpy(error, "\033[31mERROR \t\tIdentifier ");
    strcat(error, name);
    strcat(error, " already declared.\033[0m");

    strcat(error, "\nPrevious declaration at line ");
    int error_line = ht_get_line(table, name);
    char temp[10];
    sprintf(temp, "%d", error_line);
    strcat(error, temp);
    strcat(error, "\nError");
    yyerror(error);

    exit(1);
  }
  else
  {
    if(ht_set(table, name, datatype, line, node) == NULL) 
    {
      exit_nomem();
    }
    else
    {
      //printf("Added to table %s with value %s\n", ht_get_key(table, $2.name), $1);
      return 1;
    }
  }

  return 0;
}

ASTnode* check_declaration(const char* name)
{
  if(ht_get_key(table, name) == NULL) 
  {
    char error[100];
    strcpy(error, "\033[31mERROR \t\tIdentifier ");
    strcat(error, name);
    strcat(error, " not declared.\033[0m");
    strcat(error, "\nError");
    yyerror(error);

    exit(1);
  }

  return ht_get_ASTnode(table, name);
}



int walkAST(ASTnode *root)
{
  print_value(root);

  if(root->left != NULL) walkAST(root->left);
  if(root->right != NULL) walkAST(root->right);

  return 0;
}


void freeAST(ASTnode *root)
{
  if(root->left != NULL) freeAST(root->left);
  if(root->right != NULL) freeAST(root->right);

  free(root);
}


int main()
{
    table = ht_create();
    if (table == NULL) 
    {
        exit_nomem();
    }

    root = mkASTnode(NULL, NULL);

    yyin = fopen("input.txt", "r");
    int token;
    yyparse();

    for(int i = 0; i < table->capacity; i++) 
    { 
        if(table->entries[i].key != NULL) 
        {
            //printf("index %d:\t%s, ", i, table->entries[i].key);

            int* adr = table->entries[i].value;
            int off = 0;
            while(*adr != NULL) 
            {
            //printf("%c", (char)*adr);
            adr = table->entries[i].value + (++off);
            }

            //printf("\tat line %d", table->entries[i].line);
            //printf("\n");
        } 
        else
        {
            //printf("Index %d: Empty\n", i);
        } 
    }

    
    // Walk the AST
    walkAST(root);

    ht_destroy(table);
    freeAST(root);
    return 0;
}