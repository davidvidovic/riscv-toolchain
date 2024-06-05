#include "ir.h"

/*
* Function that walks the AST and creates IR tree based on it
*/

int register_counter = 1;
int label_counter = 1;
extern int sp_offset;

IR_node* populate_IR(ASTnode *root, IR_node *head, Stack *stack, Stack *secondary_stack)
{
    IR_node *this_node = NULL;

    if(root->nodetype == FUNCTION_NODE)
    { 
        this_node = insert_IR(root, head, stack, secondary_stack);

        if(root->right != NULL) 
        {
            this_node = populate_IR(root->right, this_node, stack, secondary_stack);
        }
    }
    else
    {
        if(root->right != NULL) 
        {
            this_node = populate_IR(root->right, head, stack, secondary_stack);
        }
    }


    


    /* Prio execution */

    if(root->nodetype == IF_NODE || root->nodetype == ELSE_NODE)
    { 
        this_node = insert_IR(root, this_node, stack, secondary_stack);
    }

    if(root->right != NULL)
    {
        if(root->right->nodetype == ELSE_NODE || root->right->nodetype == WHILE_NODE || root->right->nodetype == FOR_NODE)
        {    
            this_node = insert_IR(root, this_node, stack, secondary_stack);
        }
    }


    /* Normal execution */
    if(root->left != NULL && root->nodetype != FUNCTION_NODE) // watch out when added parameters to non-main function
    {
        if(this_node == NULL)
            this_node = populate_IR(root->left, head, stack, secondary_stack);
        else
            this_node = populate_IR(root->left, this_node, stack, secondary_stack);
    }

    if(this_node != NULL)
        head = this_node; 

    if(root->nodetype == FUNCTION_NODE)
    {
        char *ls = malloc(5 * sizeof(char));
        sprintf(ls, "L%d", root->right->value.label_count);
        while(stack->top != NULL)
        {
            IR_node *help = pop(stack);
            if(help->instr_type == JAL)
                help->rs1.label = ls;
            else
                help->rd.label = ls;
        }

        return this_node;
    }

    if(root->nodetype == ELSE_NODE 
        || (root->right != NULL && (root->right->nodetype == ELSE_NODE || root->right->nodetype == WHILE_NODE || root->right->nodetype == FOR_NODE))
        )
        return this_node;
    else
        return insert_IR(root, head, stack, secondary_stack);
}

/*
* Create head
*/

IR_node* create_IR()
{
    IR_node *IR_head = (IR_node *)malloc(sizeof(IR_node));
    IR_head->ir_type = NONE;
    IR_head->instr_type = HEAD;
    IR_head->next = NULL;
    IR_head->prev = NULL;
    IR_head->reg = 0;
    return IR_head;
}

/*
* Create base IR node
*/

IR_node* insert_IR(ASTnode *root, IR_node *head, Stack *stack, Stack *secondary_stack)
{ 
    IR_node *node = (IR_node *)malloc(sizeof(IR_node));
    
    node->prev = NULL;
    node->next = head;
    head->prev = node;
    
    node->reg = register_counter++;
    node->ir_type = NONE;
    node->instr_type = IR_NO_TYPE;

    char *tmp = malloc(5 * sizeof(char));
    IR_node *jmp = (IR_node *)malloc(sizeof(IR_node));

    switch(root->nodetype)
    {
        case CONSTANT_NODE:
            node->ir_type = IR_LOAD_IMM;
            node->instr_type = LUI;
            node->instruction = "lui";
            node->rd.reg = node->reg;

            switch(root->type)
            {
                case TYPE_INT:
                    node->rs1.int_constant = root->value.value_INT;
                break;

                case TYPE_FLOAT:
                    /*
                    * NOT SUPPORTING FLOATING POINT SO NEEDS TO BE SEEN HOW IT IS IMPLEMENTED
                    */
                    node->rs1.float_constant = root->value.value_FLOAT;
                break;

                case TYPE_CHAR:
                    node->rs1.char_constant = root->value.value_CHAR;
                break;
            }

        break;

        case ID_NODE:
            node->instr_type = LW;
            node->instruction = "lw";
            node->rs1.name = root->name;
            node->rd.reg = node->reg;      

        break;

        case OPERATION_NODE:
            switch(root->operation)
            {
                case EQU_OP:
                    node->instr_type = SW;
                    node->instruction = "sw";
                    node->rd.name = node->next->rs1.name;
                    
                    // Delete LW node created by visiting right ID node from list, it is not needed
                    node->next = (node->next)->next;
                    (node->next)->prev = node;

                    node->rs1.reg = node->next->rd.reg;
                break;

                case ADD_OP:
                    node->instr_type = ADD;
                    node->instruction = "add";
                    node->rd.reg = node->reg;
                    node->rs1.reg = node->next->rd.reg;
                    node->rs2.reg = (node->next)->next->rd.reg;
                break;

                case SUB_OP:
                    node->instr_type = SUB;
                    node->instruction = "sub";
                    node->rd.reg = node->reg;
                    node->rs1.reg = node->next->rd.reg;
                    node->rs2.reg = (node->next)->next->rd.reg;
                break;

                /*
                * If it is logical expression it should be followed by branch or jump instructions
                * No matter if it is expression evaluating if statement or value assigment to variable
                */
                case LOGIC_EQU_OP:
                    node->instr_type = BNE;
                    node->ir_type = IR_BRANCH;
                    node->instruction = "bne";
                    node->rs1.reg = node->next->reg;
                    node->rs2.reg = node->next->next->reg;    
                    push(stack, node);       
                break;

                case LOGIC_GET_OP:
                    node->instr_type = BLT;
                    node->ir_type = IR_BRANCH;
                    node->instruction = "blt";
                    node->rs1.reg = node->next->next->reg; 
                    node->rs2.reg = node->next->reg; 
                    push(stack, node); 
                break;

                case LOGIC_GT_OP:
                /*
                * BLE - branch if less or equal => is BGE when operand are reversed
                */
                    node->instr_type = BLE;
                    node->ir_type = IR_BRANCH;
                    node->instruction = "bge";
                    node->rs2.reg = node->next->next->reg; 
                    node->rs1.reg = node->next->reg; 
                    push(stack, node); 
                break;

                case LOGIC_LET_OP:
                /*
                * BGT - branch if greater than => is BLT when operand are reversed
                */
                    node->instr_type = BGT;
                    node->ir_type = IR_BRANCH;
                    node->instruction = "blt";
                    node->rs2.reg = node->next->next->reg; 
                    node->rs1.reg = node->next->reg; 
                    push(stack, node); 
                break;

                case LOGIC_LT_OP:
                    // godbolt does optimization by reducing constant operand by one... and using diff instr
                    node->instr_type = BGE;
                    node->ir_type = IR_BRANCH;
                    node->instruction = "bge";
                    node->rs1.reg = node->next->next->reg; 
                    node->rs2.reg = node->next->reg; 
                    push(stack, node);  
                break;

                case LOGIC_NEQU_OP:
                    node->instr_type = BEQ;
                    node->ir_type = IR_BRANCH;
                    node->instruction = "beq";
                    node->rs1.reg = node->next->next->reg; 
                    node->rs2.reg = node->next->reg; 
                    push(stack, node); 
                break;

                case LOGIC_NOT_OP:
                    node->instr_type = SLTIU;
                    node->instruction = "sltiu";
                    node->rd.reg = node->next->reg;
                    node->rs1.reg = node->next->reg; 
                    node->rs2.int_constant = 1;

                    IR_node *nxt = (IR_node *)malloc(sizeof(IR_node));
                    nxt->next = node;
                    nxt->prev = NULL;
                    node->prev = nxt;

                    nxt->instr_type = ANDI;
                    nxt->instruction = "andi";
                    nxt->rd.reg = node->rd.reg;
                    nxt->rs1.reg = node->rs1.reg;
                    nxt->rs2.int_constant = 0xff;

                    return nxt;
                break;

                case LOGIC_AND_OP:
                    if(root->right->nodetype != OPERATION_NODE)
                    {
                        IR_node *temp_node = (IR_node *)malloc(sizeof(IR_node));

                        temp_node->next = head->next;
                        head->next->prev = temp_node;       
                        head->next = temp_node;
                        temp_node->prev = head;
                        
                        temp_node->reg = register_counter++;
                        temp_node->instr_type = BEQ;
                        temp_node->ir_type = IR_BRANCH;
                        temp_node->instruction = "beq";
                        temp_node->rs1.reg = temp_node->next->reg; 
                        temp_node->rs2.reg = 0; 

                        push(stack, temp_node);
                    }

                    node->next = head;
                    node->prev = NULL;
                    head->prev = node;

                    node->instr_type = BEQ;
                    node->ir_type = IR_BRANCH;
                    node->instruction = "beq";
                    node->rs1.reg = node->next->reg; 
                    node->rs2.reg = 0; 

                    push(stack, node); 
                    break;


                case LOGIC_OR_OP:
                    /* Checking if this is the root of logical sub-tree (boolean exp for if is deeper than 2 operands) */
                    if(root->right->nodetype != OPERATION_NODE) 
                    {
                        IR_node *temp_node = (IR_node *)malloc(sizeof(IR_node));

                        //printf("HEAD IS: %s R%d %s\n", head->instruction, head->rd.reg, head->rs1.name);

                        temp_node->next = head->next;
                        head->next->prev = temp_node;       
                        head->next = temp_node;
                        temp_node->prev = head;
                        
                        temp_node->reg = register_counter++;
                        temp_node->instr_type = BNE;
                        node->ir_type = IR_BRANCH;
                        temp_node->instruction = "bne";
                        temp_node->rs1.reg = temp_node->next->reg; 
                        temp_node->rs2.reg = 0; 

                        push(secondary_stack, temp_node); 

                        node->next = head;
                        node->prev = NULL;
                        head->prev = node;

                        node->instr_type = BEQ;
                        node->ir_type = IR_BRANCH;
                        node->instruction = "beq";
                        node->rs1.reg = node->next->reg; 
                        node->rs2.reg = 0; 

                        push(stack, node);
                    }
                    else if(root->right->operation == LOGIC_OR_OP)
                    {
                        head->next->instr_type = BNE;
                        node->ir_type = IR_BRANCH;
                        head->next->instruction = "bne";
                        push(secondary_stack, pop(stack)); // holy shit

                        node->next = head;
                        node->prev = NULL;
                        head->prev = node;

                        node->instr_type = BEQ;
                        node->ir_type = IR_BRANCH;
                        node->instruction = "beq";
                        node->rs1.reg = node->next->reg; 
                        node->rs2.reg = 0; 

                        push(stack, node);
                    }
                    else
                    {
                        // when AND op is on the right (mixed operations example)

                        node->instr_type = BNE;
                        node->ir_type = IR_BRANCH;
                        node->instruction = "bne";
                        node->rs1.reg = node->next->reg; 
                        node->rs2.reg = 0;
                        push(secondary_stack, node);

                        /* First insert current head - lui, into a proper spot (4 instructions deep) */
                        IR_node *temp = head;
                        
                        head = head->next;
                        head->prev = NULL;

                        temp->next = head->next->next->next->next;
                        head->next->next->next->next->prev = temp;
                        temp->prev = head->next->next->next;
                        temp->prev->next = temp;

                        /* Next create this node of type bne and insert it again 4 instructions deep, and return head */
                        node->next = head->next->next->next->next;
                        head->next->next->next->next->prev = node;
                        node->prev = head->next->next->next;
                        node->prev->next = node;

                        return head;
                    }
                break;

                case BITWISE_NOT_OP:
                    /* Pseudo NOT by ISA docs */
                    node->instr_type = XORI;
                    node->instruction = "xori";
                    node->rd.reg = node->next->reg;
                    node->rs1.reg = node->next->reg;
                    node->rs2.int_constant = -1;
                break;
            }
        break;

        case IF_NODE:
            root->value.label_count = label_counter++;
            node->instr_type = LABEL;
            node->ir_type = IR_LABEL;
            
            sprintf(tmp, "L%d", root->value.label_count);
            node->instruction = tmp;
            root->left->value.label_count = root->value.label_count;

            while(secondary_stack->top != NULL)
            {
                IR_node* help = pop(secondary_stack);
                if(help->instr_type == JAL)
                    help->rs1.label = tmp;
                else
                    help->rd.label = tmp;
            }
        break;

        case ELSE_NODE:           
            jmp->instr_type = JAL;
            jmp->ir_type = IR_JUMP;
            jmp->instruction = "jal";
            jmp->rd.reg = 0; // by ISA docs - pseudo j (jamp) instruction is jal with rd set as x0
            jmp->next = head->next; // delete IFs node
            jmp->prev = NULL;
            head->next->prev = jmp;

            node->next = jmp;
            node->prev = NULL;
            jmp->prev = node;


            root->value.label_count = root->right->value.label_count;

            node->instr_type = LABEL;
            node->ir_type = IR_LABEL;
            
            sprintf(tmp, "L%d", root->value.label_count);
            node->instruction = tmp;
            root->left->value.label_count = root->value.label_count;

            IR_node *help = pop(stack);
            if(help->instr_type == JAL)
                help->rs1.label = tmp;
            else
                help->rd.label = tmp;

            ASTnode *tail = root->right->right;
            tail->right = NULL;
            int num_AND = count_subtree_AND_OPs(tail);

            for(int i = 0; i < num_AND; i++)
            {
                help = pop(stack);
                if(help->instr_type == JAL)
                    help->rs1.label = tmp;
                else
                    help->rd.label = tmp;
            }
            
            push(stack, jmp);
        break;

        case EXPRESSION_NODE:
            if(root->right != NULL)
            {
                char *tmp = malloc(5 * sizeof(char));
                if(root->right->nodetype == IF_NODE)
                {
                    //node->instr_type = LABEL;
                    root->value.label_count = root->right->value.label_count;
                    //root->value.label_count = label_counter++;
                    
                    sprintf(tmp, "L%d", root->value.label_count);
                    node->instruction = tmp;                    

                    IR_node *help = pop(stack);
                    if(help->instr_type == JAL)
                        help->rs1.label = tmp;
                    else
                        help->rd.label = tmp;

                    ASTnode *tail = root->right->right;
                    tail->right = NULL;

                    /* 
                    * When there are multiple subexpressions or operands in logical expressions, for each branching
                    * there will be and instruction node created and it will be pushed to stack, so I make a temporary sub-tree
                    * and cut sub-tree from the rest of AST by inserting NULL pointer to its tail, so I traverse only subtree from current above-IF node
                    * down to boolean expression of if node and count how many times I need to pop nodes off the stack 
                    */

                    int num_AND = count_subtree_AND_OPs(tail);

                    for(int i = 0; i < num_AND; i++)
                    {
                        help = pop(stack);
                        if(help->instr_type == JAL)
                            help->rs1.label = tmp;
                        else
                            help->rd.label = tmp;
                    }
                }
                else if(root->right->nodetype == ELSE_NODE)
                {
                    node->instr_type = LABEL;
                    node->ir_type = IR_LABEL;
                    root->value.label_count = label_counter++;
                    sprintf(tmp, "L%d", root->value.label_count);
                    node->instruction = tmp;                    

                    IR_node *help = pop(stack);
                    if(help->instr_type == JAL)
                        help->rs1.label = tmp;
                    else
                        help->rd.label = tmp;
                }
                else if(root->right->nodetype == WHILE_NODE)
                {
                    node->instr_type = LABEL;
                    node->ir_type = IR_LABEL;
                    root->value.label_count = label_counter++;
                    
                    sprintf(tmp, "L%d", root->value.label_count);
                    node->instruction = tmp; 

                    IR_node *help = pop(stack);
                    sprintf(tmp, "L%d", root->value.label_count);
                    if(help->instr_type == JAL)
                        help->rs1.label = tmp;
                    else
                        help->rd.label = tmp;

                    ASTnode *tail = root->right->right->right;
                    tail->right = NULL;

                    int num_AND = count_subtree_AND_OPs(tail);

                    for(int i = 0; i < num_AND; i++)
                    {
                        help = pop(stack);
                        if(help->instr_type == JAL)
                            help->rs1.label = tmp;
                        else
                            help->rd.label = tmp;
                    }
                }
                else if(root->right->nodetype == FOR_NODE)
                {
                    node->instr_type = LABEL;
                    node->ir_type = IR_LABEL;
                    root->value.label_count = label_counter++;
                    sprintf(tmp, "L%d", root->value.label_count);
                    node->instruction = tmp; 

                    IR_node *help = pop(stack);
                    sprintf(tmp, "L%d", root->value.label_count);
                    if(help->instr_type == JAL)
                        help->rs1.label = tmp;
                    else
                        help->rd.label = tmp;

                    ASTnode *tail = root->right->right;
                    tail->right = NULL;

                    int num_AND = count_subtree_AND_OPs(tail);

                    for(int i = 0; i < num_AND; i++)
                    {
                        help = pop(stack);
                        if(help->instr_type == JAL)
                            help->rs1.label = tmp;
                        else
                            help->rd.label = tmp;
                    }
                }
            }
        break;

        case SCOPE_NODE:
            if(root->left != NULL)
            {
                if(root->left->nodetype != EXPRESSION_NODE) // when something other than exp is to the left of scope
                {
                    node->instr_type = LABEL; 
                    node->ir_type = IR_LABEL;
                    char *tmp = malloc(5 * sizeof(char));
                    root->value.label_count = label_counter++;
                    sprintf(tmp, "L%d", root->value.label_count);
                    node->instruction = tmp;     
                }
                else
                {
                    root->value.label_count = root->left->value.label_count;
                }
            }      
        break;

        case FUNCTION_NODE:
            node->instr_type = LABEL;
            node->ir_type = IR_LABEL;
            node->instruction = root->left->name;

            /* Stack frame allocation time */

        break;

        case WHILE_NODE:
            //IR_node *temp_node = (IR_node *)malloc(sizeof(IR_node));
            
            node->instr_type = JAL;
            node->ir_type = IR_JUMP;
            node->instruction = "jal";
            node->rd.reg = 0; // by ISA docs - pseudo j (jump) instruction is jal with rd set as x0
            //push(stack, node);
            sprintf(tmp, "L%d", root->right->right->right->value.label_count);
            node->rs1.label = tmp;
            root->value.label_count = root->right->right->right->value.label_count;
            //root->value.label_count = label_counter++;
        break;

        case FOR_NODE:
            node->instr_type = JAL;
            node->ir_type = IR_JUMP;
            node->instruction = "jal";
            node->rd.reg = 0; // by ISA docs - pseudo j (jump) instruction is jal with rd set as x0
            sprintf(tmp, "L%d", root->right->right->value.label_count);
            node->rs1.label = tmp;
            root->value.label_count = root->right->right->value.label_count;
            //root->value.label_count = label_counter++;
        break;

        case LABEL_NODE:
            /* This is a special type of node for inserting label before while boolean expression is evaluated */
            node->instr_type = LABEL;
            node->ir_type = IR_LABEL;
            root->value.label_count = label_counter++;
            sprintf(tmp, "L%d", root->value.label_count);
            node->instruction = tmp; 

            /* Risky thing about to happen */
            while(secondary_stack->top != NULL)
            {
                IR_node *help = pop(secondary_stack);
                if(help->instr_type == JAL)
                    help->rs1.label = tmp;
                else
                    help->rd.label = tmp;
            }
        break;
    }


    if(node->instr_type == IR_NO_TYPE)
        return head;
    return node;
}

int count_subtree_AND_OPs(ASTnode* root)
{
    int temp = 0;

    if(root->right != NULL)
        temp += count_subtree_AND_OPs(root->right);

    if(root->left != NULL)
        temp += count_subtree_AND_OPs(root->left); 

    if(root->operation == LOGIC_AND_OP)
    {
        temp++;
    }

    return temp;
}


/*
* Create LIFO node for LIFO queue for labels
*/

void init_stack(Stack* stack)
{
    stack->top = NULL;
}

LIFO_node* create_LIFO_node(IR_node *node)
{
    LIFO_node* item = (LIFO_node*)malloc(sizeof(LIFO_node));
    item->ptr = node;
    item->next = NULL;
    return item;
}

void push(Stack *stack, IR_node *node)
{
    LIFO_node* item = create_LIFO_node(node);

    if(stack->top == NULL)
    {
        stack->top = item;
    }
    else
    {
        item->next = stack->top;
        stack->top = item;
    }
}

IR_node* pop(Stack *stack)
{
    if(stack->top == NULL)
    {
        printf("Stack empty\n");
        return NULL;
    }
    
    LIFO_node *temp = stack->top;
    IR_node *ptr = temp->ptr;
    if(temp->next == NULL)
    {
        stack->top = NULL;
    }
    else
    {
        stack->top = temp->next;
    }
    return ptr;
}


void print_IR(IR_node *IR_head, IR_node *IR_tail)
{
    printf("\n\nASM:\n\n");
    FILE *asm_file = fopen("output.s", "w");

    while(IR_head != IR_tail)
    {
      IR_head = IR_head->prev;
      switch(IR_head->instr_type)
      {
        case LABEL:
          printf(".%s:\n", IR_head->instruction);
          fprintf(asm_file, ".%s:\n", IR_head->instruction);
        break;
        
        case SW:
          printf("\t%s %s,R%d\n", IR_head->instruction, IR_head->rd.name, IR_head->rs1.reg);
          fprintf(asm_file, "\t%s %s,R%d\n", IR_head->instruction, IR_head->rd.name, IR_head->rs1.reg);
        break;

        case LW:
          printf("\t%s R%d,%s\n", IR_head->instruction, IR_head->rd.reg, IR_head->rs1.name);
          fprintf(asm_file, "\t%s R%d,%s\n", IR_head->instruction, IR_head->rd.reg, IR_head->rs1.name);
        break;

        case IR_LOAD_IMM:
          printf("\t%s R%d,%d\n", IR_head->instruction, IR_head->rd.reg, IR_head->rs1.int_constant);
          fprintf(asm_file, "\t%s R%d,%d\n", IR_head->instruction, IR_head->rd.reg, IR_head->rs1.int_constant);
        break;        

        case ADD:
        case SUB:
          printf("\t%s R%d,R%d,R%d\n", IR_head->instruction, IR_head->rd.reg, IR_head->rs1.reg, IR_head->rs2.reg);
          fprintf(asm_file, "\t%s R%d,R%d,R%d\n", IR_head->instruction, IR_head->rd.reg, IR_head->rs1.reg, IR_head->rs2.reg);
        break;

        case BEQ:
        case BNE:
        case BLT:
        case BGE:
        case BLTU:
        case BGEU:
        case BGT:
        case BLE:
          printf("\t%s R%d,R%d,.%s\n", IR_head->instruction, IR_head->rs1.reg, IR_head->rs2.reg, IR_head->rd.label);
          fprintf(asm_file, "\t%s R%d,R%d,.%s\n", IR_head->instruction, IR_head->rs1.reg, IR_head->rs2.reg, IR_head->rd.label);
        break;

        case JAL:
          printf("\t%s R%d,.%s\n", IR_head->instruction, IR_head->rd.reg, IR_head->rs1.label);
          fprintf(asm_file, "\t%s R%d,.%s\n", IR_head->instruction, IR_head->rd.reg, IR_head->rs1.label);
        break;

        case XORI:
        case ANDI:
        case SLTIU:
          printf("\t%s R%d,R%d,%d\n", IR_head->instruction, IR_head->rd.reg, IR_head->rs1.reg, IR_head->rs2.int_constant);
          fprintf(asm_file, "\t%s R%d,R%d,%d\n", IR_head->instruction, IR_head->rd.reg, IR_head->rs1.reg, IR_head->rs2.int_constant);
        break;

        case ADDI: // for now like this!
          printf("\t%s %s,%s,%d\n", IR_head->instruction, IR_head->rd.name, IR_head->rs1.name, IR_head->rs2.int_constant);
          fprintf(asm_file, "\t%s %s,%s,%d\n", IR_head->instruction, IR_head->rd.name, IR_head->rs1.name, IR_head->rs2.int_constant);
        break;

        case IR_NO_TYPE:
        case HEAD:
        break;
      }
    }


    fclose(asm_file);
}